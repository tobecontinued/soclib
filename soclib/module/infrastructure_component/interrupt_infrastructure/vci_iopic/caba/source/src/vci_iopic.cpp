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
 * Copyright (c) UPMC, Lip6
 *         Alain Greiner <alain.greiner@lip6.fr> 2014
 */

#include <strings.h>
#include "../../../include/soclib/iopic.h"
#include "../include/vci_iopic.h"
#include "alloc_elems.h"

namespace soclib {
namespace caba {

#define tmpl(...) template<typename vci_param> __VA_ARGS__ VciIopic<vci_param>

/////////////////////////////////
tmpl(void)::transition()
/////////////////////////////////
{
    if ( p_resetn.read() == 0 ) 
    {
        r_ini_fsm = I_IDLE;
        r_tgt_fsm = T_IDLE;

        for ( size_t channel = 0 ; channel < m_channels ; channel++ )
        {
            r_hwi[channel]       = false;
            r_counter[channel]   = 0;
            r_error[channel]     = false;
            r_address[channel]   = 0;
            r_extend[channel]    = 0;
        }
        return;
    }

    // This is done at all cycles for all channels
    for ( size_t k = 0 ; k < m_channels ; k++ )
    {
        // update r_hwi[k] flip-flops 
        r_hwi[k] = p_hwi[k].read();

        // decrement r_conter[k] if pending HWI
        if ( r_hwi[k].read() and (r_counter[k].read() > 0) ) 
            r_counter[k] = r_counter[k].read() - 1;
            
    }

    // VCI initiator FSM
    switch ( r_ini_fsm.read() ) {
    case I_IDLE:
    {
        for( size_t k = 0 ; k < m_channels ; k++ )
        {
            // WTI transaction if 
            // - the channel k is not in error state
            // - a rising edge is detected on p_hwi[k] port,
            // - or there is a pending r_hwi[k] and period elapsed
            if ( not r_error[k].read() and
                 ( (p_hwi[k].read() and not r_hwi[k].read()) or
                   (r_hwi[k].read() and (r_counter[k].read() == 0)) ) ) 
            {
                r_channel = k;
                r_ini_fsm = I_SEND_CMD;
                break;
            }
        }
        break;
    }
    case I_SEND_CMD:
    {
        if ( p_vci_initiator.cmdack.read() )
        {
            r_counter[r_channel.read()] = m_period;
            r_ini_fsm = I_WAIT_RSP;
        }
        break;
    }
    case I_WAIT_RSP:
    {
        if ( p_vci_initiator.rspval.read() ) 
        {
            if ( p_vci_initiator.rerror.read() & 0x1 ) r_error[r_channel.read()] = true;
            r_ini_fsm = I_IDLE;
        }
        break;
    } }  // end switch r_ini_fsm

    // VCI target FSM
    switch ( r_tgt_fsm.read() ) {
    case T_IDLE:            
    {
        if ( p_vci_target.cmdval.read() )
        {
            r_srcid = p_vci_target.srcid.read();
            r_trdid = p_vci_target.trdid.read();
            r_pktid = p_vci_target.pktid.read();

            uint64_t address = (uint64_t)p_vci_target.address.read();
            size_t   cell    = (size_t)((address & 0xfff)>>2);
            size_t   reg     = cell % IOPIC_SPAN;
            size_t   channel = cell / IOPIC_SPAN;

            bool     seg_error = true;
            std::list<soclib::common::Segment>::iterator seg;
            for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ )
            {
                if ( seg->contains( address ) and p_vci_target.eop.read() )
                {
                    seg_error   = false;
                    break;
                }
            }

            if      ( not seg_error and
                      (p_vci_target.cmd.read() == vci_param::CMD_WRITE) and
                      (reg == IOPIC_ADDRESS) and
                      (channel < m_channels) )        // write address
            {
                r_address[channel] = p_vci_target.wdata.read();
                r_tgt_fsm          = T_WRITE;
            }
            else if ( not seg_error and
                      (p_vci_target.cmd.read() == vci_param::CMD_READ) and
                      (reg == IOPIC_ADDRESS) and
                      (channel < m_channels) )        // read address
            {
                r_rdata            = r_address[channel].read();
                r_tgt_fsm          = T_READ;
            }
            else if ( not seg_error and
                      (p_vci_target.cmd.read() == vci_param::CMD_WRITE) and
                      (reg == IOPIC_EXTEND) and
                      (channel < m_channels) )        // write extend
            {
                r_extend[channel] = p_vci_target.wdata.read();
                r_tgt_fsm         = T_WRITE;
            }
            else if ( not seg_error and
                      (p_vci_target.cmd.read() == vci_param::CMD_READ) and
                      (reg == IOPIC_EXTEND) and
                      (channel < m_channels) )        // read extend
            {
                r_rdata           = r_extend[channel].read();
                r_tgt_fsm         = T_READ;
            }
            else if ( not seg_error and
                      (p_vci_target.cmd.read() == vci_param::CMD_READ) and
                      (reg == IOPIC_STATUS) and
                      (channel < m_channels) )        // read status
            {
                r_rdata          = (uint32_t)r_hwi[channel] | 
                                   (((uint32_t)r_error[channel])<<1);
                r_error[channel] = false;
                r_tgt_fsm        = T_READ;
            }
            else if ( p_vci_target.eop.read() )
            {
                r_tgt_fsm  = T_ERROR;
            }
        }
        break;
    }
    case T_WRITE:
    case T_READ:
    case T_ERROR:
    {
        if( p_vci_target.rspack.read() ) r_tgt_fsm = T_IDLE;
        break;
    } }  // end switch r_tgt_fsm
}  // end transition

////////////////////////////
tmpl(void)::genMoore()
////////////////////////////
{
    //////// VCI target port
    if ( r_tgt_fsm.read() == T_IDLE )
    {
        p_vci_target.cmdack = true;
        p_vci_target.rspval = false;
    }
    else if ( r_tgt_fsm.read() == T_READ )
    {           
        p_vci_target.cmdack = false;
        p_vci_target.rspval = true;
        p_vci_target.rdata  = r_rdata.read();
        p_vci_target.rsrcid = r_srcid.read();
        p_vci_target.rtrdid = r_trdid.read();
        p_vci_target.rpktid = r_pktid.read();
        p_vci_target.rerror = vci_param::ERR_NORMAL;
        p_vci_target.reop   = true;
    }
    else if ( r_tgt_fsm.read() == T_WRITE )
    {
        p_vci_target.cmdack = false;
        p_vci_target.rspval = true; 
        p_vci_target.rdata  = 0;
        p_vci_target.rsrcid = r_srcid.read();
        p_vci_target.rtrdid = r_trdid.read();
        p_vci_target.rpktid = r_pktid.read();
        p_vci_target.rerror = vci_param::ERR_NORMAL;
        p_vci_target.reop   = true;
    }
    else   // T_ERROR:
    {
        p_vci_target.cmdack = false;
        p_vci_target.rspval = true; 
        p_vci_target.rdata  = 0;
        p_vci_target.rsrcid = r_srcid.read();
        p_vci_target.rtrdid = r_trdid.read();
        p_vci_target.rpktid = r_pktid.read();
        p_vci_target.rerror = vci_param::ERR_GENERAL_DATA_ERROR;
        p_vci_target.reop   = true;
    }

    //////// VCI initiator port
    if ( r_ini_fsm.read() == I_IDLE )
    {
        p_vci_initiator.cmdval  = false;
        p_vci_initiator.rspack  = false;
    }
    else if ( r_ini_fsm.read() == I_SEND_CMD )
    {
        p_vci_initiator.cmdval  = true;
        p_vci_initiator.address = ((addr_t)r_extend[r_channel.read()].read()<<32) +
                                  ((addr_t)r_address[r_channel.read()].read());
        p_vci_initiator.cmd     = vci_param::CMD_WRITE;
        p_vci_initiator.wdata   = (data_t)r_channel.read();
        p_vci_initiator.be      = 0xF;
        p_vci_initiator.plen    = 4;
        p_vci_initiator.srcid   = m_srcid;
        p_vci_initiator.trdid   = 0;
        p_vci_initiator.pktid   = 0;
        p_vci_initiator.cons    = false;
        p_vci_initiator.contig  = true;
        p_vci_initiator.wrap    = false;
        p_vci_initiator.cfixed  = false;
        p_vci_initiator.clen    = 0;
        p_vci_initiator.eop     = true;
        p_vci_initiator.rspack  = false;
    }
    else   // I_WAIT_RSP state
    {
        p_vci_initiator.cmdval  = false;
        p_vci_initiator.rspack  = true;
    }
}  // end genMoore()

///////////////////////////
tmpl(void)::print_trace()
{

    const char* tgt_fsm_str[] =
    {
        "TGT_IDLE",
        "TGT_WRITE",
        "TGT_READ",
        "TGT_ERROR",
    };

    const char* ini_fsm_str[] =
    {
        "INI_IDLE",
        "INI_SEND_CMD",
        "INI_WAIT_RSP",
    };

    std::cout << "IOPIC " << name() 
              << " : " << tgt_fsm_str[r_tgt_fsm.read()] 
              << " / " << ini_fsm_str[r_ini_fsm.read()] << std::endl;

    for (unsigned i = 0; i < m_channels ; i++)
    {
        if ( r_hwi[i].read() ) 
        std::cout << "  - HWI = " << std::dec << i
                  << " / address = " << std::hex << r_address[i].read()
                  << " / extend  = " << r_extend[i].read() 
                  << " / count = " << r_counter[i].read() << std::endl;
    }
}

///////////////////////////////////////////////////////////
tmpl()::VciIopic( sc_core::sc_module_name             name,
                  const soclib::common::MappingTable  &mt,
                  const soclib::common::IntTab        &srcid,
                  const soclib::common::IntTab        &tgtid,
                  const size_t                        channels,
                  const size_t                        period )
       : caba::BaseModule(name),
       m_srcid( mt.indexForId(srcid) ),
       m_channels( channels ),
       m_seglist( mt.getSegmentList(tgtid) ),
       m_period( period ),
       p_clk("clk"),
       p_resetn("resetn"),
       p_vci_initiator("p_vci_initiator"),
       p_vci_target("p_vci_target")
{
    std::cout << "  - Building VciIopic " << name << std::endl;

    r_hwi     = soclib::common::alloc_elems<sc_signal<bool> >("r_hwi", channels);
    r_counter = soclib::common::alloc_elems<sc_signal<uint32_t> >("r_counter", channels);
    r_address = soclib::common::alloc_elems<sc_signal<uint32_t> >("r_address", channels);
    r_extend  = soclib::common::alloc_elems<sc_signal<uint32_t> >("r_extend", channels);
    r_error   = soclib::common::alloc_elems<sc_signal<bool> >("r_error", channels);

    p_hwi     = soclib::common::alloc_elems<sc_in<bool> >("p_hwi", channels);

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();

    size_t nbsegs = 0;
    std::list<soclib::common::Segment>::iterator seg;
    for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ ) 
    {
        nbsegs++;
        
	    if ( (seg->baseAddress() & 0x00000FFF) != 0 ) 
	    {
		    std::cout << "Error in component VciIopic : " << name 
		              << "The base address of segment " << seg->name()
                      << " must be multiple of 4 Kbytes" << std::endl;
		    exit(1);
	    }
	    if ( seg->size() < 4096 ) 
	    {
		    std::cout << "Error in component VciIopic : " << name 
	                  << "The size of segment " << seg->name()
                      << " cannot be smaller than 4 K bytes" << std::endl;
		    exit(1);
	    }
        std::cout << "    => segment " << seg->name()
                  << " / base = " << std::hex << seg->baseAddress()
                  << " / size = " << seg->size() << std::endl; 
    }

    if( nbsegs == 0 )
    {
		std::cout << "Error in component VciIopic : " << name
		          << " No segment allocated" << std::endl;
		exit(1);
    }
} // end constructor

///////////////////
tmpl()::~VciIopic()
{
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

