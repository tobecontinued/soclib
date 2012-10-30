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

//////////////////////////////////////////////////////////////////////////////////
//  This component is a multi-channels DMA controller supporting chained buffers.
//  It can be used to move a stream from one set of haîned buffers (src_chbuf) 
//  to another set of chained buffers (dst_chbuf), without involving software.
//
//  A "chbuf descriptor" is an array of "buffer descriptor", stored in main
//  memory. Each buffer descriptor contains two 32 bits words:
//  - STATUS[31:0] : buffer status (buffer full when STATUS non zero)
//  - PADDR[31:0]  : buffer base address
//  The buffer size must be the same for src_chbuf and dst_chbuf.
//
//  As this DMA controller uses a polling policy to access both the src_chbuf
//  and the dst chbuf, it introduces a delay between to access to a chbuf.
//  This delay is an optional constructor argument. Default value is 1000 cycles.
//  
//  This component makes the assumption that the VCI RDATA & WDATA fields
//  have 32 bits. The number of channels and the max size of a data burst are
//  constructor parameters. 
//
//  The chbuf descriptor address (CHBUF_DESC), and the number of chained 
//  buffers (CHBUF_NBUFS), as well as the elementary buffer size (BUF_SIZE)
//  are software parameters that must be  written in addressable registers 
//  when launching a transfer between two chbufs.
//
//  - The number of channels (simultaneous transfers) cannot be larger than 8.
//  - The burst length (in bytes) must be a power of 2 no larger than 128,
//    and is typically equal to the sytem cache line width.
//  - The elementary buffer size and all buffers base addresses must be multiple 
//    of 4 bytes. If the source and destination buffers are not aligned on a burst 
//    boundary, the DMA controler split the burst in two VCI transactions.
//
//  In order to support various protection mechanisms, each channel
//  takes 4K bytes in the address space, and the segment size is 32 K bytes. 
//  Only 8 address bits are decoded :
//  - The 5 bits ADDRESS[4:Ø] define the target register (see chbuf_dma.h)
//  - The 3 bits ADDRESS[14:12] define the selected channel.
//
//  For each channel, the relevant values for the channel status are:
//  - CHANNEL_IDLE           : 0
//  - CHANNEL_SRC_DESC_ERROR : 1
//  - CHANNEL_DST_DESC_ERROR : 2
//  - CHANNEL_SRC_DATA_ERROR : 3
//  - CHANNEL_DST_DATA_ERROR : 4
//  - CHANNEL_BUSY           : > 4
// 
//  There is one private IRQ line for each channel, that is only used
//  for error signaling.
//
//  In order to support multiple simultaneous transactions, the channel
//  index is transmited in the VCI TRDID field.
//  As the LSB bit of the TRDID is used to indicate a non-cachable access,
//  the channel index is encoded in the next 3 bits, and the TRDID width
//  must be at least 4 bits.
//  
//////////////////////////////////////////////////////////////////////////////////
//  Implementation note:
// 
//  The tgt_fsm controls the configuration commands and responses 
//  on the VCI target ports.
//
//  The cmd_fsm controls the read and write data transfer commands 
//  on the VCI initiator port. It uses four registers : 
//  - r_cmd fsm 
//  - r_cmd_count                counter of bytes in a burst 
//  - r_cmd_channel              selected channel
//  - r_cmd_bytes               VCI PLEN
//
//  The rsp_fsm controls the read and write data transfer responses 
//  on the VCI initiator port. It uses four registers : 
//  - r_rsp fsm 
//  - r_rsp_count                counter of bytes in a burst
//  - r_rsp_channel              selected channel
//  - r_rsp_bytes               VCI PLEN
//
//  Each channel [k] is controled by a channel FSM using the following registers: 
//
//  - r_channel_fsm[k]
//  - r_channel_run[k]	         channel active <=> transfer requested   (W)
//  - r_channel_buf_size[k]      buffer size (bytes) for SRC and DST     (R/W)
//
//  - r_channel_src_desc[k]      address of source chbuf descriptor      (R/W)
//  - r_channel_src_nbufs[k]     number of buffers in source chbuf       (R/W)
//  - r_channel_src_index[k]     current buffer index in source chbuf
//  - r_channel_src_addr[k]      current address in source buffer
//  - r_channel_src_offset[k]    number of non aligned bytes for source buffer
//  - r_channel_src_full[k]      current source buffer status
//
//  - r_channel_dst_desc[k]      address of destination chbuf descriptor (R/W)
//  - r_channel_dst_nbufs[k]     number of buffers in dest chbuf         (R/W)
//  - r_channel_dst_index[k]     current buffer index in dest chbuf
//  - r_channel_dst_addr[k]      current address in des buffer
//  - r_channel_dst_offset[k]    number of non aligned bytes for dest buffer
//  - r_channel_dst_full[k]      current destination buffer status
//
//  - r_channel_delay[k]         cycle counter for status polling
//  - r_channel_todo_bytes[k]    number of bytes not yet transfered in a buffer 
//  - r_channel_vci_req[k]       valid request from CHANNEL FSM to CMD FSM
//  - r_channel_vci_type[k]      request type  from CHANNEL FSM to CMD FSM
//  - r_channel_vci_rsp[k]       valid response from RSP FSM to CHANNEL FSM
//  - r_channel_vci_error[k]     error signaled by RSP FSM to CHANNEL FSM
//  - r_channel_bytes_first[k]   number of bytes for first data VCI transaction
//  - r_channel_bytes_second[k]  number of bytes for second data VCI transaction
//  - r_channel_last[k]          last read/write DMA transaction
//  - r_channel_buf[k][word]	 local burst buffer 
//
///////////////////////////////////i///////////////////////////////////////////////

#include <stdint.h>
#include <cassert>

#include "alloc_elems.h"
#include "../include/vci_chbuf_dma.h"
#include "../../../include/soclib/chbuf_dma.h"

namespace soclib { namespace caba {

#define tmpl(t) template<typename vci_param> t VciChbufDma<vci_param>

/////////////////////////
tmpl(void)::transition()
{
    if (!p_resetn) 
    {
        r_tgt_fsm    = TGT_IDLE;
        r_cmd_fsm    = CMD_IDLE;
        r_rsp_fsm    = RSP_IDLE;
        r_cmd_channel  = 0;
        r_rsp_channel  = 0;
        for ( uint32_t k = 0 ; k < m_channels ; k++ )
        {
            r_channel_fsm[k] 	   = CHANNEL_IDLE;
            r_channel_run[k]	   = false;
            r_channel_vci_req[k]   = false;   
            r_channel_vci_rsp[k]   = false;   
        }
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // This TGT_FSM controls the VCI TARGET port
    // It access the following registers:
    //  - r_channel_run[k]	         channel active => transfer requested    (W)
    //  - r_channel_buf_size[k]      buffer size (bytes) for both SRC & DST  (R/W)
    //  - r_channel_src_desc[k]      address of source chbuf descriptor      (R/W)
    //  - r_channel_src_nbufs[k]     number of buffers in source chbuf       (R/W)
    //  - r_channel_dst_desc[k]      address of destination chbuf descriptor (R/W)
    //  - r_channel_dst_nbufs[k]     number of buffers in dest chbuf         (R/W)
    ///////////////////////////////////////////////////////////////////////////////

    switch(r_tgt_fsm.read()) 
    {
        case TGT_IDLE:
        {
            if (p_vci_target.cmdval.read() )
            {
                typename vci_param::addr_t	address = p_vci_target.address.read();
                typename vci_param::data_t	wdata   = p_vci_target.wdata.read();
                typename vci_param::cmd_t	cmd     = p_vci_target.cmd.read();
               
                r_tgt_srcid					= p_vci_target.srcid.read();
                r_tgt_trdid					= p_vci_target.trdid.read();
                r_tgt_pktid					= p_vci_target.pktid.read();
                
                int 	cell    = (int)((address & 0x1C) >> 2);
                uint32_t	channel = (uint32_t)((address & 0x7000) >> 12);

                assert( (channel < m_channels) and 
                "VCI_CHBUF_DMA error : The channel index (ADDR[14:12] is too large");

                assert( p_vci_target.eop.read() and
                "VCI_CHBUF_DMA error : A configuration request mut be one single VCI flit");

                //////////////////////////////////////////////////////////
	            if ( (cell == CHBUF_RUN) and (cmd == vci_param::CMD_WRITE) )
                {
                    r_channel_run[channel] = (wdata != 0);
                    r_tgt_fsm              = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////
	            else if ( (cell == CHBUF_STATUS) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_fsm[channel].read();
                    r_tgt_fsm   = TGT_READ;
                }
                ////////////////////////////////////////////////////////////////////
	            else if ( (cell == CHBUF_SRC_DESC) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( (not r_channel_run[channel].read()) and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    assert( (wdata%4 == 0) and
                    "VCI_CHBUF_DMA error : SRC descriptor address not multiple of 4");

                    r_channel_src_desc[channel]  = wdata;
                    r_channel_src_index[channel] = 0;
                    r_tgt_fsm                    = TGT_WRITE;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_DESC) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_src_desc[channel].read();
                    r_tgt_fsm   = TGT_READ;
                }
                ////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_DESC) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( (not r_channel_run[channel].read()) and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    assert( (wdata%4 == 0) and
                    "VCI_CHBUF_DMA error : DST descriptor address not multiple of 4");

                    r_channel_dst_desc[channel]  = wdata;
                    r_channel_dst_index[channel] = 0;
                    r_tgt_fsm                    = TGT_WRITE;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_DESC) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_dst_desc[channel].read();
                    r_tgt_fsm   = TGT_READ;
                }

                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_BUF_SIZE) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[channel].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    assert( (wdata%4 == 0) and
                    "VCI_CHBUF_DMA error : buffer size not multiple of 4");

                    r_channel_buf_size[channel] = wdata;
                    r_tgt_fsm                   = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_BUF_SIZE) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_buf_size[channel].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_NBUFS) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[channel].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    r_channel_src_nbufs[channel] = wdata;
                    r_tgt_fsm                     = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_NBUFS) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_src_nbufs[channel].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_NBUFS) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[channel].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    r_channel_dst_nbufs[channel] = wdata;
                    r_tgt_fsm                    = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_NBUFS) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_dst_nbufs[channel].read();
                    r_tgt_fsm   = TGT_READ;
                }

                /////
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

    /////////////////////////////////////////////////////////////////////
    // These CHANNEL_FSM define the transfer state for each channel.
    // Each channel FSM implements two nested loops:
    //
    // - In external loop, we get the SRC buffer address, and the
    //   DST buffer address from the chbufs descriptors, we transfer
    //   a full buffer (internal loop), and release SRC & DST buffers.
    // - In the internal loop, a burst of size m_burst_max_length
    //   (corresponding to the storage capacity of the local buffer)
    //   is transfered at each iteration.
    //   The read transaction is split in two VCI transactions
    //   if the source buffer is not aligned on a burst boundary.
    //   The write transaction is split in two VCI transactions
    //   if the destination buffer is not aligned on a burst boundary.
    //
    // Each channel FSM set the r_channel_vci_req[k] register to
    // request a VCI transaction to the CMD FSM. The CMD FSM analyse
    // the request type to build the relevant VCI command.
    // the RSP FSM analyse the request type to write the data in
    // the relevant register, and reset the r_channel_vci_req[k] register
    // to signal the VCI transaction completion.
    /////////////////////////////////////////////////////////////////////

    for ( uint32_t k=0 ; k<m_channels ; k++ )
    {
        switch( r_channel_fsm[k].read() )
        {
            //////////////////
            case CHANNEL_IDLE:
            {
                if ( r_channel_run[k] ) r_channel_fsm[k] = CHANNEL_READ_SRC_STATUS;
                break;
            }

            // get SRC buffer base address from SRC chbuf descriptor

            /////////////////////////////
            case CHANNEL_READ_SRC_STATUS:   // request VCI READ for SRC buffer status 
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_READ_SRC_STATUS;
                r_channel_fsm[k]      = CHANNEL_READ_SRC_STATUS_WAIT;
                break;
            }
            //////////////////////////////////
            case CHANNEL_READ_SRC_STATUS_WAIT:  // wait response for SRC buffer status
            {
                if ( r_channel_vci_rsp[k].read() ) 
                {
                    if ( r_channel_vci_error[k] )   
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_DESC_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;
                    if ( not r_channel_src_full[k].read() ) // buffer not full
                    {
                        r_channel_fsm[k]   = CHANNEL_READ_SRC_STATUS_DELAY;
                        r_channel_delay[k] = m_delay;
                    }
                    else                                  // buffer full
                    {
                        r_channel_fsm[k] = CHANNEL_READ_SRC_BUFADDR;
                    }
                }
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_SRC_STATUS_DELAY:  // delay to access SRC buffer status
            {
                uint32_t delay = r_channel_delay[k];
                if ( delay == 0 ) r_channel_fsm[k]   = CHANNEL_READ_SRC_STATUS;
                else              r_channel_delay[k] = delay - 1;
                break;
            }
            //////////////////////////////
            case CHANNEL_READ_SRC_BUFADDR:   // request VCI READ for SRC buffer address
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_READ_SRC_BUFADDR;
                r_channel_fsm[k]      = CHANNEL_READ_SRC_BUFADDR_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_SRC_BUFADDR_WAIT:  // wait response for SRC buffer address
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    if ( r_channel_vci_error[k] )   
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_DESC_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;
                    r_channel_fsm[k]     = CHANNEL_READ_DST_STATUS;
                }
                break;
            }

            // get DST buffer base address from DST chbuf descriptor

            /////////////////////////////
            case CHANNEL_READ_DST_STATUS:   // request VCI READ for DST buffer status 
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_READ_DST_STATUS;
                r_channel_fsm[k]      = CHANNEL_READ_DST_STATUS_WAIT;
                break;
            }
            //////////////////////////////////
            case CHANNEL_READ_DST_STATUS_WAIT:  // wait response for DST buffer status
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    if ( r_channel_vci_error[k] )   
                    {
                        r_channel_fsm[k] = CHANNEL_DST_DESC_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;

                    if ( r_channel_dst_full[k].read() ) // buffer full
                    {
                        r_channel_fsm[k]   = CHANNEL_READ_DST_STATUS_DELAY;
                        r_channel_delay[k] = m_delay;
                    }
                    else                                  // buffer not full
                    {
                        r_channel_fsm[k] = CHANNEL_READ_DST_BUFADDR;
                    }
                }
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_DST_STATUS_DELAY:  // delay to access DST buffer status
            {
                uint32_t delay = r_channel_delay[k];
                if ( delay == 0 ) r_channel_fsm[k]   = CHANNEL_READ_DST_STATUS;
                else              r_channel_delay[k] = delay - 1;
                break;
            }
            //////////////////////////////
            case CHANNEL_READ_DST_BUFADDR:   // request VCI READ for DST buffer address
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_READ_DST_BUFADDR;
                r_channel_fsm[k]      = CHANNEL_READ_DST_BUFADDR_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_DST_BUFADDR_WAIT:  // wait response for DST buffer address
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    if ( r_channel_vci_error[k] )   
                    {
                        r_channel_fsm[k] = CHANNEL_DST_DESC_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k]    = false;
                    r_channel_todo_bytes[k] = r_channel_buf_size[k].read();
                    r_channel_fsm[k]        = CHANNEL_READ_BURST;
                }
                break;
            }

            // move data from SRC buffer to DST buffer (internal loop)

            ///////////////////////////////
            case CHANNEL_READ_BURST:    // prepare the VCI READ burst
            {
                uint32_t first  = m_burst_max_length - r_channel_src_offset[k].read();
                uint32_t second = r_channel_src_offset[k].read();
                uint32_t length = r_channel_todo_bytes[k].read();

                if ( length > (first + second) ) 
                {
                    r_channel_bytes_first[k] = first;
                    r_channel_bytes_second[k] = second;
                    r_channel_last[k]   = false;
                }
                else if ( length > first  )
                {
                    r_channel_bytes_first[k] = first;
                    r_channel_bytes_second[k] = length - first;
                    r_channel_last[k]   = true;
                }
                else   // length <= first
                {
                    r_channel_bytes_first[k] = length;
                    r_channel_bytes_second[k] = 0;
                    r_channel_last[k]   = true;
                }
                r_channel_fsm[k] = CHANNEL_READ_REQ_FIRST;
                break;
            }
            ////////////////////////////
            case CHANNEL_READ_REQ_FIRST:	// request first VCI READ transaction
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_READ_FIRST_DATA;
                r_channel_fsm[k]      = CHANNEL_READ_WAIT_FIRST;
                break;
            }
            /////////////////////////////
            case CHANNEL_READ_WAIT_FIRST: 	// wait response for first VCI READ
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k]  = false;

                    if ( r_channel_vci_error[k] )   
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_DATA_ERROR;
                    }
                    else
                    {
                        r_channel_src_addr[k] = r_channel_src_addr[k].read() +
                                                r_channel_bytes_first[k].read();

                        if ( r_channel_bytes_second[k].read() == 0 ) 
                            r_channel_fsm[k] = CHANNEL_WRITE_BURST;
                        else            
                            r_channel_fsm[k] = CHANNEL_READ_REQ_SECOND;
                    }
                }
                break;
            }
            /////////////////////////////
            case CHANNEL_READ_REQ_SECOND:   // request second VCI READ transaction
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_READ_SECOND_DATA;
                r_channel_fsm[k]      = CHANNEL_READ_WAIT_SECOND;
                break;
            }
            //////////////////////////////
            case CHANNEL_READ_WAIT_SECOND:  // wait response for second VCI READ
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k]  = false;

                    if ( r_channel_vci_error[k] )   
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_DATA_ERROR;
                    }
                    else
                    {
                        r_channel_src_addr[k] = r_channel_src_addr[k].read() +
                                                r_channel_bytes_second[k].read();
                        r_channel_fsm[k] = CHANNEL_WRITE_BURST;
                    }
                }
                break;
            }
            ////////////////////////////////
            case CHANNEL_WRITE_BURST:	// prepare the VCI WRITE transaction(s)
            {
                uint32_t first  = m_burst_max_length - r_channel_dst_offset[k].read();
                uint32_t second = r_channel_dst_offset[k].read();
                uint32_t length = r_channel_todo_bytes[k].read();

                if ( length > (first + second) ) 
                {
                    r_channel_bytes_first[k] = first;
                    r_channel_bytes_second[k] = second;
                    r_channel_last[k]   = false;
                }
                else if ( length > first  )
                {
                    r_channel_bytes_first[k] = first;
                    r_channel_bytes_second[k] = length - first;
                    r_channel_last[k]   = true;
                }
                else   // length <= first
                {
                    r_channel_bytes_first[k] = length;
                    r_channel_bytes_second[k] = 0;
                    r_channel_last[k]   = true;
                }
                r_channel_fsm[k] = CHANNEL_WRITE_REQ_FIRST;
                break;
            }
            /////////////////////////////
            case CHANNEL_WRITE_REQ_FIRST:	// request first VCI WRITE transaction
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_WRITE_FIRST_DATA;
                r_channel_fsm[k]      = CHANNEL_WRITE_WAIT_FIRST;
                break;
            }
            //////////////////////////////
            case CHANNEL_WRITE_WAIT_FIRST:	// wait response for first VCI WRITE
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k] = false;

                    if ( r_channel_vci_error[k] )
                    {
                        r_channel_fsm[k] = CHANNEL_DST_DATA_ERROR;
                        break;
                    }

                    r_channel_dst_addr[k] = r_channel_dst_addr[k].read() +
                                            r_channel_bytes_first[k].read();

                    if ( r_channel_bytes_second[k].read() != 0 ) 
                    {
                            r_channel_fsm[k] = CHANNEL_WRITE_REQ_SECOND;
                    }
                    else          
                    {
                        if ( r_channel_last[k].read() ) // buffer completed
                        {
                            r_channel_fsm[k] = CHANNEL_SRC_STATUS_WRITE;
                        }
                        else
                        {
                            r_channel_todo_bytes[k] = r_channel_todo_bytes[k].read()
                                                       - m_burst_max_length;
                            r_channel_fsm[k]        = CHANNEL_READ_BURST;
                        }
                    }
                }
                break;
            }
            //////////////////////////////
            case CHANNEL_WRITE_REQ_SECOND:	// request second VCI WRITE transaction
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_WRITE_SECOND_DATA;
                r_channel_fsm[k]      = CHANNEL_WRITE_WAIT_SECOND;
                break;
            }
            ///////////////////////////////
            case CHANNEL_WRITE_WAIT_SECOND:	// wait response for second VCI WRITE
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k] = false;

                    if ( r_channel_vci_error[k] )
                    {
                        r_channel_fsm[k] = CHANNEL_DST_DATA_ERROR;
                        break;
                    }

                    r_channel_dst_addr[k] = r_channel_dst_addr[k].read() +
                                            r_channel_bytes_second[k].read();

                    if ( r_channel_last[k].read() ) 
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_STATUS_WRITE;
                    }
                    else
                    {
                        r_channel_todo_bytes[k] = r_channel_todo_bytes[k].read()
                                                       - m_burst_max_length;
                        r_channel_fsm[k]        = CHANNEL_READ_BURST;
                    }
                }
                break;
            }

            // Release SRC & DST buffers and increment buffer indexes 

            /////////////////////////////
            case CHANNEL_SRC_STATUS_WRITE:
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_WRITE_SRC_STATUS;
                r_channel_fsm[k]      = CHANNEL_SRC_STATUS_WRITE_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_SRC_STATUS_WRITE_WAIT:
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k] = false;
                    if ( r_channel_vci_error[k] ) r_channel_fsm[k] = CHANNEL_SRC_DESC_ERROR;
                    else                      r_channel_fsm[k] = CHANNEL_DST_STATUS_WRITE;
                }
                break;
            }
            //////////////////////////////
            case CHANNEL_DST_STATUS_WRITE:
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_type[k] = REQ_WRITE_SRC_STATUS;
                r_channel_fsm[k]      = CHANNEL_SRC_STATUS_WRITE_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_DST_STATUS_WRITE_WAIT:
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k] = false;
                    if ( r_channel_vci_error[k] ) r_channel_fsm[k] = CHANNEL_DST_DESC_ERROR;
                    else                      r_channel_fsm[k] = CHANNEL_SRC_NEXT_BUFFER;
                }
                break;
            }
            /////////////////////////////
            case CHANNEL_SRC_NEXT_BUFFER:  // update SRC buffer descriptor index
            {
                if ( r_channel_src_index[k].read() == (r_channel_src_nbufs[k].read()-1) )
                {
                    r_channel_src_index[k] = 0;
                }
                else
                {
                    r_channel_src_index[k] = r_channel_src_index[k].read()+1;
                }
                r_channel_fsm[k] = CHANNEL_DST_NEXT_BUFFER;
                break;
            }
            /////////////////////////////
            case CHANNEL_DST_NEXT_BUFFER:  // update DST buffer descriptor index
            {
                if ( r_channel_dst_index[k].read() == (r_channel_dst_nbufs[k].read()-1) )
                {
                    r_channel_dst_index[k] = 0;
                }
                else
                {
                    r_channel_dst_index[k] = r_channel_dst_index[k].read()+1;
                }
                r_channel_fsm[k] = CHANNEL_IDLE;
                break;
            }

            // errors states
            ////////////////////////
            case CHANNEL_SRC_DATA_ERROR:
            case CHANNEL_DST_DATA_ERROR:
            case CHANNEL_SRC_DESC_ERROR:
            case CHANNEL_DST_DESC_ERROR:
            {
                if ( not r_channel_run[k] )	r_channel_fsm[k] = CHANNEL_IDLE;
                break; 
            }
        } // end switch r_channel_fsm[k]
    }
                
    ////////////////////////////////////////////////////////////////////////////
    // This CMD_FSM controls the VCI INIT command port
    // It updates the r_channel_src_addr[k] & r_channel_dst_addr[k] registers.
    ////////////////////////////////////////////////////////////////////////////

    switch(r_cmd_fsm.read()) 
    {
        //////////////
        case CMD_IDLE:
        {
            // round-robin arbitration between channels to send a command
            bool not_found = true;

            for( uint32_t n = 0 ; (n < m_channels) and not_found ; n++ )
            {
                uint32_t k = (r_cmd_channel.read() + n) % m_channels;
                if ( r_channel_vci_req[k].read() )
                {
                    not_found      = false;
                    r_cmd_channel  = k;
                    r_cmd_count    = 0;

                    switch ( r_channel_vci_type[k].read() )
                    {
                        case REQ_READ_SRC_STATUS:
                        {
                            r_cmd_address = r_channel_src_desc[k].read() + 
                                           (r_channel_src_index[k].read() << 3);
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_DST_STATUS:
                        {
                            r_cmd_address = r_channel_dst_desc[k].read() + 
                                           (r_channel_dst_index[k].read() << 3);
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_SRC_BUFADDR:
                        {
                            r_cmd_address = r_channel_src_desc[k].read() + 4 + 
                                           (r_channel_src_index[k].read() << 3);
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_DST_BUFADDR:
                        {
                            r_cmd_address = r_channel_dst_desc[k].read() + 4 +
                                           (r_channel_dst_index[k].read() << 3);
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_FIRST_DATA:
                        {
                            r_cmd_address         = r_channel_src_addr[k].read();
                            r_cmd_bytes           = r_channel_bytes_first[k].read();
                            r_cmd_fsm             = CMD_READ;
                            r_channel_src_addr[k] = r_channel_src_addr[k].read() +
                                                    r_channel_bytes_first[k].read();
                            break;
                        }
                        case REQ_READ_SECOND_DATA:
                        {
                            r_cmd_address         = r_channel_src_addr[k].read();
                            r_cmd_bytes           = r_channel_bytes_second[k].read();
                            r_cmd_fsm             = CMD_READ;
                            r_channel_src_addr[k] = r_channel_src_addr[k].read() +
                                                    r_channel_bytes_second[k].read();
                            break;
                        }
                        case REQ_WRITE_FIRST_DATA:
                        {
                            r_cmd_address         = r_channel_dst_addr[k].read();
                            r_cmd_bytes           = r_channel_bytes_first[k].read();
                            r_cmd_fsm             = CMD_WRITE;
                            r_channel_dst_addr[k] = r_channel_dst_addr[k].read() +
                                                    r_channel_bytes_first[k].read();
                            break;
                        }
                        case REQ_WRITE_SECOND_DATA:
                        {
                            r_cmd_address         = r_channel_dst_addr[k].read();
                            r_cmd_bytes           = r_channel_bytes_second[k].read();
                            r_cmd_fsm             = CMD_WRITE;
                            r_channel_dst_addr[k] = r_channel_dst_addr[k].read() +
                                                    r_channel_bytes_second[k].read();
                            break;
                        }
                        case REQ_WRITE_SRC_STATUS:
                        {
                            r_cmd_address = r_channel_src_desc[k].read() + 
                                           (r_channel_src_index[k].read() << 3);
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_WRITE;
                            break;
                        }
                        case REQ_WRITE_DST_STATUS:
                        {
                            r_cmd_address = r_channel_dst_desc[k].read() + 
                                           (r_channel_dst_index[k].read() << 3);
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_WRITE;
                            break;
                        }
                    } // end switch
                    r_channel_vci_req[k]= false;
                } // end if
            }
            break;
        }
        //////////////
        case CMD_READ:
        {
            if ( p_vci_initiator.cmdack.read() )
            {
                r_cmd_fsm = CMD_IDLE;
            }
            break;
        }
        ///////////////
        case CMD_WRITE:
        {
            if ( p_vci_initiator.cmdack.read() )
            {
                if ( r_cmd_count.read() == (r_cmd_bytes.read() - 4) )
                {
                    r_cmd_fsm = CMD_IDLE;
                }
                r_cmd_count = r_cmd_count.read() + 4;
            }
            break;
        }
    } // end switch cmd_fsm

    /////////////////////////////////////////////////////////////////////////
    // This RSP_FSM controls the VCI INIT response port
    // It writes in the relevant register, depending ont the transaction
    // defined by the r_channel_vci_req[k] registers.
    // It reset r_channel_vci_req[k], and set r_channel_vci_error[k]
    // to signal completion of the read / write VCI transaction.
    /////////////////////////////////////////////////////////////////////////
    switch(r_rsp_fsm.read()) 
    {
        //////////////
        case RSP_IDLE:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                uint32_t k      = (uint32_t)p_vci_initiator.rtrdid.read()>>1;
                switch ( r_channel_vci_req[k].read() ) 
                {
                    case REQ_READ_SRC_STATUS:
                        r_rsp_fsm = RSP_READ_SRC_STATUS;
                        break;
                    case REQ_READ_SRC_BUFADDR:
                        r_rsp_fsm = RSP_READ_SRC_BUFADDR;
                        break;
                    case REQ_READ_DST_STATUS:
                        r_rsp_fsm = RSP_READ_DST_STATUS;
                        break;
                    case REQ_READ_DST_BUFADDR:
                        r_rsp_fsm = RSP_READ_DST_BUFADDR;
                        break;
                    case REQ_READ_FIRST_DATA:
                        r_rsp_count = 0;
                    case REQ_READ_SECOND_DATA:
                        r_rsp_fsm = RSP_READ_DATA; 
                        break;
                    case REQ_WRITE_FIRST_DATA:
                    case REQ_WRITE_SECOND_DATA:
                    case REQ_WRITE_SRC_STATUS:
                    case REQ_WRITE_DST_STATUS:
                        r_rsp_fsm = RSP_WRITE;
                        break;
                } // end switch
                r_rsp_channel = k;
            }
            break;
        } 
        /////////////////////////
        case RSP_READ_SRC_STATUS:
        {
            uint32_t k              = r_rsp_channel.read();
            r_channel_src_full[k]   = (p_vci_initiator.rdata.read() != 0);
            r_channel_vci_rsp[k]    = true;
            r_channel_vci_error[k]  = (p_vci_initiator.rerror.read()&0x1 != 0);
            r_rsp_fsm               = RSP_IDLE;
            break;
        }
        //////////////////////////
        case RSP_READ_SRC_BUFADDR:
        {
            uint32_t k              = r_rsp_channel.read();
            uint32_t rdata          = p_vci_initiator.rdata.read();
            r_channel_src_addr[k]   = rdata;
            r_channel_src_offset[k] = rdata % m_burst_max_length;
            r_channel_vci_rsp[k]    = true;
            r_channel_vci_error[k]  = (p_vci_initiator.rerror.read()&0x1 != 0);
            r_rsp_fsm               = RSP_IDLE;
            break;
        }
        /////////////////////////
        case RSP_READ_DST_STATUS:
        {
            uint32_t k              = r_rsp_channel.read();
            r_channel_dst_full[k]   = (p_vci_initiator.rdata.read() != 0);
            r_channel_vci_rsp[k]    = true;
            r_channel_vci_error[k]  = (p_vci_initiator.rerror.read()&0x1 != 0);
            r_rsp_fsm               = RSP_IDLE;
            break;
        }
        //////////////////////////
        case RSP_READ_DST_BUFADDR:
        {
            uint32_t k              = r_rsp_channel.read();
            uint32_t rdata          = p_vci_initiator.rdata.read();
            r_channel_dst_addr[k]   = rdata;
            r_channel_dst_offset[k] = rdata % m_burst_max_length;
            r_channel_vci_rsp[k]    = true;
            r_channel_vci_error[k]  = (p_vci_initiator.rerror.read()&0x1 != 0);
            r_rsp_fsm               = RSP_IDLE;
        }
        ///////////////////
        case RSP_READ_DATA:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                uint32_t k    = r_rsp_channel.read();
                uint32_t word = r_rsp_count.read()>>2;
                r_channel_buf[k][word] = p_vci_initiator.rdata.read(); 
                r_rsp_count = r_rsp_count.read() + 4;

                if ( p_vci_initiator.reop.read() )
                {
                    assert( (r_rsp_count.read() < m_burst_max_length) and
                    "VCI_CHBUF_DMA error : wrong number of flits for a read response packet");

                    r_channel_vci_rsp[k]    = true;
                    r_channel_vci_error[k] = (p_vci_initiator.rerror.read()&0x1 != 0);
                    r_rsp_fsm = RSP_IDLE;
                } 
            }
            break;
        } 
        ///////////////
        case RSP_WRITE:
        {
            assert( (p_vci_initiator.reop.read() == true) and
             "VCI_CHBUF_DMA error : write response packed contains more than one flit");  

            uint32_t k             = r_rsp_channel.read();
            r_channel_vci_rsp[k]   = true;
            r_channel_vci_error[k] = (p_vci_initiator.rerror.read()&0x1 != 0);
            r_rsp_fsm              = RSP_IDLE;
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
            p_vci_initiator.cmdval  = true;
            p_vci_initiator.address = r_cmd_address.read();
            p_vci_initiator.wdata   = 0;
            p_vci_initiator.be      = 0xF;
            p_vci_initiator.plen    = r_cmd_bytes.read();
            p_vci_initiator.cmd     = vci_param::CMD_READ;
            p_vci_initiator.trdid   = r_cmd_channel.read()<<1;	
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
            uint32_t k    = r_cmd_channel.read();
            uint32_t n    = r_cmd_count.read() / 4;
            uint32_t wdata;
            if      ( r_channel_vci_req[k] == REQ_WRITE_SRC_STATUS ) wdata = 0;
            else if ( r_channel_vci_req[k] == REQ_WRITE_DST_STATUS ) wdata = 1;
            else    wdata = r_channel_buf[k][n].read();

            p_vci_initiator.cmdval  = true;
            p_vci_initiator.address = r_cmd_address.read() + r_cmd_count.read();
            p_vci_initiator.wdata   = wdata;
            p_vci_initiator.be      = 0xF;
            p_vci_initiator.plen    = r_cmd_bytes.read();
            p_vci_initiator.cmd     = vci_param::CMD_WRITE;
            p_vci_initiator.trdid   = r_cmd_channel.read()<<1;	
            p_vci_initiator.pktid   = 0;
            p_vci_initiator.srcid   = m_srcid;
            p_vci_initiator.cons    = false;
            p_vci_initiator.wrap    = false;
            p_vci_initiator.contig  = true;
            p_vci_initiator.clen    = 0;
            p_vci_initiator.cfixed  = false;
            p_vci_initiator.eop     = ( r_cmd_count.read() == r_cmd_bytes.read() - 4 );
            break;
        }
    } // end switch cmd_fsm

    /////// VCI INIT RSP port ////// 
    if ( r_rsp_fsm.read() == RSP_IDLE )  p_vci_initiator.rspack = false;
    else                                 p_vci_initiator.rspack = true;

    ////// VCI TARGET port /////// 
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
            p_vci_target.rsrcid = r_tgt_srcid.read();
            p_vci_target.rtrdid = r_tgt_trdid.read();
            p_vci_target.rpktid = r_tgt_pktid.read();
            p_vci_target.reop   = true;
            break;
        }
        case TGT_READ:
        {
            p_vci_target.cmdack = false;
            p_vci_target.rspval = true;
            p_vci_target.rdata  = r_tgt_rdata.read();
            p_vci_target.rerror = vci_param::ERR_NORMAL;
            p_vci_target.rsrcid = r_tgt_srcid.read();
            p_vci_target.rtrdid = r_tgt_trdid.read();
            p_vci_target.rpktid = r_tgt_pktid.read();
            p_vci_target.reop   = true;
            break;
        }
        case TGT_ERROR:
        {
            p_vci_target.cmdack = false;
            p_vci_target.rspval = true;
            p_vci_target.rdata  = 0;
            p_vci_target.rerror = vci_param::ERR_GENERAL_DATA_ERROR;
            p_vci_target.rsrcid = r_tgt_srcid.read();
            p_vci_target.rtrdid = r_tgt_trdid.read();
            p_vci_target.rpktid = r_tgt_pktid.read();
            p_vci_target.reop   = true;
            break;
        }
    } // end switch rsp_fsm

    /////// IRQ ports //////////
    for ( uint32_t k = 0 ; k < m_channels ; k++ )
    {
	    p_irq[k] = (r_channel_fsm[k] == CHANNEL_SRC_DESC_ERROR) || 
                   (r_channel_fsm[k] == CHANNEL_DST_DESC_ERROR) ||
                   (r_channel_fsm[k] == CHANNEL_SRC_DATA_ERROR) ||
                   (r_channel_fsm[k] == CHANNEL_DST_DATA_ERROR);
    }
}

/////////////////////////
tmpl(void)::print_trace()
{
    const char* tgt_state_str[] = 
    {
        "  TGT_IDLE",
        "  TGT_READ",
        "  TGT_WRITE",
        "  TGT_ERROR"
    };
    const char* cmd_state_str[] = 
    {
        "  CMD_IDLE",
        "  CMD_READ",
        "  CMD_WRITE"
    };
    const char* rsp_state_str[] = 
    {
        "  RSP_IDLE",
        "  RSP_READ",
        "  RSP_WRITE"
    };
    const char* channel_state_str[] = 
    {
        "  CHANNEL_IDLE",

        "  CHANNEL_SRC_DATA_ERROR",
        "  CHANNEL_DST_DATA_ERROR",
        "  CHANNEL_SRC_DESC_ERROR",
        "  CHANNEL_DST_DESC_ERROR",

        "  CHANNEL_READ_SRC_STATUS",
        "  CHANNEL_READ_SRC_STATUS_WAIT",
        "  CHANNEL_READ_SRC_STATUS_DELAY",
        "  CHANNEL_READ_SRC_BUFADDR",
        "  CHANNEL_READ_SRC_BUFADDR_WAIT",

        "  CHANNEL_READ_DST_STATUS",
        "  CHANNEL_READ_DST_STATUS_WAIT",
        "  CHANNEL_READ_DST_STATUS_DELAY",
        "  CHANNEL_READ_DST_BUFADDR",
        "  CHANNEL_READ_DST_BUFADDR_WAIT",

        "  CHANNEL_READ_BURST",
        "  CHANNEL_READ_REQ_FIRST",
        "  CHANNEL_READ_WAIT_FIRST",
        "  CHANNEL_READ_REQ_SECOND",
        "  CHANNEL_READ_WAIT_SECOND",

        "  CHANNEL_WRITE_BURST",
        "  CHANNEL_WRITE_REQ_FIRST",
        "  CHANNEL_WRITE_WAIT_FIRST"
        "  CHANNEL_WRITE_REQ_SECOND",
        "  CHANNEL_WRITE_WAIT_SECOND",

        "  CHANNEL_SRC_STATUS_WRITE",
        "  CHANNEL_SRC_STATUS_WRITE_WAIT",
        "  CHANNEL_DST_STATUS_WRITE",
        "  CHANNEL_DST_STATUS_WRITE_WAIT",
        "  CHANNEL_SRC_NEXT_BUFFER",
        "  CHANNEL_DST_NEXT_BUFFER",
    };

    std::cout << "CHBUF_DMA " << name() << " : " 
              << tgt_state_str[r_tgt_fsm.read()] << std::endl;
    for ( uint32_t k = 0 ; k < m_channels ; k++ )
    {
        std::cout << "  CHANNEL " << k << std::hex
                  << " : active = " << r_channel_run[k].read() 
                  << " : state = " << channel_state_str[r_channel_fsm[k].read()]
                  << " / src = " << r_channel_src_addr[k].read()
                  << " / dst = " << r_channel_dst_addr[k].read() << std::dec
                  << std::endl;
    }
    std::cout << cmd_state_str[r_cmd_fsm.read()] << std::dec 
              << " / channel = " << r_cmd_channel.read()
              << " / length = " << r_cmd_bytes.read()
              << " / count = " << r_cmd_count.read()/4 << std::endl;
    std::cout << rsp_state_str[r_rsp_fsm.read()] << std::dec 
              << " / channel = " << r_rsp_channel.read()
              << " / length = " << r_rsp_bytes.read()
              << " / count = " << r_rsp_count.read()/4 << std::endl;
}

////////////////////////////////////////////////////////////////////
tmpl(/**/)::VciChbufDma( sc_core::sc_module_name 		        name,
                         const soclib::common::MappingTable 	&mt,
                         const soclib::common::IntTab 		    &srcid,
                         const soclib::common::IntTab 		    &tgtid,
	                     const uint32_t 				        burst_max_length,
                         const uint32_t 				        channels,
                         const uint32_t 				        delay )
	: caba::BaseModule(name),

          r_tgt_fsm("r_tgt_fsm"),
          r_tgt_srcid("r_tgt_srcid"),
          r_tgt_trdid("r_tgt_trdid"),
          r_tgt_pktid("r_tgt_pktid"),
          r_tgt_rdata("r_tgt_rdata"),

          r_channel_fsm(soclib::common::alloc_elems<sc_signal<int> >
                    ("r_channel_fsm", channels)),
          r_channel_run(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_run", channels)),
          r_channel_buf_size(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_buf_size", channels)),

          r_channel_src_desc(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_desc", channels)),
          r_channel_src_nbufs(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_nbufs", channels)),
          r_channel_src_addr(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_addr", channels)),
          r_channel_src_index(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_index", channels)),
          r_channel_src_offset(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_offset", channels)),
          r_channel_src_full(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_src_full", channels)),

          r_channel_dst_desc(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_desc", channels)),
          r_channel_dst_nbufs(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_nbufs", channels)),
          r_channel_dst_addr(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_addr", channels)),
          r_channel_dst_index(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_index", channels)),
          r_channel_dst_offset(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_offset", channels)),
          r_channel_dst_full(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_dst_full", channels)),

          r_channel_delay(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_delay", channels)),
          r_channel_todo_bytes(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_todo_bytes", channels)),
          r_channel_bytes_first(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_bytes_first", channels)),
          r_channel_bytes_second(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_bytes_second", channels)),
          r_channel_vci_req(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_vci_req", channels)),
          r_channel_vci_type(soclib::common::alloc_elems<sc_signal<int> >
                    ("r_channel_vci_type", channels)),
          r_channel_vci_rsp(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_vci_rsp", channels)),
          r_channel_vci_error(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_vci_error", channels)),
          r_channel_last(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_last", channels)),
          r_channel_buf(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_buf", channels, burst_max_length/4)),

          r_cmd_fsm("r_cmd_fsm"),
          r_cmd_count("r_cmd_count"),
          r_cmd_address("r_cmd_address"),
          r_cmd_channel("r_cmd_channel"),
          r_cmd_bytes("r_cmd_bytes"),

          r_rsp_fsm("r_rsp_fsm"),
          r_rsp_count("r_rsp_count"),
          r_rsp_channel("r_rsp_channel"),
          r_rsp_bytes("r_rsp_bytes"),

          m_segment(mt.getSegment(tgtid)),
          m_burst_max_length(burst_max_length),
          m_channels(channels),
          m_srcid(mt.indexForId(srcid)),
          m_delay(delay),

          p_clk("p_clk"),
          p_resetn("p_resetn"),
          p_vci_target("p_vci_target"),
          p_vci_initiator("p_vci_initiator"),
          p_irq(soclib::common::alloc_elems<sc_core::sc_out<bool> >("p_irq", channels))
{
    assert( (vci_param::T >= 4) and 
    "VCI_CHBUF_DMA error : The VCI TRDID field must be at least 4 bits");

    assert( (vci_param::B == 4) and 
    "VCI_CHBUF_DMA error : The VCI DATA field must be 32 bits");

    assert( (burst_max_length < (1<<vci_param::K)) and 
    "VCI_CHBUF_DMA error : The VCI PLEN size is too small for requested burst length");

    assert( ((burst_max_length==4) or (burst_max_length==8) or (burst_max_length==16) or 
             (burst_max_length==32) or (burst_max_length==64)) or (burst_max_length==128) and
    "VCI_CHBUF_DMA error : The requested burst length must be 4, 8, 16, 32, 64, or 128 bytes");
    
    assert( (channels <= 8)  and
    "VCI_CHBUF_DMA error : The number of channels cannot be larger than 8");

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

