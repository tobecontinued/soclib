/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "caba/util/vci_target_fsm.h"
#include "common/register.h"
#include "common/base_module.h"

namespace soclib {
namespace caba {

#define tmpl(x) template<typename vci_param,bool default_target,size_t fifo_depth>\
x VciTargetFsm<vci_param,default_target,fifo_depth>

tmpl(/**/)::VciTargetFsm(
    soclib::caba::VciTarget<vci_param> &vci,
    const std::list<soclib::common::Segment> &seglist )
    : p_vci(vci),
      m_segments(seglist.begin(), seglist.end()),
      m_rsp_info("m_rsp_info")
{
}

tmpl(void)::_on_read_write(
    soclib::caba::BaseModule *owner_module,
    wrapper_read_t *read_func,
    wrapper_write_t *write_func )
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

    m_rsp_info.init();
}

tmpl(void)::transition()
{
	rsp_info_t rsp_info;

	if ( p_vci.cmdval.read() &&
         p_vci.cmdack.read() )
    {
        switch(m_state)
        {
        case TARGET_IDLE:
        case TARGET_WRITE_RSP:
        case TARGET_READ_RSP:
        {
            addr_t address = p_vci.address.read();
            data_t rdata = 0;
            /*
             * This variable tracks whether at least one segment was
             * reached, if we are not default target, this variable
             * will be optimized out by compiler
             */
            bool reached = false;

            rsp_info.error = false;

            std::vector<soclib::common::Segment>::const_iterator seg;
            size_t i=0;
            for ( seg = m_segments.begin();
                  seg < m_segments.end() && !reached;
                  ++i, ++seg ) {
                if ( ! seg->contains(address) )
                    continue;

                reached = true;
                address -= seg->baseAddress();

                switch (p_vci.cmd.read())
                {
                case VCI_CMD_WRITE:
                    rsp_info.rdata = 0;
                    if (m_on_write_f(m_owner, i, address, p_vci.wdata.read(), p_vci.be.read()))
                        m_state = TARGET_WRITE_RSP;
                    else {
                        m_state = TARGET_ERROR_RSP;
                        rsp_info.error = true;
                    }
                    break;

                case VCI_CMD_READ:
                    if (m_on_read_f(m_owner, i, address, rdata)) {
                        m_state = TARGET_READ_RSP;
                        rsp_info.rdata = rdata;
                    } else {
                        m_state = TARGET_ERROR_RSP;
                        rsp_info.error = true;
                        rsp_info.rdata = 0x0bad0bad;
                    }
                    break;
                }
            }

            if ( default_target && !reached )
                m_state = TARGET_ERROR_RSP;

            break;
        }

        case TARGET_ERROR_RSP :
            if ( p_vci.eop.read() )
                m_state = TARGET_IDLE;
            rsp_info.error = true;
            break;
        }

        rsp_info.srcid = p_vci.srcid.read();
        rsp_info.trdid = p_vci.trdid.read();
        rsp_info.pktid = p_vci.pktid.read();
        rsp_info.eop = p_vci.eop.read();

        if ( p_vci.rspval.read() && p_vci.rspack.read() )
            m_rsp_info.put_and_get( rsp_info );
        else
            m_rsp_info.simple_put( rsp_info );
    } else
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

tmpl(void)::trace(
    sc_trace_file &tf,
    const std::string base_name,
    unsigned int what)
{
#if 0
    if (what == SOCLIB_TRACE_INTERFACE) {
        SOCLIB_ADD_TRACE(tf, base_name, m_state);
    }
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

