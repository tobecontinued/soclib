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
#ifdef SOCLIB_MODULE_DEBUG
# include "vci_buffers.h"
#endif

namespace soclib {
namespace caba {

#define tmpl(x) template<typename vci_param,bool default_target,bool support_llsc> \
    x VciTargetFsm<vci_param,default_target,support_llsc>

tmpl(/**/)::VciTargetFsm(
    soclib::caba::VciTarget<vci_param> &vci,
    const std::list<soclib::common::Segment> &seglist )
    : p_vci(vci),
      m_atomic(support_llsc ? (1<<vci_param::S) : 0),
      m_segments(seglist.begin(), seglist.end())
{
}

tmpl(/**/)::VciTargetFsm(
    soclib::caba::VciTarget<vci_param> &vci,
    const std::list<soclib::common::Segment> &seglist,
    size_t fifo_depth )
    : p_vci(vci),
      m_atomic(support_llsc ? (1<<vci_param::S) : 0),
      m_segments(seglist.begin(), seglist.end())
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
    m_atomic.clearAll();
    m_mode = MODE_IDLE;
    m_send_rsp = false;
}

tmpl(void)::handle_one()
{
    /*
     * This variable tracks whether at least one segment was
     * reached, if we are not default target, this variable
     * will be optimized out by compiler
     */
    bool reached = false;
    std::vector<soclib::common::Segment>::const_iterator seg = m_segments.begin();
    size_t seg_no=0;
    addr_t address = m_current_cmd.addr;
    addr_t offset = 0;
    while ( seg != m_segments.end() && !reached ) {
        if ( seg->contains(address) ) {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " segment " << *seg << " contains " << address << std::endl;
#endif
            reached = true;
            offset = address - seg->baseAddress();
        } else {
            ++seg_no;
            ++seg;
        }
    }

    if ( !reached ) {
        if ( default_target ) {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " ERROR: address " << address << " not for me" << std::endl;
#endif
            m_current_cmd.error = vci_param::ERR_GENERAL_DATA_ERROR;
        } else {
            assert(0&&"Receiving wrong req and not default target");
        }
        return;
    }

#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << name()
        << " Handling command " << m_current_cmd.cmd
        << " @ " << m_current_cmd.addr
        << " off " << offset << " of " << *seg
        << " be: " << m_current_cmd.be
        << " wdata: " << m_current_cmd.wdata
        << std::endl;
#endif
    switch (m_current_cmd.cmd)
    {
    case vci_param::CMD_WRITE:
        if ( support_llsc )
            m_atomic.accessDone( address );
        m_current_cmd.rdata = 0;
        if ( ! (m_owner->*m_on_write_f)(seg_no, offset, m_current_cmd.wdata, m_current_cmd.be)) {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " ERROR: write error" << std::endl;
#endif
            m_current_cmd.error = vci_param::ERR_GENERAL_DATA_ERROR;
        } else {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " write ok" << std::endl;
#endif
        }
        break;

    case vci_param::CMD_STORE_COND:
        assert(support_llsc && "Received a SC on a non-SC-supporting target");
        if ( ! m_atomic.isAtomic( address, m_current_cmd.srcid ) ) {
            m_current_cmd.rdata = 1;
            break;
        }
        m_current_cmd.rdata = 0;

        m_atomic.accessDone( address );
        if ( ! (m_owner->*m_on_write_f)(seg_no, offset, m_current_cmd.wdata, m_current_cmd.be)) {
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " ERROR: write error" << std::endl;
#endif
            m_current_cmd.error = vci_param::ERR_GENERAL_DATA_ERROR;
        } else {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " SC ok" << std::endl;
#endif
        }
        break;

    case vci_param::CMD_LOCKED_READ:
        assert(support_llsc && "Received a LL on a non-LL-supporting target");
        m_atomic.doLoadLinked( address, m_current_cmd.srcid );
    case vci_param::CMD_READ:
        if ( ! (m_owner->*m_on_read_f)(seg_no, offset, m_current_cmd.rdata)) {
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " ERROR: read error" << std::endl;
#endif
            m_current_cmd.error = vci_param::ERR_GENERAL_DATA_ERROR;
            m_current_cmd.rdata = 0x0bad0bad;
        } else {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " read ok, rdata: " << m_current_cmd.rdata << std::endl;
#endif
        }
        break;
    }
}

tmpl(void)::transition()
{
    if ( p_vci.peerAccepted() ) {
#ifdef SOCLIB_MODULE_DEBUG
        std::cout
            << name() << " peer accepted response" << std::endl
            << " reop: " << p_vci.reop.read() << std::endl
            << " rdata: " << p_vci.rdata.read() << std::endl
            << " rerror: " << p_vci.rerror.read() << std::endl
            << " rsrcid: " << p_vci.rsrcid.read() << std::endl
            << " rtrdid: " << p_vci.rtrdid.read() << std::endl
            << " rpktid: " << p_vci.rpktid.read() << std::endl
            << std::endl;
#endif
        m_send_rsp = false;
    }

#ifdef SOCLIB_MODULE_DEBUG
    static const char *const modes[] = {
        "IDLE", "INOUT_QUERY", "SIZED_READ",
        "SIZED_WRITE", "SIZED_READ_FLUSH_CMD", "FLUSH_CMD" };
    std::cout
        << name()
        << " send_rsp: " << m_send_rsp
        << " " << modes[m_mode]
        << std::endl;
    if ( p_vci.iAccepted() ) {
        VciCmdBuffer<vci_param> buf;
        buf.readFrom( p_vci );
        std::cout
            << name()
            << " read command: " << buf
            << std::endl;
    }
#endif

    switch ( m_mode ) {
    case MODE_IDLE:
        if ( p_vci.iAccepted() ) {
            m_current_cmd.addr = p_vci.address.read();
            m_current_cmd.cmd = p_vci.cmd.read();
            m_current_cmd.be = p_vci.be.read();
            m_current_cmd.srcid = p_vci.srcid.read();
            m_current_cmd.trdid = p_vci.trdid.read();
            m_current_cmd.pktid = p_vci.pktid.read();
            m_current_cmd.wdata = p_vci.wdata.read();
            m_current_cmd.eop = p_vci.eop.read();
            m_current_cmd.plen = p_vci.plen.read();

            if ( m_current_cmd.plen ) {
                switch ( m_current_cmd.cmd ) {
                case vci_param::CMD_READ:
                    if ( m_current_cmd.eop )
                        m_mode = MODE_SIZED_READ;
                    else {
                        m_mode = MODE_SIZED_READ_FLUSH_CMD;
                        m_current_cmd.base_addr = m_current_cmd.addr;
                        m_cmd_word = 0;
                    }
                    handle_one();
                    m_send_rsp = true;
                    m_cells_to_go = (m_current_cmd.plen+vci_param::B-1)/vci_param::B;
                    break;
                case vci_param::CMD_WRITE:
                    if ( m_current_cmd.eop ) {
                        m_mode = MODE_INOUT_QUERY;
                        handle_one();
                        m_send_rsp = true;
                        assert( m_current_cmd.plen <= vci_param::B );
                    } else {
                        m_mode = MODE_SIZED_WRITE;
                        handle_one();
                        m_cells_to_go = (m_current_cmd.plen+vci_param::B-1)/vci_param::B;
                    }
                    break;
                default:
                    m_mode = MODE_INOUT_QUERY;
                    handle_one();
                    m_send_rsp = true;
                    m_cells_to_go = 1;
                    assert( m_current_cmd.eop && "LL-SC command with no EOP ? multi-word LL/SC ???");
                }
            } else {
                    m_mode = MODE_INOUT_QUERY;
                    handle_one();
                    m_send_rsp = true;
                    m_cells_to_go = 1;
            }
        }
        break;
    case MODE_INOUT_QUERY:
        if ( m_send_rsp )
            break;

        m_mode = MODE_IDLE;
        break;

    case MODE_SIZED_WRITE:
        if ( p_vci.iAccepted() ) {
            m_cells_to_go--;
            m_current_cmd.addr += vci_param::B;
            assert( m_current_cmd.addr == p_vci.address.read() );
            m_current_cmd.wdata = p_vci.wdata.read();
            m_current_cmd.be = p_vci.be.read();
            m_current_cmd.eop = p_vci.eop.read();
            handle_one();

            if ( m_current_cmd.eop ) {
                m_mode = MODE_IDLE;
                assert( m_cells_to_go == 1 );
                m_send_rsp = true;
            }
        }
        break;

    case MODE_SIZED_READ_FLUSH_CMD:
        if ( ! p_vci.iAccepted() )
            break;
        m_cmd_word++;
        assert(m_cmd_word*vci_param::B + m_current_cmd.base_addr == p_vci.address.read());
        if ( p_vci.eop.read() )
            m_mode = MODE_SIZED_READ;
    case MODE_SIZED_READ:
        if ( m_send_rsp )
            break;

        m_cells_to_go--;
        m_current_cmd.addr += vci_param::B;

        m_current_cmd.eop = m_cells_to_go == 1;
        if ( m_cells_to_go ) {
            handle_one();
            m_send_rsp = true;
        } else {
            if ( m_mode == MODE_SIZED_READ_FLUSH_CMD )
                m_mode = MODE_FLUSH_CMD;
            else
                m_mode = MODE_IDLE;
        }
        break;

    case MODE_FLUSH_CMD:
        if ( ! p_vci.iAccepted() )
            break;
        if ( p_vci.eop.read() )
            m_mode = MODE_IDLE;
        break;
    }
}

tmpl(void)::genMoore()
{
    switch ( m_mode ) {
    case MODE_IDLE:
    case MODE_SIZED_READ_FLUSH_CMD:
    case MODE_FLUSH_CMD:
        p_vci.cmdack = true;
        break;
    case MODE_INOUT_QUERY:
    case MODE_SIZED_READ:
        p_vci.cmdack = false;
        break;
    case MODE_SIZED_WRITE:
        p_vci.cmdack = ! m_current_cmd.eop;
        break;
    }

	p_vci.rspval = m_send_rsp;
    p_vci.rspSetIds( m_current_cmd.srcid, m_current_cmd.trdid, m_current_cmd.pktid );
    p_vci.reop = m_current_cmd.eop;
    p_vci.rdata = m_current_cmd.rdata;
    p_vci.rerror = m_current_cmd.error;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

