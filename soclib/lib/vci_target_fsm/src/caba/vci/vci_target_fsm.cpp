/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "vci_target_fsm.h"
#include "register.h"
#include "base_module.h"

namespace soclib {
namespace caba {

#define tmpl(x) template<typename vci_param,bool default_target,bool support_llsc> \
    x VciTargetFsm<vci_param,default_target,support_llsc>

tmpl(/**/)::VciTargetFsm(
    soclib::caba::VciTarget<vci_param> &vci,
    const std::list<soclib::common::Segment> &seglist,
    const size_t fifo_depth )
    : p_vci(vci),
      m_atomic(support_llsc ? (1<<vci_param::S) : 0),
      m_segments(seglist.begin(), seglist.end()),
      m_fifo_depth(fifo_depth),
      m_rsp_info("m_rsp_info", fifo_depth)
{
}

tmpl(void)::_on_read_write(
    soclib::caba::BaseModule *owner_module,
    wrapper_read_t read_func,
    wrapper_write_t write_func )
{
    m_owner = owner_module;
    m_on_read_f = read_func;
    m_on_write_f = write_func;
}

tmpl(/**/)::~VciTargetFsm()
{
}

tmpl(void)::reset()
{
    m_state = TARGET_IDLE;
    m_atomic.clearAll();

    m_rsp_info.init();
}

tmpl(void)::transition()
{
	if ( p_vci.cmdval.read() &&
         p_vci.cmdack.read() )
    {
        m_served_word.srcid = p_vci.srcid.read();
        m_served_word.trdid = p_vci.trdid.read();
        m_served_word.pktid = p_vci.pktid.read();
        m_served_word.eop = p_vci.eop.read();

        switch(m_state)
        {
        case TARGET_IDLE:
        case TARGET_WRITE_RSP:
        case TARGET_READ_RSP:
        {
            const addr_t address = p_vci.address.read();
            /*
             * This variable tracks whether at least one segment was
             * reached, if we are not default target, this variable
             * will be optimized out by compiler
             */
            bool reached = false;

            m_served_word.error = vci_param::ERR_NORMAL;

            std::vector<soclib::common::Segment>::const_iterator seg;
            size_t i=0;
            for ( seg = m_segments.begin();
                  seg != m_segments.end() && !reached;
                  ++i, ++seg ) {
                if ( ! seg->contains(address) )
                    continue;

                reached = true;
                addr_t offset = address - seg->baseAddress();

                switch (p_vci.cmd.read())
                {
                case vci_param::CMD_WRITE:
                    if ( support_llsc )
                        m_atomic.accessDone( address );
                    m_served_word.rdata = 0;
                    if ((m_owner->*m_on_write_f)(i, offset, p_vci.wdata.read(), p_vci.be.read()))
                        m_state = TARGET_WRITE_RSP;
                    else {
                        if ( p_vci.eop.read() )
                            m_state = TARGET_IDLE;
                        else
                            m_state = TARGET_ERROR_RSP;
                        m_served_word.error = vci_param::ERR_GENERAL_DATA_ERROR;
                    }
                    break;

                case vci_param::CMD_STORE_COND:
                    assert(support_llsc && "Received a SC on a non-SC-supporting target");
                    if ( ! m_atomic.isAtomic( address, p_vci.srcid.read() ) ) {
                        m_served_word.rdata = 1;
                        m_state = TARGET_WRITE_RSP;
                        break;
                    }
                    m_served_word.rdata = 0;

                    m_atomic.accessDone( address );
                    if ((m_owner->*m_on_write_f)(i, offset, p_vci.wdata.read(), p_vci.be.read()))
                        m_state = TARGET_WRITE_RSP;
                    else {
                        if ( p_vci.eop.read() )
                            m_state = TARGET_IDLE;
                        else
                            m_state = TARGET_ERROR_RSP;
                        m_served_word.error = vci_param::ERR_GENERAL_DATA_ERROR;
                    }
                    break;

                case vci_param::CMD_LOCKED_READ:
                    assert(support_llsc && "Received a LL on a non-LL-supporting target");
                    m_atomic.doLoadLinked( address, p_vci.srcid.read() );
                case vci_param::CMD_READ:
                    if ((m_owner->*m_on_read_f)(i, offset, m_served_word.rdata)) {
                        m_state = TARGET_READ_RSP;
                    } else {
                        if ( p_vci.eop.read() )
                            m_state = TARGET_IDLE;
                        else
                            m_state = TARGET_ERROR_RSP;
                        m_served_word.error = vci_param::ERR_GENERAL_DATA_ERROR;
                        m_served_word.rdata = 0x0bad0bad;
                    }
                    break;
                }
            }

            if ( !reached ) {
                if ( default_target ) {
                    m_served_word.error = vci_param::ERR_GENERAL_DATA_ERROR;
                    if ( p_vci.eop.read() )
                        m_state = TARGET_IDLE;
                    else
                        m_state = TARGET_ERROR_RSP;
                } else {
                    assert(0&&"Receiving wrong req and not default target");
                }
            }

            break;
        }

        case TARGET_ERROR_RSP :
            if ( p_vci.eop.read() )
                m_state = TARGET_IDLE;
            m_served_word.error = vci_param::ERR_GENERAL_DATA_ERROR;
            break;
        }

        if ( p_vci.rspval.read() && p_vci.rspack.read() )
            m_rsp_info.put_and_get( m_served_word );
        else
            m_rsp_info.simple_put( m_served_word );
    } else if ( p_vci.rspval.read() && p_vci.rspack.read() )
        m_rsp_info.simple_get();
}

tmpl(void)::genMoore()
{
	const rsp_info_t &rsp_info = m_rsp_info.read();

	// Only accept queries if the cell in ring buffer is free
	p_vci.cmdack = m_rsp_info.wok();

	// Put rsp whatever ring buffer says
	p_vci.rspSetIds( rsp_info.srcid, rsp_info.trdid, rsp_info.pktid );
	p_vci.reop = rsp_info.eop;
	p_vci.rspval = m_rsp_info.rok();
	p_vci.rdata = rsp_info.rdata;
	p_vci.rerror = rsp_info.error;

#if 0
    if ( m_rsp_info.rok() )
        std::cout
        << static_cast<soclib::BaseModule*>(m_owner)->name() << std::hex << std::endl
        << " srcid: " << (unsigned int)rsp_info.srcid << std::endl
        << " trdid: " << (unsigned int)rsp_info.trdid << std::endl
        << " pktid: " << (unsigned int)rsp_info.pktid << std::endl
        << " rdata: " << (unsigned int)rsp_info.rdata << std::endl
        << " eop  : " << (unsigned int)rsp_info.eop   << std::endl
        << " error: " << (unsigned int)rsp_info.error << std::endl
        << std::endl;
#endif
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

