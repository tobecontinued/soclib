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
 *         Alain Greiner <alain.greiner@lip6.fr> November 2010
 */

///////////////////////////////////////////////////////////////////////////////
//  This component is a multi-channels DMA controller.
//  The number of channels and the burst length are constructor parameters.
//  The max_burst_length parameter must be a multiple of 4 bytes.
//  The number of channels (simultaneous transfers) cannot be larger than 8.
//  This component makes the assumption that the VCI RDATA & WDATA fields
//  have 32 bits: The source buffer address, the destination buffer address, 
//  and the memory buffer length must be multiple of 4 bytes.
//  The memory buffer length is not constrained to be a multiple of the
//  max_burst_length.
//
//  The general arbitration policy between the active channels is round-robin.
//  The choice of the channel, the base adress of the source and destination
//  buffers, the length of the transfer, are controled by the software.
//  There is one private IRQ line for each channel.
//
//  The aligned segment size associated to this component is 256 bytes,
//  and only 10 address bits are decoded :
//  - The 5 LSB bits define the target register (see dma.h)
//  - The 3 MSB bits define the selected channel.
//////////////////////////////////////////////////////////////////////////////
//  Implementation note:
//  This component contains three FSMs :
//  - the tgt_fsm controls the configuration commands and responses 
//    on the VCI target ports.
//  - the cmd_fsm controls the read and write data transfer commands 
//    on the VCI initiator port.
//    It uses four registers : r_cmd fsm (state), r_cmd_count
//    (counter of bytes in a burst), r_cmd_index (selected channel),
//    r_cmd_length (actual length of a burst).
//  - the rsp_fsm controls the read and write data transfer responses 
//    on the VCI initiator port.
//    It uses four registers : r_rsp fsm (state), r_rsp_count
//    (counter of bytes in a burst), r_rsp_index (selected channel),
//    r_rsp_length (actual length of a burst).
//  Each channel [k] has a set of "state" registers: 
//  - r_activate[k]	channel actived (a transfer has been requested)
//  - r_src_addr[k]	address of the source memory buffer
//  - r_dst_addr[k]	address of the destination memory buffer
//  - r_length[k]	total length of the memory buffer (bytes)
//  - r_read[k]		status of the burst transfer (read or write)
//  - r_irq[k]		IRQ status
//  - r_buf[k][word]	local burst buffer 
/////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <cassert>

#include "alloc_elems.h"
#include "../include/vci_multi_dma.h"

namespace soclib { namespace caba {

#define tmpl(t) template<typename vci_param> t VciMultiDma<vci_param>

/////////////////////////
tmpl(void)::transition()
{
    if (!p_resetn) 
    {
        r_tgt_fsm    = TGT_IDLE;
        r_cmd_fsm    = CMD_IDLE;
        r_rsp_fsm    = RSP_IDLE;
        r_cmd_index  = 0;
        r_rsp_index  = 0;
        for ( size_t k = 0 ; k < m_channels ; k++ )
        {
            r_channel_fsm[k] 	= CHANNEL_IDLE;
            r_activate[k]	= false;
            r_done[k] 		= false;
            r_error[k] 		= false;
        }
        return;
    }

    ///////////////////////////////////////////////////////////////////////
    // This TGT_FSM controls the VCI TARGET port
    // and the following registers:
    // - r_tgt_fsm
    // - r_src_addr[k] (when the channel is not active)
    // - r_dst_addr[k] (when the channel is not active)
    // - r_length[k]   (when the channel is not active)
    // - r_read[k]     (when the channel is not active)
    // - r_activate[k]  
    //////////////////////////////////////////////////////////////////////
    switch(r_tgt_fsm.read()) 
    {
        case TGT_IDLE:
        {
            if (p_vci_target.cmdval.read() )
            {
                typename vci_param::addr_t	address = p_vci_target.address.read();
                typename vci_param::data_t	wdata   = p_vci_target.wdata.read();
                typename vci_param::cmd_t	cmd     = p_vci_target.cmd.read();
               
                r_srcid					= p_vci_target.srcid.read();
                r_trdid					= p_vci_target.trdid.read();
                r_pktid					= p_vci_target.pktid.read();
                
                int 				cell    = (int)((address & 0x1C) >> 2);
                int				channel = (int)((address & 0xE0) >> 5);

                assert( p_vci_target.eop.read() &&
                "VCI_MULTI_DMA error : A configuration or status request mut be one single VCI flit");

	        if ( (cell == DMA_SRC) && (cmd == vci_param::CMD_WRITE) && (wdata%4 == 0) )
                {
                    assert( !r_activate[channel] &&
                    "VCI_MULTI_DMA error : Configuration request received for an active channel");
                    r_src_addr[channel] = wdata;
                    r_tgt_fsm = TGT_WRITE;
                }
                else if ( (cell == DMA_SRC) && (cmd == vci_param::CMD_READ) )
                {
                    r_rdata   = r_src_addr[channel].read();
                    r_tgt_fsm = TGT_READ;
                }
                else if ( (cell == DMA_DST) && (cmd == vci_param::CMD_WRITE) && (wdata%4 == 0) )
                {
                    assert( !r_activate[channel] &&
                    "VCI_MULTI_DMA error : Configuration request received for an active channel");
                    r_dst_addr[channel] = wdata;
                    r_tgt_fsm = TGT_WRITE;
                }
                else if ( (cell == DMA_DST) && (cmd == vci_param::CMD_READ) )
                {
                    r_rdata   = r_dst_addr[channel].read();
                    r_tgt_fsm = TGT_READ;
                }
                else if ( (cell == DMA_LEN) && (cmd == vci_param::CMD_WRITE) && (wdata%4 == 0) )
                {
                    assert( !r_activate[channel] &&
                    "VCI_MULTI_DMA error : Configuration request received for an active channel");
                    r_length[channel] = wdata;
                    r_activate[channel] = true;
                    r_tgt_fsm = TGT_WRITE;
                }
                else if ( (cell == DMA_LEN) && (cmd == vci_param::CMD_READ) )
                {
                    r_rdata   = r_length[channel].read();
                    r_tgt_fsm = TGT_READ;
                }
                else if ( (cell == DMA_RESET) && (cmd == vci_param::CMD_WRITE) )
                {
                    r_activate[channel] = false;
                    r_tgt_fsm = TGT_WRITE;
                }
                else if ( (cell == DMA_IRQ_DISABLED) && (cmd == vci_param::CMD_WRITE) )
                {
                   // No action : this is just for compatibility with previous vci_dma 
                    r_tgt_fsm = TGT_WRITE;
                }
                else
                {
                    r_tgt_fsm = TGT_ERROR;
                }
            }
            break;
        }
        case TGT_WRITE:
        case TGT_READ:
        case TGT_ERROR:
        {
            if ( p_vci_target.rspack.read() )
            {
                r_tgt_fsm = TGT_IDLE;
            }
            break;
        }
    } // end switch tgt_fsm

    //////////////////////////////////////////////////////////
    // These CHANNEL_FSM define the transfer state for each
    // channel and control the following registers
    // - r_channel_fsm[k]
    // - r_done[k] reset
    // - r_error[k] reset
    //////////////////////////////////////////////////////////
    for ( size_t k=0 ; k<m_channels ; k++ )
    {
        switch( r_channel_fsm[k].read() )
        {
            case CHANNEL_IDLE:
            {
                if ( r_activate[k] ) r_channel_fsm[k] = CHANNEL_READ_REQ;
                break;
            }
            case CHANNEL_READ_REQ:	// requesting a VCI READ transaction
            {
                if ( (r_cmd_fsm == CMD_READ) && (r_cmd_index.read() == k) ) 
                    r_channel_fsm[k] = CHANNEL_READ_WAIT;
                break;
            }
            case CHANNEL_READ_WAIT: 	// waiting  VCI READ response
            {
                if ( r_done[k] ) 
                {
                    if ( r_error[k] )   r_channel_fsm[k] = CHANNEL_ERROR;
                    else		r_channel_fsm[k] = CHANNEL_WRITE_REQ;
                    r_done[k] = false;
                }
                break;
            }
            case CHANNEL_WRITE_REQ:	// requesting a VCI WRITE transaction
            {
                if ( (r_cmd_fsm == CMD_WRITE) && (r_cmd_index.read() == k) ) 
                    r_channel_fsm[k] = CHANNEL_WRITE_WAIT;
                break;
            }
            case CHANNEL_WRITE_WAIT:	// waiting VCI WRITE response
            {
                if ( r_done[k] ) 
                {
                    if ( r_error[k] )  			r_channel_fsm[k] = CHANNEL_ERROR;
                    else if ( r_length[k].read() == 0 ) r_channel_fsm[k] = CHANNEL_DONE;
                    else 				r_channel_fsm[k] = CHANNEL_READ_REQ;
                    r_done[k] = false;
                }
                break;
            }
            case CHANNEL_DONE:
            case CHANNEL_ERROR:
            {
                if ( !r_activate[k] )	r_channel_fsm[k] = CHANNEL_IDLE;
                break; 
            }
        } // end switch r_channel_fsm[k]
    }
                
    //////////////////////////////////////////////////////////
    // This CMD_FSM controls the VCI INIT command port
    // and the following registers
    // - r_cmd_fsm
    // - r_cmd_index
    // - r_cmd_count
    // - r_cmd_length
    // - r_src_addr[k] (increment when the channel is activate)
    // - r_dst_addr[k] (increment when the channel is activate)
    //////////////////////////////////////////////////////////
    switch(r_cmd_fsm.read()) 
    {
        case CMD_IDLE:
        {
            // round-robin arbitration between channels to send a command
            bool not_found = true;
            for( size_t n = 0 ; (n < m_channels) && not_found ; n++ )
            {
                size_t k = (r_cmd_index.read() + n) % m_channels;
                if ( (r_channel_fsm[k] == CHANNEL_READ_REQ) || (r_channel_fsm[k] == CHANNEL_WRITE_REQ) )
                {
                    not_found        = false;
                    r_cmd_index      = k;
                    r_cmd_count = 0;
                    if ( r_length[k].read() < m_burst_max_length ) r_cmd_length = r_length[k].read();
                    else                                           r_cmd_length = m_burst_max_length;
                    if ( r_channel_fsm[k] == CHANNEL_READ_REQ )    r_cmd_fsm    = CMD_READ; 	
                    else                                           r_cmd_fsm    = CMD_WRITE;
                }
            }
            break;
        }
        case CMD_READ:
        {
            if ( p_vci_initiator.cmdack.read() )
            {
                size_t k = r_cmd_index.read();
                r_cmd_fsm = CMD_IDLE;
                r_src_addr[k] = r_src_addr[k].read() + r_cmd_length.read();
            }
            break;
        }
        case CMD_WRITE:
        {
            if ( p_vci_initiator.cmdack.read() )
            {
                if ( r_cmd_count.read() == r_cmd_length.read() - 4 )
                {
                    size_t k = r_cmd_index.read();
                    r_cmd_fsm = CMD_IDLE;
                    r_dst_addr[k] = r_dst_addr[k].read() + r_cmd_length.read();
                }
                r_cmd_count = r_cmd_count.read() + 4;
            }
            break;
        }
    } // end switch cmd_fsm

    ///////////////////////////////////////////////////////////
    // This RSP_FSM controls the VCI INIT response port
    // and the following registers:
    // - r_rsp_fsm
    // - r_rsp_count
    // - r_rsp_index
    // - r_rsp_length
    // - r_length[k]	(decrement when the channel is active)
    // - r_done[k] set
    // - r_error[k] set
    ///////////////////////////////////////////////////////////
    switch(r_rsp_fsm.read()) 
    {
        case RSP_IDLE:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                size_t k = (size_t)p_vci_initiator.rtrdid.read();
                r_rsp_count = 0;
                r_rsp_index = k;
		if ( r_channel_fsm[k].read() == CHANNEL_READ_WAIT) // read response expected
                {
                    r_rsp_fsm   = RSP_READ;
                }
                else if ( r_channel_fsm[k].read() == CHANNEL_WRITE_WAIT) // write response expected
                {
                    r_rsp_fsm   = RSP_WRITE;
                }
                else
                {
                    std::cout << "VCI_MULTI_DMA error : unexpected VCI response packed" << std::endl;
                    exit(0);
                }  
                if ( r_length[k].read() < m_burst_max_length ) r_rsp_length = r_length[k].read();
                else                                           r_rsp_length = m_burst_max_length;
            }
            break;
        }
        case RSP_READ:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                size_t k    = r_rsp_index.read();
                size_t word = r_rsp_count.read() / 4;

                r_buf[k][word] = p_vci_initiator.rdata.read(); 
                if ( p_vci_initiator.reop.read() )
                {
                    assert( (r_rsp_count.read() == r_rsp_length.read() - 4 ) &&
                    "VCI_MULTI_DMA error : the number of flits of a read response packet is wrong");
                    r_done[k] = true;
                    r_error[k] = (p_vci_initiator.rerror.read() != 0);
                    r_rsp_fsm = RSP_IDLE;
                } 
                r_rsp_count = r_rsp_count + 4;
            }
            break;
        } 
        case RSP_WRITE:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                 assert( (p_vci_initiator.reop.read() == true) &&
                 "VCI_MULTI_DMA error : a write response packed cannot contain more than one flit");  
                size_t k  = r_rsp_index.read();
                r_length[k]  = r_length[k].read() - r_rsp_length.read();
                r_done[k] = true;
                r_error[k] = (p_vci_initiator.rerror.read() != 0);
                r_rsp_fsm = RSP_IDLE;
            }
            break;
        } 
    } // end switch rsp_fsm
} // end transition

//////////////////////
tmpl(void)::genMoore()
{
    /////// VCI INIT CMD ports ////// 
    switch( r_cmd_fsm.read() ) {
        case CMD_IDLE:
        {
            p_vci_initiator.cmdval  = false;
            p_vci_initiator.address = 0;
            p_vci_initiator.wdata   = 0;
            p_vci_initiator.be      = 0;
            p_vci_initiator.plen    = 0;
            p_vci_initiator.cmd     = vci_param::CMD_WRITE;
            p_vci_initiator.trdid   = 0;
            p_vci_initiator.pktid   = 0;
            p_vci_initiator.srcid   = 0;
            p_vci_initiator.cons    = false;
            p_vci_initiator.wrap    = false;
            p_vci_initiator.contig  = false;
            p_vci_initiator.clen    = 0;
            p_vci_initiator.cfixed  = false;
            p_vci_initiator.eop     = false;
            break;
        }
        case CMD_READ:
        {
            size_t k    = r_cmd_index.read();
            p_vci_initiator.cmdval  = true;
            p_vci_initiator.address = r_src_addr[k].read();
            p_vci_initiator.wdata   = 0;
            p_vci_initiator.be      = 0xF;
            p_vci_initiator.plen    = r_cmd_length.read();
            p_vci_initiator.cmd     = vci_param::CMD_READ;
            p_vci_initiator.trdid   = k;
            p_vci_initiator.pktid   = 0;
            p_vci_initiator.srcid   = m_srcid;
            p_vci_initiator.cons    = false;
            p_vci_initiator.wrap    = false;
            p_vci_initiator.contig  = true;
            p_vci_initiator.clen    = 0;
            p_vci_initiator.cfixed  = false;
            p_vci_initiator.eop     = true;
            break;
        }
        case CMD_WRITE:
        {
            size_t k    = r_cmd_index.read();
            size_t n    = r_cmd_count.read() / 4;
            p_vci_initiator.cmdval  = true;
            p_vci_initiator.address = r_dst_addr[k].read() + r_cmd_count.read();
            p_vci_initiator.wdata   = r_buf[k][n].read();
            p_vci_initiator.be      = 0xF;
            p_vci_initiator.plen    = r_cmd_length.read();
            p_vci_initiator.cmd     = vci_param::CMD_WRITE;
            p_vci_initiator.trdid   = k;
            p_vci_initiator.pktid   = 0;
            p_vci_initiator.srcid   = m_srcid;
            p_vci_initiator.cons    = false;
            p_vci_initiator.wrap    = false;
            p_vci_initiator.contig  = true;
            p_vci_initiator.clen    = 0;
            p_vci_initiator.cfixed  = false;
            p_vci_initiator.eop     = ( r_cmd_count.read() == r_cmd_length.read() - 4 );
            break;
        }
    } // end switch cmd_fsm

    /////// VCI INIT RSP port ////// 
    if ( r_rsp_fsm.read() == RSP_IDLE )  p_vci_initiator.rspack = false;
    else                                 p_vci_initiator.rspack = true;

    ////// VCI TARGET ports /////// 
    switch( r_tgt_fsm.read() ) {
        case TGT_IDLE:
        {
            p_vci_target.cmdack = true;
            p_vci_target.rspval = false;
            break;
        }
        case TGT_WRITE:
        {
            p_vci_target.cmdack = false;
            p_vci_target.rspval = true;
            p_vci_target.rdata  = 0;
            p_vci_target.rerror = vci_param::ERR_NORMAL;
            p_vci_target.rsrcid = r_srcid.read();
            p_vci_target.rtrdid = r_trdid.read();
            p_vci_target.rpktid = r_pktid.read();
            p_vci_target.reop   = true;
            break;
        }
        case TGT_READ:
        {
            p_vci_target.cmdack = false;
            p_vci_target.rspval = true;
            p_vci_target.rdata  = r_rdata.read();
            p_vci_target.rerror = vci_param::ERR_NORMAL;
            p_vci_target.rsrcid = r_srcid.read();
            p_vci_target.rtrdid = r_trdid.read();
            p_vci_target.rpktid = r_pktid.read();
            p_vci_target.reop   = true;
            break;
        }
        case TGT_ERROR:
        {
            p_vci_target.cmdack = false;
            p_vci_target.rspval = true;
            p_vci_target.rdata  = 0;
            p_vci_target.rerror = vci_param::ERR_GENERAL_DATA_ERROR;
            p_vci_target.rsrcid = r_srcid.read();
            p_vci_target.rtrdid = r_trdid.read();
            p_vci_target.rpktid = r_pktid.read();
            p_vci_target.reop   = true;
            break;
        }
    } // end switch rsp_fsm

    /////// IRQ ports //////////
    for ( size_t k = 0 ; k < m_channels ; k++ )
    {
	p_irq[k] = (r_channel_fsm[k] == CHANNEL_DONE) || (r_channel_fsm[k] == CHANNEL_ERROR);
    }
}

/////////////////////////
tmpl(void)::print_trace()
{
    const char* tgt_state_str[] = {
        "  TGT_IDLE ",
        "  TGT_READ ",
        "  TGT_WRITE",
        "  TGT_ERROR"};
    const char* cmd_state_str[] = {
        "  CMD_IDLE ",
        "  CMD_READ ",
        "  CMD_WRITE"};
    const char* rsp_state_str[] = {
        "  RSP_IDLE ",
        "  RSP_READ ",
        "  RSP_WRITE"};
    const char* channel_state_str[] = {
        "  CHANNEL_IDLE      ",
        "  CHANNEL_READ_REQ  ",
        "  CHANNEL_READ_WAIT ",
        "  CHANNEL_WRITE_REQ ",
        "  CHANNEL_WRITE_WAIT",
        "  CHANNEL_DONE      ",
        "  CHANNEL_ERROR     "};

    std::cout << "MULTI_DMA " << name() << " : " << tgt_state_str[r_tgt_fsm.read()] << std::endl;
    for ( size_t k = 0 ; k < m_channels ; k++ )
    {
        if ( r_activate[k].read() )
        {
            std::cout << "  CHANNEL " << k << std::hex
                      << " : " << channel_state_str[r_channel_fsm[k].read()]
                      << " / src = " << r_src_addr[k].read()
                      << " / dst = " << r_dst_addr[k].read() << std::dec
                      << " / len = " << r_length[k].read() << std::endl;
        }
    }
    std::cout << cmd_state_str[r_cmd_fsm.read()] 
              << " / channel = " << r_cmd_index.read()
              << " / length = " << r_cmd_length.read()
              << " / count = " << r_cmd_count.read()/4 << std::endl;
    std::cout << rsp_state_str[r_rsp_fsm.read()] 
              << " / channel = " << r_rsp_index.read()
              << " / length = " << r_rsp_length.read()
              << " / count = " << r_rsp_count.read()/4 << std::endl;
}

////////////////////////////////////////////////////////////////////
tmpl(/**/)::VciMultiDma( sc_core::sc_module_name 		name,
                         const soclib::common::MappingTable 	&mt,
                         const soclib::common::IntTab 		&srcid,
                         const soclib::common::IntTab 		&tgtid,
	                 const size_t 				burst_max_length,
                         const size_t 				channels )
	: caba::BaseModule(name),
          r_tgt_fsm("r_tgt_fsm"),
          r_srcid("r_srcid"),
          r_trdid("r_trdid"),
          r_pktid("r_pktid"),
          r_rdata("r_rdata"),
          r_activate(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_activate", channels)),
          r_channel_fsm(soclib::common::alloc_elems<sc_signal<int> >
                    ("r_channel_fsm", channels)),
          r_src_addr(soclib::common::alloc_elems<sc_signal<typename vci_param::addr_t> >
                    ("r_src_addr", channels)),
          r_dst_addr(soclib::common::alloc_elems<sc_signal<typename vci_param::addr_t> >
                    ("r_dst_addr", channels)),
          r_length(soclib::common::alloc_elems<sc_signal<size_t> >
                    ("r_length", channels)),
          r_buf(soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >
                    ("r_buf", channels, burst_max_length/4)),
          r_done(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_done", channels)),
          r_error(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_error", channels)),
          r_cmd_fsm("r_cmd_fsm"),
          r_cmd_count("r_cmd_count"),
          r_cmd_index("r_cmd_index"),
          r_cmd_length("r_cmd_length"),
          r_rsp_fsm("r_rsp_fsm"),
          r_rsp_count("r_rsp_count"),
          r_rsp_index("r_rsp_index"),
          r_rsp_length("r_rsp_length"),

	  m_segment(mt.getSegment(tgtid)),
	  m_burst_max_length(burst_max_length),
          m_channels(channels),
          m_srcid(mt.indexForId(srcid)),

          p_clk("p_clk"),
          p_resetn("p_resetn"),
          p_vci_target("p_vci_target"),
          p_vci_initiator("p_vci_initiator"),
          p_irq(soclib::common::alloc_elems<sc_core::sc_out<bool> >("p_irq", channels))
{
    assert( (vci_param::B == 4) && 
    "VCI_MULTI_DMA error : The VCI data field must be 32 bits");
    assert( burst_max_length && 
    "VCI_MULTI_DMA error : The requested burst length cannot be 0");
    assert( (burst_max_length < (1<<vci_param::K)) && 
    "VCI_MULTI_DMA error : The requested burst length is not possible with the current VCI PLEN size");
    assert( (burst_max_length%4 == 0) &&
    "VCI_MULTI_DMA error : The requested burst length must be multiple of 4 bytes");
    assert( (channels <= 8)  &&
    "VCI_MULTI_DMA error : The number of channels cannot be larger than 8");

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

