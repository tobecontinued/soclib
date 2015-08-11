/*
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
//  It can be used to move a stream from one set of chained buffers (SRC chbuf) 
//  to another set of chained buffers (DST chbuf), without involving software.
//
//  A "chbuf descriptor" is a circular array of "buffer descriptors". A buffer
//  descriptor contains the address of the buffer and the address of the buffer
//  status (full or not). The buffer and the status have the same address
//  extension (bits[43:32]). Both addresses must be a multiple of 64 bytes.
//  Each buffer descriptor occupies 8 bytes (64 bits unsigned long long):
//  - The 12 MSB bits contain the common extension of the buffer address and
//    the buffer status address
//  - The 26 following bits contain the bits [31:6] of the buffer address
//  - The 26 LSB bits contain the bits [31:6] of the buffer status address
//
//  The status occupies 64 bytes but only the LSB bit of first byte contains
//  useful information: 0 if buffer empty / 1 if buffer full.
//
//  The buffer length must be the same for src_chbuf and dst_chbuf.
//  The "chbuf descriptor" base address must be a multiple of 64 bytes.
//
//  This DMA controller implements two "modes" to scan the SRC and DST chbufs:
//
//  - IN ORDER FIFO: The chained buffers are read and write in strict order,
//    with a polling policy to access both the expected SRC chbuf and the 
//    expected DST chbuf. The delay between two accesses is defined, for each
//    channel, by the CHBUF_PERIOD addressable register, and must be non zero.
//  - OUT OF ORDER: The first full SRC buffer found is read, the first empty
//    DST buffer found is written, with a round robin priority for the search.
//    This mode is activated when the CHBUF_PERIOD value is zero (default value).
//  
//  This component supports both 32 bits and 64 bits VCI RDATA & WDATA fields.
//
//  The number of channels, the max burst size and the number of pipelined bursts
//  that can be pseudo-parallelised are constructor parameters. 
//
//  The chbuf descriptor address (CHBUF_DESC), and the number of chained 
//  buffers (CHBUF_NBUFS), as well as the elementary buffer size (BUF_SIZE)
//  are software parameters that must be  written in addressable registers 
//  when launching a transfer between two chbufs.
//
//  - The number of channels (simultaneous transfers) cannot be larger than 8.
//  - The burst length (in bytes) must be a power of 2 no larger than 64,
//    and is typically equal to the system cache line width.
//  - The number of pipelined bursts cannot be larger 4.
//  - The elementary buffer size must be multiple of 4 bytes.
//
//  In order to support various protection mechanisms, for each channel,
//  the channel addressable registers takes 4K bytes in the address space. 
//  Only 8 address bits are decoded .
//  - The 5 bits ADDRESS[4:Ø] define the target register (see chbuf_dma.h)
//  - The 3 bits ADDRESS[14:12] define the selected channel.
//
//  For each channel, the relevant values for the channel status are:
//  - CHANNEL_IDLE             : 0   / channel not running
//  - CHANNEL_SRC_DESC_ERROR   : 1   / bus error accessing SRC CHBUF descriptor
//  - CHANNEL_DST_DESC_ERROR   : 2   / bus error accessing DST CHBUF descriptor
//  - CHANNEL_SRC_STATUS_ERROR : 3   / bus error accessing SRC BUF status
//  - CHANNEL_DST_STATUS_ERROR : 4   / bus error accessing DST BUF status
//  - CHANNEL_DATA_ERROR       : 5   / bus error accessing SRC or DST CHBUF data
//  - CHANNEL_BUSY             : > 5 / channel running
// 
//  There is one private IRQ line for each channel, that is only used
//  for bus error signaling, and is activated when channel[k] enters
//  an error state. The channel can be reset by writing a nul value
//  in register CHBUF_RUN[k], focing channel[k] to IDLE state.
//
//  In order to support multiple simultaneous transactions, the channel
//  index is transmited in the VCI TRDID field.    
//////////////////////////////////////////////////////////////////////////////////
//  Implementation note:
// 
//  The tgt_fsm controls the configuration commands and responses 
//  on the VCI target ports.
//
//  The cmd_fsm controls the read and write data transfer commands 
//  on the VCI initiator port. It uses five registers : 
//  - r_cmd fsm 
//  - r_cmd_count      counter of bytes in VCI transaction (shared)
//  - r_cmd_address    VCI command address
//  - r_cmd_channel    selected channel
//  - r_cmd_bytes      VCI PLEN
//
//  The rsp_fsm controls the read and write data transfer responses 
//  on the VCI initiator port. It uses six registers : 
//  - r_rsp fsm 
//  - r_rsp_count      counter of bytes in the VCI response (one per channel)
//  - r_rsp_channel    selected channel
//  - r_rsp_bytes      VCI PLEN
//  - r_rsp_next_read  burst index of the next expected VCI rsp read (one per channel)
//  - r_rsp_next_write burst index of the next expected VCI rsp write (one per channel)
//
//  Each channel [k] is controled by a channel FSM using the following registers: 
//
//  - r_channel_fsm[k]
//  - r_channel_run[k]	        channel active <=> transfer requested   (W)
//  - r_channel_buf_size[k]     buffer size (bytes) for SRC and DST     (R/W)
//
//  - r_channel_src_desc[k]     address of source chbuf descriptor      (R/W)
//  - r_channel_src_nbufs[k]    number of buffers in source chbuf       (R/W)
//  - r_channel_src_buf_addr[k] current address of source buffer (32 LSB)
//  - r_channel_src_sts_addr[k] current address of source buffer status (32 LSB)
//  - r_channel_src_ext[k]      current address extension for src buffer and status address  
//  - r_channel_src_index[k]    current buffer index in source chbuf
//  - r_channel_src_full[k]     current source buffer status (full or empty)
//
//  - r_channel_dst_desc[k]     address of destination chbuf descriptor (R/W)
//  - r_channel_dst_nbufs[k]    number of buffers in dest chbuf         (R/W)
//  - r_channel_dst_buf_addr[k] current address of dest buffer (32 LSB)
//  - r_channel_dst_sts_addr[k] current address of dest buffer status (32 LSB)
//  - r_channel_dst_ext[k]      current address extension for dst buffer and status address
//  - r_channel_dst_index[k]    current buffer index in dest chbuf
//  - r_channel_dst_full[k]     current destination buffer status (full or empty)
//
//  - r_channel_timer[k]        cycle counter for status polling
//  - r_channel_period[k]       status polling period
//  - r_channel_todo_bytes[k]   number of words not yet transfered in a buffer
//  - r_channel_bytes[k][i];    burst length for burst number i
//  - r_channel_vci_req[k]      valid request from CHANNEL FSM to CMD FSM
//  - r_channel_vci_req_type[k] request type  from CHANNEL FSM to CMD FSM
//  - r_channel_vci_rsp[k]      valid and complete response from RSP FSM to CHANNEL FSM
//  - r_channel_rsp_read[k][i]  valid first flit of response from RSP FSM to CHANNEL FSM for a burst read transaction
//  - r_channel_rsp_write[k][i] valid response from RSP FSM to CHANNEL FSM for a burst write transaction
//  - r_channel_vci_error[k]    error signaled by RSP FSM to CHANNEL FSM
//  - r_channel_last[k]         last read/write DMA transaction
//  - r_channel_fifo[k]     	local fifo where data is stored 
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <cassert>

#include "alloc_elems.h"
#include "../../../../../../../lib/generic_fifo/include/caba/generic_fifo.h"
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
            r_channel_period[k]    = 1000;
            r_channel_run[k]	   = false;
            r_channel_vci_req[k]   = false;   
            r_channel_vci_rsp[k]   = false;
            r_rsp_next_read[k]     = 0;
            r_rsp_next_write[k]    = 0;
            r_channel_burst_id[k]  = 0;
            r_channel_fifo[k].init();
        }
        return;
    }

    // set default values for the two selected hardware FIFOs,
    bool     vci_fifo_put         = false;   // the RSP_FSM put data to only one hardware FIFO
    uint32_t vci_fifo_put_channel = 0;
    bool     vci_fifo_get         = false;   // the CMD_FSM get data from a hardware FIFO
    uint32_t vci_fifo_get_channel = 0;
    uint64_t vci_fifo_wdata       = 0;       // data written into hardware FIFO by RSP_FSM
    

    ///////////////////////////////////////////////////////////////////////////////
    // This TGT_FSM controls the VCI TARGET port
    // It access the following registers:
    //  - r_channel_run[k]	         channel active => transfer requested    (W)
    //  - r_channel_buf_size[k]      buffer size (bytes) for both SRC & DST  (R/W)
    //  - r_channel_src_desc[k]      source chbuf descriptor paddr           (R/W)
    //  - r_channel_src_nbufs[k]     number of buffers in source chbuf       (R/W)
    //  - r_channel_dst_desc[k]      dest chbuf descriptor paddr             (R/W)
    //  - r_channel_dst_nbufs[k]     number of buffers in dest chbuf         (R/W)
    //  - r_channel_period[k]        status polling period                   (R/W)
    ///////////////////////////////////////////////////////////////////////////////

    switch(r_tgt_fsm.read()) 
    {
        case TGT_IDLE:
        {
            if (p_vci_target.cmdval.read() )
            {
                typename vci_param::fast_addr_t	address = p_vci_target.address.read();
                typename vci_param::cmd_t	    cmd     = p_vci_target.cmd.read();

                bool found = false;
                std::list<soclib::common::Segment>::iterator seg;
                for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ ) 
                {
                    if ( seg->contains(address) ) found = true;
                }

                assert ( found  and
                "ERROR in VCI_CHBUF_DMA : VCI address is out of segment");
               
                r_tgt_srcid	= p_vci_target.srcid.read();
                r_tgt_trdid	= p_vci_target.trdid.read();
                r_tgt_pktid	= p_vci_target.pktid.read();
                
                int 	  cell = (int)((address & 0x3C) >> 2);
                uint32_t  k    = (uint32_t)((address & 0x7000) >> 12);

                assert( (k < m_channels) and 
                "VCI_CHBUF_DMA error : The channel index (ADDR[14:12] is too large");

                assert( p_vci_target.eop.read() and
                "VCI_CHBUF_DMA error : A configuration request must be one single flit");

                // get write data value for both 32 bits and 64 bits data width
                unsigned int wdata;
                if( (vci_param::B == 8) and (p_vci_target.be.read() == 0xF0) ) 
                    wdata = (uint32_t)(p_vci_target.wdata.read()>>32);
                else
                    wdata = p_vci_target.wdata.read();

                //////////////////////////////////////////////////////////
	            if ( (cell == CHBUF_RUN) and (cmd == vci_param::CMD_WRITE) )
                {
                    r_channel_run[k] = (wdata != 0);
                    r_tgt_fsm        = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////
	            else if ( (cell == CHBUF_STATUS) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_fsm[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                ////////////////////////////////////////////////////////////////////
	            else if ( (cell == CHBUF_SRC_DESC) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( (not r_channel_run[k].read()) and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    assert( (wdata%4 == 0) and
                    "VCI_CHBUF_DMA error : SRC descriptor address not multiple of 4");

                    r_channel_src_desc[k]  = wdata;
                    r_channel_src_index[k] = 0;
                    r_tgt_fsm              = TGT_WRITE;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_EXT) and (cmd == vci_param::CMD_WRITE) )
                {               
                    assert( (not r_channel_run[k].read()) and 
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");
                   
                    r_channel_src_desc[k] = (r_channel_src_desc[k].read() & 0XFFFFFFFF) + 
                                      ((uint64_t)wdata << 32) ;
                    r_tgt_fsm             = TGT_WRITE;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_DESC) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = (uint32_t)r_channel_src_desc[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_EXT) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = (uint32_t)(r_channel_src_desc[k].read()>>32);
                    r_tgt_fsm   = TGT_READ;
                }
                ////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_DESC) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( (not r_channel_run[k].read()) and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    assert( (wdata%4 == 0) and
                    "VCI_CHBUF_DMA error : DST descriptor address not multiple of 4");

                    r_channel_dst_desc[k]  = wdata;
                    r_channel_dst_index[k] = 0;
                    r_tgt_fsm              = TGT_WRITE;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_EXT) and (cmd == vci_param::CMD_WRITE) )
                {               
                    assert( (not r_channel_run[k].read()) and 
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");
                   
                    r_channel_dst_desc[k] = (r_channel_dst_desc[k].read() & 0XFFFFFFFF) + 
                                      ((typename vci_param::fast_addr_t)wdata << 32);
                    r_tgt_fsm             = TGT_WRITE;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_DESC) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_dst_desc[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                ///////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_EXT) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = (uint32_t)(r_channel_dst_desc[k].read()>>32);
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_BUF_SIZE) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[k].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    assert( (wdata%4 == 0) and
                    "VCI_CHBUF_DMA error : buffer size not multiple of 4");

                    r_channel_buf_size[k] = wdata;
                    r_tgt_fsm             = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_BUF_SIZE) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_buf_size[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_NBUFS) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[k].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    r_channel_src_nbufs[k] = wdata;
                    r_tgt_fsm              = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_NBUFS) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_src_nbufs[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_NBUFS) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[k].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    r_channel_dst_nbufs[k] = wdata;
                    r_tgt_fsm              = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_NBUFS) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_dst_nbufs[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_PERIOD) and (cmd == vci_param::CMD_WRITE) )
                {
                    assert( not r_channel_run[k].read() and
                    "VCI_CHBUF_DMA error : Configuration request for an active channel");

                    r_channel_period[k] = wdata;
                    r_tgt_fsm           = TGT_WRITE;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_PERIOD) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_period[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_SRC_INDEX) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_src_index[k].read();
                    r_tgt_fsm   = TGT_READ;
                }
                /////////////////////////////////////////////////////////////////////
                else if ( (cell == CHBUF_DST_INDEX) and (cmd == vci_param::CMD_READ) )
                {
                    r_tgt_rdata = r_channel_dst_index[k].read();
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
    // - In the internal loop, m_pipelined_bursts bursts of size m_burst_max_length
    //   are transfered at each iteration. (the storage capacity of the
    //   local fifo is m_pipelined_bursts*m_burst_max_length). The m_pipelined_bursts read
    //   requests are sent successively before the write requests are sent.
    //
    // Each channel FSM set the r_channel_vci_req[k] register to
    // request a VCI transaction to the CMD FSM. The CMD FSM analyse
    // the request type to build the relevant VCI command.
    // In the external loop, the RSP FSM analyse the request type to write
    // the data in the relevant register, and reset the r_channel_vci_req[k]
    // register to signal the VCI transaction completion.
    // In the internal loop, the RSP FSM analyse the LSB of the rtrtid field
    // of the VCI response, the r_rsp_next_read and the r_rsp_next_write
    // registers to identify the response type.
    /////////////////////////////////////////////////////////////////////

    for ( uint32_t k=0 ; k<m_channels ; k++ )
    {
        switch( r_channel_fsm[k].read() )
        {
            //////////////////
            case CHANNEL_IDLE:
            {
                if ( r_channel_run[k] ) r_channel_fsm[k] = CHANNEL_READ_SRC_DESC;
                break;
            }

            // read the buffer descriptor and get SRC buffer and status base addresses

           /////////////////////////////
            case CHANNEL_READ_SRC_DESC:   // request VCI READ for SRC buffer descriptor 
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_READ_SRC_DESC;
                r_channel_fsm[k]          = CHANNEL_READ_SRC_DESC_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_SRC_DESC_WAIT:  // wait response for SRC buffer descriptor
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    if ( r_channel_vci_error[k].read() )   
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_DESC_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;
                    r_channel_fsm[k]     = CHANNEL_READ_SRC_STATUS;
                }
                break;
            }
            //////////////////////////////
            case CHANNEL_READ_SRC_STATUS:   // request VCI READ for SRC buffer status
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_READ_SRC_STATUS;
                r_channel_fsm[k]          = CHANNEL_READ_SRC_STATUS_WAIT;
                break;
            }
            //////////////////////////////////
            case CHANNEL_READ_SRC_STATUS_WAIT:  // wait response for SRC buffer status
            {
                if ( r_channel_vci_rsp[k].read() ) 
                {
                    if ( r_channel_vci_error[k].read() )   
                    {
                        r_channel_fsm[k] = CHANNEL_SRC_STATUS_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;
                    if ( not r_channel_src_full[k].read() ) // buffer not full: failure
                    {
                        if ( r_channel_period[k] )          // in order mode
                        { 
                            r_channel_fsm[k]   = CHANNEL_READ_SRC_STATUS_DELAY;
                            r_channel_timer[k] = r_channel_period[k];
                        }
                        else                                // no order mode
                        {
                            // increment SRC buffer index
                            if ( r_channel_src_index[k].read() == 
                                 (r_channel_src_nbufs[k].read()-1) )
                            {
                                r_channel_src_index[k] = 0;
                            }
                            else
                            {
                                r_channel_src_index[k] = r_channel_src_index[k].read()+1;
                            }
                            r_channel_fsm[k]   = CHANNEL_READ_SRC_DESC;
                        }
                    }
                    else                                    // buffer full: success
                    {
                        r_channel_fsm[k] = CHANNEL_READ_DST_DESC;
                    }
                }
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_SRC_STATUS_DELAY:  // delay to access SRC buffer status
            {
                if ( r_channel_timer[k].read() == 0 ) 
                {
                    r_channel_fsm[k]   = CHANNEL_READ_SRC_STATUS;
                }
                else
                {
                    r_channel_timer[k] = r_channel_timer[k].read() - 1;
                }
                break;
            }

            // get DST buffer base address from DST chbuf descriptor

           /////////////////////////////
            case CHANNEL_READ_DST_DESC:   // request VCI READ for DST buffer descriptor 
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_READ_DST_DESC;
                r_channel_fsm[k]          = CHANNEL_READ_DST_DESC_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_DST_DESC_WAIT:  // wait response for DST buffer descriptor
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    if ( r_channel_vci_error[k].read() )   
                    {
                        r_channel_fsm[k] = CHANNEL_DST_DESC_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;
                    r_channel_fsm[k]     = CHANNEL_READ_DST_STATUS;
                }
                break;
            }
            //////////////////////////////
            case CHANNEL_READ_DST_STATUS:   // request VCI READ for DST buffer status
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_READ_DST_STATUS;
                r_channel_fsm[k]          = CHANNEL_READ_DST_STATUS_WAIT;
                break;
            }
            //////////////////////////////////
            case CHANNEL_READ_DST_STATUS_WAIT:  // wait response for DST buffer status
            {
                if ( r_channel_vci_rsp[k].read() ) 
                {
                    if ( r_channel_vci_error[k].read() )   
                    {
                        r_channel_fsm[k] = CHANNEL_DST_STATUS_ERROR;
                        break;
                    }
                    r_channel_vci_rsp[k] = false;
                    if ( r_channel_dst_full[k].read() ) // buffer full: failure
                    {
                        if ( r_channel_period[k] )          // in order mode
                        { 
                            r_channel_fsm[k]   = CHANNEL_READ_DST_STATUS_DELAY;
                            r_channel_timer[k] = r_channel_period[k];
                        }
                        else                                // no order mode
                        {
                            // increment DST buffer index
                            if ( r_channel_dst_index[k].read() == 
                                 (r_channel_dst_nbufs[k].read()-1) )
                            {
                                r_channel_dst_index[k] = 0;
                            }
                            else
                            {
                                r_channel_dst_index[k] = r_channel_dst_index[k].read()+1;
                            }
                            r_channel_fsm[k]   = CHANNEL_READ_DST_DESC;
                        }
                    }
                    else                                    // buffer not full: success
                    {
                        r_channel_todo_bytes[k] = r_channel_buf_size[k].read();
                        r_channel_fsm[k] = CHANNEL_BURST;
                    }
                }
                break;
            }
            ///////////////////////////////////
            case CHANNEL_READ_DST_STATUS_DELAY:  // delay to access DST buffer status
            {
                if ( r_channel_timer[k].read() == 0 ) 
                {
                    r_channel_fsm[k]   = CHANNEL_READ_DST_STATUS;
                }
                else
                {
                    r_channel_timer[k] = r_channel_timer[k].read() - 1;
                }
                break;
            }

            // move data from SRC buffer to DST buffer (internal loop)

            ////////////////////////
            case CHANNEL_BURST:    // prepare the VCI transactions for the following m_pipelined_bursts bursts
            {
                uint32_t length = r_channel_todo_bytes[k].read();

                for ( uint32_t i = 0 ; i < m_pipelined_bursts ; i++ )
                {
                    // re-initialize r_channel_rsp for read transactions
                    r_channel_rsp_read[k][i] = false;

                    // compute the length of each burst
                    if ( length > (i+1) * m_burst_max_length )
                        r_channel_bytes[k][i] = m_burst_max_length;
                    else
                        r_channel_bytes[k][i] = length - (i * m_burst_max_length);
                }

                r_channel_burst_id[k] = 0; // begin with burst number 0
                r_channel_last[k] =  length <= m_pipelined_bursts * m_burst_max_length;
                r_channel_fsm[k] = CHANNEL_READ_REQ;
                break;
            }
            ////////////////////////////
            case CHANNEL_READ_REQ:	// request VCI READ transaction
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_READ_DATA;
                r_channel_fsm[k]          = CHANNEL_READ_REQ_WAIT;
                break;
            }
            /////////////////////////////
            case CHANNEL_READ_REQ_WAIT: 	// wait end command for VCI READ
            {
                if ( not r_channel_vci_req[k] )
                {
                    uint32_t i = r_channel_burst_id[k].read();

                    r_channel_src_buf_addr[k] =
                        r_channel_src_buf_addr[k].read() +
                        r_channel_bytes[k][i].read();
                
                    // if there is another burst in this series 
                    if ( i+1 < m_pipelined_bursts  and r_channel_bytes[k][i+1].read() != 0 )
                    {
                        r_channel_burst_id[k] = i + 1; 
                        r_channel_fsm[k] = CHANNEL_READ_REQ;
                    }
                    else  // no other burst in this series
                    {
                        r_channel_burst_id[k] = 0; // re-initialize burst_id to 0 for write transactions 
                        r_channel_fsm[k] = CHANNEL_RSP_WAIT;
                    }
                }
                break;
            }
            
            //////////////////////////////////
            case CHANNEL_RSP_WAIT:  // wait first flit of response from RSP fsm for read data
                                    // and for the previous write data if there is one
            {
                uint32_t i = r_channel_burst_id[k].read();
                
                if ( r_channel_rsp_read[k][i].read() and
                     ( r_channel_rsp_write[k][i].read() or
                       (r_channel_todo_bytes[k].read() == r_channel_buf_size[k].read()) ))
                {
                    // re-initialize rsp write for this burst
                    r_channel_rsp_write[k][i] = false;
                    if (r_channel_vci_error[k].read())
                    {
                        r_channel_fsm[k] = CHANNEL_DATA_ERROR;
                    }
                    else
                    {
                        r_channel_fsm[k] = CHANNEL_WRITE_REQ;
                    }
                }
                break;
            }
            /////////////////////////////
            case CHANNEL_WRITE_REQ:	// request VCI WRITE transaction
            {
                r_channel_vci_req[k]  = true;
                r_channel_vci_req_type[k] = REQ_WRITE_DATA;
                r_channel_fsm[k]      = CHANNEL_WRITE_REQ_WAIT;
                break;
            }
            //////////////////////////////
            case CHANNEL_WRITE_REQ_WAIT:	// wait end command for VCI WRITE
            {
                if ( not r_channel_vci_req[k] )
                {
                    uint32_t i = r_channel_burst_id[k].read();

                    if ( r_channel_vci_error[k].read() )
                    {
                        r_channel_fsm[k] = CHANNEL_DATA_ERROR;
                    }
                    // else if there is another burst in this series
                    else if ( i+1 < m_pipelined_bursts  and r_channel_bytes[k][i+1].read() != 0 )
                    {
                        r_channel_burst_id[k] = i + 1;
                        r_channel_dst_buf_addr[k] =
                            r_channel_dst_buf_addr[k].read() +
                            r_channel_bytes[k][i].read();
                        r_channel_fsm[k] = CHANNEL_RSP_WAIT; 
                    }
                    else if ( r_channel_last[k].read() ) // buffer completed
                    {
                        r_channel_fsm[k] = CHANNEL_BURST_RSP_WAIT;
                    }
                    else // prepare the next series of bursts
                    {
                        r_channel_todo_bytes[k] = r_channel_todo_bytes[k].read() -
                                                  (m_pipelined_bursts * m_burst_max_length);
                        r_channel_dst_buf_addr[k] = r_channel_dst_buf_addr[k].read() +
                                                    r_channel_bytes[k][i].read();
                        r_channel_fsm[k] = CHANNEL_BURST;
                    }
                }
                break;
            }

            ////////////////////////////
            case CHANNEL_BURST_RSP_WAIT:  // wait response from the last write transaction(s)
            {
                bool finished = true;
                uint32_t i = 0;

                // check all the write rsp have been received
                for ( i = 0 ; i < m_pipelined_bursts ; i++ )
                {
                    if ( (r_channel_bytes[k][i].read() != 0) and
                         (!r_channel_rsp_write[k][i].read()) ) 
                        finished = false;
                }

                if ( finished )
                {
                   // re-initialize burst vci rsp
                    for ( i = 0 ; i < m_pipelined_bursts ; i++ )
                    {
                        r_channel_rsp_read[k][i] = false;
                    }

                    // if no error exit of the internal loop
                    if ( r_channel_vci_error[k].read() )
                        r_channel_fsm[k] = CHANNEL_DATA_ERROR;
                     else
                         r_channel_fsm[k] = CHANNEL_SRC_STATUS_WRITE;
                }
                break;
            }

            // Release SRC & DST buffers and increment buffer indexes 

            /////////////////////////////
            case CHANNEL_SRC_STATUS_WRITE:
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_WRITE_SRC_STATUS;
                r_channel_fsm[k]          = CHANNEL_SRC_STATUS_WRITE_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_SRC_STATUS_WRITE_WAIT:
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k] = false;
                    if ( r_channel_vci_error[k].read() ) 
                        r_channel_fsm[k] = CHANNEL_SRC_STATUS_ERROR;
                    else                      
                        r_channel_fsm[k] = CHANNEL_DST_STATUS_WRITE;
                }
                break;
            }
            //////////////////////////////
            case CHANNEL_DST_STATUS_WRITE:
            {
                r_channel_vci_req[k]      = true;
                r_channel_vci_req_type[k] = REQ_WRITE_DST_STATUS;
                r_channel_fsm[k]          = CHANNEL_DST_STATUS_WRITE_WAIT;
                break;
            }
            ///////////////////////////////////
            case CHANNEL_DST_STATUS_WRITE_WAIT:
            {
                if ( r_channel_vci_rsp[k].read() )
                {
                    r_channel_vci_rsp[k] = false;
                    if ( r_channel_vci_error[k].read() ) 
                        r_channel_fsm[k] = CHANNEL_DST_STATUS_ERROR;
                    else
                        r_channel_fsm[k] = CHANNEL_NEXT_BUFFERS;
                }
                break;
            }
            //////////////////////////
            case CHANNEL_NEXT_BUFFERS:  // update SRC & DST buffers index
            {
                if ( r_channel_src_index[k].read() == (r_channel_src_nbufs[k].read()-1) )
                {
                    r_channel_src_index[k] = 0;
                }
                else
                {
                    r_channel_src_index[k] = r_channel_src_index[k].read()+1;
                }

                if ( r_channel_dst_index[k].read() == (r_channel_dst_nbufs[k].read()-1) )
                {
                    r_channel_dst_index[k] = 0;
                }
                else
                {
                    r_channel_dst_index[k] = r_channel_dst_index[k].read()+1;
                }

                r_rsp_next_read[k] = 0;
                r_rsp_next_write[k] = 0;
                r_channel_fsm[k] = CHANNEL_IDLE;
                break;
            }

            ////////////////////////////
            case CHANNEL_DATA_ERROR:
            case CHANNEL_SRC_DESC_ERROR:
            case CHANNEL_DST_DESC_ERROR:
            case CHANNEL_SRC_STATUS_ERROR:
            case CHANNEL_DST_STATUS_ERROR:
            {
                if ( not r_channel_run[k] )	r_channel_fsm[k] = CHANNEL_IDLE;
                break; 
            }
        }
    } // end switch r_channel_fsm[k]
        
                
    ////////////////////////////////////////////////////////////////////////////
    // This CMD_FSM controls the VCI INIT command port
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

                    switch ( r_channel_vci_req_type[k].read() )
                    {
                        case REQ_READ_SRC_DESC:
                        {
                            r_cmd_address = r_channel_src_desc[k].read() + 
                                            (r_channel_src_index[k].read() << 3);
                            r_cmd_bytes   = 8;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_DST_DESC:
                        {
                            r_cmd_address = r_channel_dst_desc[k].read() + 
                                            (r_channel_dst_index[k].read() << 3);
                            r_cmd_bytes   = 8;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_SRC_STATUS:
                        {
                            r_cmd_address = ((uint64_t)r_channel_src_ext[k].read() << 32) +
                                            r_channel_src_sts_addr[k].read();
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_DST_STATUS:
                        {
                            r_cmd_address = ((uint64_t)r_channel_dst_ext[k].read() << 32) +
                                            r_channel_dst_sts_addr[k].read();
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_READ_DATA:
                        {
                            r_cmd_address = ((uint64_t)r_channel_src_ext[k].read() << 32) +
                                             r_channel_src_buf_addr[k].read();
                            r_cmd_bytes   = r_channel_bytes[k][r_channel_burst_id[k].read()].read();
                            r_cmd_fsm     = CMD_READ;
                            break;
                        }
                        case REQ_WRITE_DATA:
                        {
                            r_cmd_address = ((uint64_t)r_channel_dst_ext[k].read() << 32) +
                                             r_channel_dst_buf_addr[k].read();
                            r_cmd_bytes   = r_channel_bytes[k][r_channel_burst_id[k].read()].read();
                            r_cmd_fsm     = CMD_WRITE;
                            break;
                        }
                         case REQ_WRITE_SRC_STATUS:
                        {
                            r_cmd_address = ((uint64_t)r_channel_src_ext[k].read() << 32) +
                                            r_channel_src_sts_addr[k].read();
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_WRITE;
                            break;
                        }
                        case REQ_WRITE_DST_STATUS:
                        {
                            r_cmd_address = ((uint64_t)r_channel_dst_ext[k].read() << 32) +
                                            r_channel_dst_sts_addr[k].read();
                            r_cmd_bytes   = 4;
                            r_cmd_fsm     = CMD_WRITE;
                            break;
                        }
                    } // end switch
                } // end if
            }
            break;
        }
        //////////////
        case CMD_READ:
        {
            if ( p_vci_initiator.cmdack.read() )
            {
                uint32_t k = r_cmd_channel.read();
                r_channel_vci_req[k]= false;
                r_cmd_fsm = CMD_IDLE;
            }
            break;
        }
        ///////////////
        case CMD_WRITE:
        {
            if ( p_vci_initiator.cmdack.read() )
            {
                uint32_t k = r_cmd_channel.read();
                vci_fifo_get = r_channel_fifo[k].rok();
                vci_fifo_get_channel = k;

                if (vci_param::B==4)
                {
                    if ( r_cmd_count.read() == (r_cmd_bytes.read() - 4) )
                    {
                        r_channel_vci_req[k]= false;
                        r_cmd_fsm = CMD_IDLE;
                    }
                    r_cmd_count = r_cmd_count.read() + 4;
                }
                else
                {
                    if ( r_cmd_count.read() == (r_cmd_bytes.read() - 4) )
                    {
                        r_channel_vci_req[k]= false;
                        r_cmd_fsm = CMD_IDLE;
                        r_cmd_count = r_cmd_count.read() + 4;
                    }
                    else
                    {
                        if ( r_cmd_count.read() == (r_cmd_bytes.read() - 8))
                        {
                            r_channel_vci_req[k]= false;
                            r_cmd_fsm = CMD_IDLE; 
                        }
                        r_cmd_count = r_cmd_count.read() + 8;
                    }
                }
             }
            break;
        }
    } // end switch cmd_fsm

    ///////////////////////////////////////////////////////////////////////////
    // This RSP_FSM controls the VCI INIT response port
    // It set r_channel_vci_rsp[k], and set r_channel_vci_error[k]
    // to signal completion of the read / write VCI transaction.
    ///////////////////////////////////////////////////////////////////////////
    switch(r_rsp_fsm.read()) 
    {
        //////////////
        case RSP_IDLE:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                uint32_t k      = (uint32_t)p_vci_initiator.rtrdid.read()>>1;
                switch ( r_channel_vci_req_type[k].read() ) 
                {
                    case REQ_READ_SRC_DESC:
                        r_rsp_count[k] = 0;
                        r_rsp_fsm = RSP_READ_SRC_DESC;
                        break;
                    case REQ_READ_SRC_STATUS:
                        r_rsp_count[k] = 0;
                        r_rsp_fsm = RSP_READ_SRC_STATUS;
                        break;
                    case REQ_READ_DST_DESC:
                        r_rsp_count[k] = 0;
                        r_rsp_fsm = RSP_READ_DST_DESC;
                        break;
                    case REQ_READ_DST_STATUS:
                        r_rsp_count[k] = 0;
                        r_rsp_fsm = RSP_READ_DST_STATUS;
                        break;
                    case REQ_READ_DATA:
                    case REQ_WRITE_DATA:
                        r_rsp_count[k] = 0;
                        // The rtrdid LSB indicates the type of response (read or write)
                        if ( p_vci_initiator.rtrdid.read() & 0x1 )
                            r_rsp_fsm = RSP_WRITE;
                        else
                            r_rsp_fsm = RSP_READ_DATA; 
                        break;
                    case REQ_WRITE_SRC_STATUS:
                    case REQ_WRITE_DST_STATUS:
                        r_rsp_count[k] = 0;
                        r_rsp_fsm = RSP_WRITE;
                        break;
                } // end switch
                r_rsp_channel = k;
            }
            break;
        } 
        ///////////////////////
        case RSP_READ_SRC_DESC:  // set SRC status and buffer address
        {
            if ( p_vci_initiator.rspval.read() )
            {
                uint32_t k    = r_rsp_channel.read();
                if (vci_param::B==4)
                {
                    uint32_t rdata = (uint32_t)p_vci_initiator.rdata.read();

                    if ( r_rsp_count[k].read() == 0 ) // read bits[31:0] of buffer descriptor
                    {
                        // bits[31:26] of buffer descriptor are bits[11:6] of buffer address
                        r_channel_src_buf_addr[k] = (rdata & 0xFC000000) << 6;

                        // bits[25:0] of buffer descriptor are bits[31:6] of buffer status address
                        r_channel_src_sts_addr[k] = (rdata & 0x3FFFFFF) << 6;
                    }

                    else // read bits[63:32] of buffer descriptor
                    {
                        // bits[63:52] of buffer descriptor (<=> bits [31:24] of rdata) are
                        // address extension of buffer address and buffer status address
                        r_channel_src_ext[k] = (rdata & 0xFFF00000) >> 20;

                        // bits[51:32] of buffer descriptor (<=> bits [19:0] of rdata) are
                        // bits[31:12] of buffer address
                        r_channel_src_buf_addr[k] = (r_channel_src_buf_addr[k].read() & 0xFFF) +
                                                    ((rdata & 0xFFFFF) << 12);
                    }

                    r_rsp_count[k] = r_rsp_count[k].read() + 4;
                }

                else // read bits[63:0] of buffer descriptor
                {
                    uint64_t rdata = (uint64_t)p_vci_initiator.rdata.read();

                    // bits[63:52] of buffer descriptor are address extension of buffer address
                    // and buffer status address
                    r_channel_src_ext[k] = (rdata & 0xFFF0000000000000ULL) >> 52;

                    // bits[51:26] of buffer descriptor are bits[31:6] of buffer address
                    r_channel_src_buf_addr[k] = (rdata & 0xFFFFFFC000000ULL) >> 20;

                    // bits[25:0] of buffer descriptor are bits[31:6] of buffer status address
                    r_channel_src_sts_addr[k] = (rdata & 0x3FFFFFFULL) << 6;
                
                    r_rsp_count[k] = r_rsp_count[k].read() + 8;
                }

                if ( p_vci_initiator.reop.read() )
                {
                    r_channel_vci_rsp[k]    = true;
                    r_channel_vci_error[k] = ((p_vci_initiator.rerror.read()&0x1) != 0);
                    r_rsp_fsm = RSP_IDLE;
                } 
            }
            break;
        } 
        /////////////////////////
        case RSP_READ_SRC_STATUS:   // set SRC status
        {
            uint32_t k              = r_rsp_channel.read();
            uint32_t rdata          = (uint32_t)p_vci_initiator.rdata.read();
            r_channel_src_full[k]   = ((rdata & 0x1) != 0);
            r_channel_vci_rsp[k]    = true;
            r_channel_vci_error[k]  = ((p_vci_initiator.rerror.read()&0x1) != 0);
            r_rsp_fsm               = RSP_IDLE;
            break;
        }
        ///////////////////////
        case RSP_READ_DST_DESC:  // set DST status and buffer address
        {
            if ( p_vci_initiator.rspval.read() )
            {
                uint32_t k    = r_rsp_channel.read();
                if(vci_param::B==4)
                {
                    uint32_t rdata = (uint32_t)p_vci_initiator.rdata.read();

                    if ( r_rsp_count[k].read() == 0 ) // read bits[31:0] of buffer descriptor
                    {
                        // bits[31:26] of buffer descriptor are bits[11:6] of buffer address
                        r_channel_dst_buf_addr[k] = (rdata & 0xFC000000) << 6;

                        // bits[25:0] of buffer descriptor are bits[31:6] of buffer status address
                        r_channel_dst_sts_addr[k] = (rdata & 0x3FFFFFF) << 6;
                    }

                    else // read bits[63:32] of buffer descriptor
                    {
                        // bits[63:52] of buffer descriptor (<=> bits [31:24] of rdata) are
                        // address extension of buffer address and buffer status address
                        r_channel_dst_ext[k] = (rdata & 0xFFF00000) >> 20;

                        // bits[51:32] of buffer descriptor (<=> bits [19:0] of rdata) are
                        // bits[31:12] of buffer address
                        r_channel_dst_buf_addr[k] = (r_channel_dst_buf_addr[k].read() & 0xFFF) +
                                                    ((rdata & 0xFFFFF) << 12);
                    }

                    r_rsp_count[k] = r_rsp_count[k].read() + 4;
                }

                else // read bits[63:0] of buffer descriptor
                {
                    uint64_t rdata = (uint64_t)p_vci_initiator.rdata.read();

                    // bits[63:52] of buffer descriptor are address extension of buffer address
                    // and buffer status address
                    r_channel_dst_ext[k] = (rdata & 0xFFF0000000000000ULL) >> 52;

                    // bits[51:26] of buffer descriptor are bits[31:6] of buffer address
                    r_channel_dst_buf_addr[k] = (rdata & 0xFFFFFFC000000ULL) >> 20;

                    // bits[25:0] of buffer descriptor are bits[31:6] of buffer status address
                    r_channel_dst_sts_addr[k] = (rdata & 0x3FFFFFFULL) << 6;
                
                    r_rsp_count[k] = r_rsp_count[k].read() + 8;
                }

                if ( p_vci_initiator.reop.read() )
                {
                    r_channel_vci_rsp[k]    = true;
                    r_channel_vci_error[k] = ((p_vci_initiator.rerror.read()&0x1) != 0);
                    r_rsp_fsm = RSP_IDLE;
                } 
            }
            break;
        } 
        /////////////////////////
        case RSP_READ_DST_STATUS:  // set DST status
        {
            uint32_t k              = r_rsp_channel.read();
            uint32_t rdata          = (uint32_t)p_vci_initiator.rdata.read();
            r_channel_dst_full[k]   = ((rdata & 0x1) != 0);
            r_channel_vci_rsp[k]    = true;
            r_channel_vci_error[k]  = ((p_vci_initiator.rerror.read()&0x1) != 0);
            r_rsp_fsm               = RSP_IDLE;
            break;
        }
        ///////////////////
        case RSP_READ_DATA:
        {
            if ( p_vci_initiator.rspval.read() )
            {
                uint32_t k    = r_rsp_channel.read();
                
                // if this is the first flit of response of the next expected read burst
                // change the value of the r_channel_rsp_read
                if ( !(r_channel_rsp_read[k][r_rsp_next_read[k].read()].read()) )
                    r_channel_rsp_read[k][r_rsp_next_read[k].read()] = true;
                                    
                r_channel_vci_error[k] = r_channel_vci_error[k].read() or
                                         ((p_vci_initiator.rerror.read()&0x1) != 0);
                
                if ( r_channel_fifo[k].wok() )
                {
                    vci_fifo_put   = true;
                    vci_fifo_put_channel = k;
                    vci_fifo_wdata = p_vci_initiator.rdata.read();
                
                    if ( vci_param::B == 4 ) r_rsp_count[k] = r_rsp_count[k].read() + 4;
                    else r_rsp_count[k] = r_rsp_count[k].read() + 8;
                   
                    if ( p_vci_initiator.reop.read() )
                    {
                        assert( (r_rsp_count[k].read() < m_burst_max_length) and
                                "VCI_CHBUF_DMA error : wrong number of flits for a read response");

                        // increment the next expected read burst id
                        if ( r_rsp_next_read[k].read() == m_pipelined_bursts - 1 )
                            r_rsp_next_read[k] = 0;
                        else
                            r_rsp_next_read[k] = r_rsp_next_read[k].read() + 1;

                        r_rsp_fsm = RSP_IDLE;
                    }
                }
            }
            break; 
        }
        ///////////////
        case RSP_WRITE:
        {
            assert( (p_vci_initiator.reop.read() == true) and
             "VCI_CHBUF_DMA error : write response packed contains more than one flit");  

            uint32_t k = r_rsp_channel.read();

            // if internal loop
            if (  r_channel_vci_req_type[k].read() == REQ_READ_DATA or
                  r_channel_vci_req_type[k].read() == REQ_WRITE_DATA )
            {
                r_channel_rsp_write[k][r_rsp_next_write[k].read()] = true;
                // increment the next expected write burst id
                if ( r_rsp_next_write[k].read() == m_pipelined_bursts - 1 )
                    r_rsp_next_write[k] = 0;
                else
                    r_rsp_next_write[k] = r_rsp_next_write[k].read() + 1;
            }
            else // external loop
                r_channel_vci_rsp[k] = true;
           
            r_channel_vci_error[k] = r_channel_vci_error[k].read() or
                                     ((p_vci_initiator.rerror.read()&0x1) != 0);
            r_rsp_fsm              = RSP_IDLE;
            break;
        } 
    } // end switch rsp_fsm

    ////////////////////////////////////////////////////////////////////////////////////////
    // update even and odd fifo
    // - the get command can be true only for the r_cmd_channel (in the CMD_WRITE state)
    // - the put command can be true only for the r_rsp_channel (in the RSP_READ_DATA state)
    ////////////////////////////////////////////////////////////////////////////////////////
    if ( vci_fifo_get and vci_fifo_put )
    {
        if (vci_fifo_get_channel == vci_fifo_put_channel )
            r_channel_fifo[vci_fifo_get_channel].update( true,
                                                          true,
                                                          vci_fifo_wdata );
        else
        {
            r_channel_fifo[vci_fifo_get_channel].update( true,
                                                          false,
                                                          0 );
            r_channel_fifo[vci_fifo_put_channel].update( false,
                                                          true,
                                                          vci_fifo_wdata );
        }
    }
    else if ( vci_fifo_get )
        r_channel_fifo[vci_fifo_get_channel].update( vci_fifo_get,
                                                      vci_fifo_put,
                                                      vci_fifo_wdata );
    else if ( vci_fifo_put )
        r_channel_fifo[vci_fifo_put_channel].update( vci_fifo_get,
                                                      vci_fifo_put,
                                                      vci_fifo_wdata );
} // end transition

//////////////////////
tmpl(void)::genMoore()
{
    /////// VCI INIT CMD ports //////
    // The last bit of trdid is equal to 1 for write command
    // and 0 for read command
    switch( r_cmd_fsm.read() ) 
    {
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
            uint32_t k    = r_cmd_channel.read();
            typename vci_param::be_t r_be;
            if(vci_param::B == 4) r_be  = 0xF;
            else 
            {
                if       ( (r_channel_vci_req_type[k] == REQ_READ_SRC_STATUS) or 
                           (r_channel_vci_req_type[k] == REQ_READ_DST_STATUS)  ) r_be = 0x0F;
                else  if ( r_cmd_bytes.read() - r_cmd_count.read() == 4)     r_be = 0x0F;
                else                                                         r_be = 0xFF;
            }    
            p_vci_initiator.cmdval  = true;
            p_vci_initiator.address = (typename vci_param::fast_addr_t)r_cmd_address.read();
            p_vci_initiator.wdata   = 0;
            p_vci_initiator.be      = r_be;      
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

            if (vci_param::B ==4)     // Data width 32 bits
            {  
                uint32_t wdata;
                bool cmdval;

                if      ( r_channel_vci_req_type[k] == REQ_WRITE_SRC_STATUS ) 
                {
                    cmdval = true;
                    wdata = 0x0;
                }
                else if ( r_channel_vci_req_type[k] == REQ_WRITE_DST_STATUS ) 
                {
                    cmdval = true;
                    wdata = 0x1;
                }
                else   // REQ_WRITE_DATA
                {
                    if ( r_channel_fifo[k].rok() )
                    {
                        cmdval = true;
                        wdata = r_channel_fifo[k].read();
                    }
                    else
                    {
                        cmdval = false;
                        wdata = 0;
                    }
                }

                p_vci_initiator.cmdval  = cmdval;
                p_vci_initiator.address = (typename vci_param::fast_addr_t)r_cmd_address.read() + 
                                          r_cmd_count.read();
                p_vci_initiator.wdata   = wdata;
                p_vci_initiator.be      = 0xF;
                p_vci_initiator.plen    = r_cmd_bytes.read();
                p_vci_initiator.cmd     = vci_param::CMD_WRITE;
                p_vci_initiator.trdid   = (r_cmd_channel.read()<<1) + 0x1;	
                p_vci_initiator.pktid   = 0x4;
                p_vci_initiator.srcid   = m_srcid;
                p_vci_initiator.cons    = false;
                p_vci_initiator.wrap    = false;
                p_vci_initiator.contig  = true;
                p_vci_initiator.clen    = 0;
                p_vci_initiator.cfixed  = false;
                p_vci_initiator.eop     = ( r_cmd_count.read() == r_cmd_bytes.read() - 4 );
                
            }
            else          // Data width 64 bits
            {
                bool cmdval;
                uint64_t wdata;
                typename vci_param::be_t be;

                if       ( r_channel_vci_req_type[k] == REQ_WRITE_SRC_STATUS )
                {   
                    cmdval = true;
                    be     = 0x0F;
                    wdata  = 0x0;
                }
                else if ( r_channel_vci_req_type[k] == REQ_WRITE_DST_STATUS ) 
                {   
                    cmdval = true;
                    be     = 0x0F;
                    wdata  = 0x1;
                }
                else  // REQ_WRITE_DATA
                {
                    if ( r_channel_fifo[k].rok() )
                    {
                        cmdval = true;
                        if ( r_cmd_bytes.read() - r_cmd_count.read() == 4)
                        {
                            be = 0x0F;
                            wdata  = (uint32_t)r_channel_fifo[k].read();
                        }
                        else
                        {
                        be     = 0xFF;
                        wdata  = r_channel_fifo[k].read();
                        }
                    }
                    else
                    {
                        cmdval = false;
                        be     = 0;
                        wdata  = 0;
                    }
                }
                
                p_vci_initiator.cmdval  = cmdval;
                p_vci_initiator.address = (typename vci_param::fast_addr_t)r_cmd_address.read() 
                                           + r_cmd_count.read();
                p_vci_initiator.wdata   = wdata ;
                p_vci_initiator.be      = be;
                p_vci_initiator.plen    = r_cmd_bytes.read();
                p_vci_initiator.cmd     = vci_param::CMD_WRITE;
                p_vci_initiator.trdid   = (r_cmd_channel.read()<<1) + 0x1;	
                p_vci_initiator.pktid   = 0x4;
                p_vci_initiator.srcid   = m_srcid;
                p_vci_initiator.cons    = false;
                p_vci_initiator.wrap    = false;
                p_vci_initiator.contig  = true;
                p_vci_initiator.clen    = 0;
                p_vci_initiator.cfixed  = false;
                p_vci_initiator.eop     = (( r_cmd_count.read() == r_cmd_bytes.read() - 4 ) 
                                         || ( r_cmd_count.read() == r_cmd_bytes.read() - 8 ));
            break;
           
            }
        }
    } // end switch cmd_fsm

    /////// VCI INIT RSP port ////// 
    if ( r_rsp_fsm.read() == RSP_IDLE )
        p_vci_initiator.rspack = false;
    else if ( r_rsp_fsm.read() == RSP_READ_DATA )
        p_vci_initiator.rspack = ( r_channel_fifo[r_rsp_channel.read()].wok() );
    else
        p_vci_initiator.rspack = true;

    ////// VCI TARGET port /////// 
    switch( r_tgt_fsm.read() ) {
        case TGT_IDLE:
        {
            p_vci_target.cmdack = true;
            p_vci_target.rspval = false;
            p_vci_target.reop   = false;
            p_vci_target.rdata  = 0;
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
        default:
        {
            assert(false);
        }
    } // end switch rsp_fsm

    /////// IRQ ports //////////
    for ( uint32_t k = 0 ; k < m_channels ; k++ )
    {
	    p_irq[k] = (r_channel_fsm[k] == CHANNEL_SRC_DESC_ERROR)   || 
                   (r_channel_fsm[k] == CHANNEL_DST_DESC_ERROR)   ||
                   (r_channel_fsm[k] == CHANNEL_SRC_STATUS_ERROR) ||
                   (r_channel_fsm[k] == CHANNEL_DST_STATUS_ERROR) ||
                   (r_channel_fsm[k] == CHANNEL_DATA_ERROR);
    }
} // end genMoore

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
        "  RSP_READ_SRC_DESC",
        "  RSP_READ_SRC_STATUS",
        "  RSP_READ_DST_DESC",
        "  RSP_READ_DST_STATUS",
        "  RSP_READ_DATA",
        "  RSP_WRITE"
    };
    const char* channel_state_str[] = 
    {
        " CHANNEL_IDLE",

        " CHANNEL_DATA_ERROR",
        " CHANNEL_SRC_DESC_ERROR",
        " CHANNEL_DST_DESC_ERROR",
        " CHANNEL_SRC_STATUS_ERROR",
        " CHANNEL_DST_STATUS_ERROR",

        " CHANNEL_READ_SRC_DESC",
        " CHANNEL_READ_SRC_DESC_WAIT",
        " CHANNEL_READ_SRC_STATUS",
        " CHANNEL_READ_SRC_STATUS_WAIT",
        " CHANNEL_READ_SRC_STATUS_DELAY",

        " CHANNEL_READ_DST_DESC",
        " CHANNEL_READ_DST_DESC_WAIT",
        " CHANNEL_READ_DST_STATUS",
        " CHANNEL_READ_DST_STATUS_WAIT",
        " CHANNEL_READ_DST_STATUS_DELAY",

        " CHANNEL_BURST",
        " CHANNEL_READ_REQ",
        " CHANNEL_READ_REQ_WAIT",
        " CHANNEL_RSP_WAIT",
        " CHANNEL_WRITE_REQ",
        " CHANNEL_WRITE_REQ_WAIT",
        " CHANNEL_BURST_RSP_WAIT",

        " CHANNEL_SRC_STATUS_WRITE",
        " CHANNEL_SRC_STATUS_WRITE_WAIT",
        " CHANNEL_DST_STATUS_WRITE",
        " CHANNEL_DST_STATUS_WRITE_WAIT",
        " CHANNEL_NEXT_BUFFERS",
    };

    std::cout << "CHBUF_DMA " << name() << " : " 
              << tgt_state_str[r_tgt_fsm.read()] << std::endl;
    for ( uint32_t k = 0 ; k < m_channels ; k++ )
    {
        
        if ( r_channel_run[k].read() )
        {
            std::cout << "  CHANNEL[" << std::dec << k << "] :"
                      << channel_state_str[r_channel_fsm[k].read()]
                      << " / src_buf_addr = " << std::hex << ( (uint64_t)r_channel_src_ext[k].read() << 32 ) +
                                                             r_channel_src_buf_addr[k].read()
                      << " / src_sts_addr = " << std::hex << ( (uint64_t)r_channel_src_ext[k].read() << 32 ) +
                                                             r_channel_src_sts_addr[k].read()
                      << " / src_buf_id = " << std::dec << r_channel_src_index[k].read()
                      << " / dst_buf_addr = " << std::hex << ( (uint64_t)r_channel_dst_ext[k].read() << 32 ) +
                                                             r_channel_dst_buf_addr[k].read()
                      << " / dst_sts_addr = " << std::hex << ( (uint64_t)r_channel_dst_ext[k].read() << 32 ) +
                                                             r_channel_dst_sts_addr[k].read()
                      << " / dst_buf_id = " << std::dec << r_channel_dst_index[k].read()
                      << std::endl
                      << "            todo_bytes = " << r_channel_todo_bytes[k].read()
                      << " / burst_id = " << std::dec << r_channel_burst_id[k].read()
                      << " / period = " << std::dec << r_channel_period[k].read()
                      << " / timer = " << r_channel_timer[k].read()
                      << std::endl
                      << std::endl;
               
            //r_channel_fifo[k]->print();
        }
        
    }
    std::cout << cmd_state_str[r_cmd_fsm.read()] << std::dec 
              << " / channel = " << r_cmd_channel.read()
              << " / length = " << r_cmd_bytes.read()
              << " / count = " << r_cmd_count.read()/4 << std::endl;
    std::cout << rsp_state_str[r_rsp_fsm.read()] << std::dec 
              << " / channel = " << r_rsp_channel.read()
              << " / length = " << r_rsp_bytes.read()
              << " / count = " << r_rsp_count[r_rsp_channel.read()].read()/4 << std::endl;
}

////////////////////////////////////////////////////////////////////
tmpl(/**/)::VciChbufDma( sc_core::sc_module_name 		        name,
                         const soclib::common::MappingTable 	&mt,
                         const soclib::common::IntTab 		    &srcid,
                         const soclib::common::IntTab 		    &tgtid,
	                     const uint32_t 				        burst_max_length,
                         const uint32_t 				        channels,
                         const uint32_t                         pipelined_bursts )
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

          r_channel_src_desc(soclib::common::alloc_elems<sc_signal<uint64_t> >
                    ("r_channel_src_desc", channels)),
          r_channel_src_nbufs(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_nbufs", channels)),
          r_channel_src_buf_addr(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_buf_addr", channels)),
          r_channel_src_sts_addr(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_sts_addr", channels)),
          r_channel_src_ext(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_ext", channels)),
          r_channel_src_index(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_src_index", channels)),
          r_channel_src_full(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_src_full", channels)),

          r_channel_dst_desc(soclib::common::alloc_elems<sc_signal<uint64_t> >
                    ("r_channel_dst_desc", channels)),
          r_channel_dst_nbufs(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_nbufs", channels)),
          r_channel_dst_buf_addr(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_buf_addr", channels)),
          r_channel_dst_sts_addr(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_sts_addr", channels)),
          r_channel_dst_ext(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_ext", channels)),
          r_channel_dst_index(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_dst_index", channels)),
          r_channel_dst_full(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_dst_full", channels)),

          r_channel_timer(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_timer", channels)),
          r_channel_period(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_period", channels)),
          r_channel_todo_bytes(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_todo_bytes", channels)),
          r_channel_bytes(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_bytes", channels, pipelined_bursts)),
          r_channel_burst_id(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_channel_burst_id", channels)),
          r_channel_vci_req(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_vci_req", channels)),
          r_channel_vci_req_type(soclib::common::alloc_elems<sc_signal<int> >
                    ("r_channel_vci_req_type", channels)),
          r_channel_vci_rsp(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_vci_rsp", channels)),
          r_channel_rsp_read(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_rsp_read", channels, pipelined_bursts)),
          r_channel_rsp_write(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_rsp_write", channels, pipelined_bursts)),
          r_channel_vci_error(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_vci_error", channels)),
          r_channel_last(soclib::common::alloc_elems<sc_signal<bool> >
                    ("r_channel_last", channels)),
                      
          r_cmd_fsm("r_cmd_fsm"),
          r_cmd_count("r_cmd_count"),
          r_cmd_address("r_cmd_address"),
          r_cmd_channel("r_cmd_channel"),
          r_cmd_bytes("r_cmd_bytes"),

          r_rsp_fsm("r_rsp_fsm"),
          r_rsp_count(soclib::common::alloc_elems<sc_signal<size_t> >
                    ("r_rsp_count", channels)),
          r_rsp_channel("r_rsp_channel"),
          r_rsp_bytes("r_rsp_bytes"),
          r_rsp_next_read(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_rsp_next_read", channels)), 
          r_rsp_next_write(soclib::common::alloc_elems<sc_signal<uint32_t> >
                    ("r_rsp_next_write", channels)), 

          m_seglist(mt.getSegmentList(tgtid)),
          m_burst_max_length(burst_max_length),
          m_pipelined_bursts(pipelined_bursts),
          m_channels(channels),
          m_srcid(mt.indexForId(srcid)),

          p_clk("p_clk"),
          p_resetn("p_resetn"),
          p_vci_target("p_vci_target"),
          p_vci_initiator("p_vci_initiator"),
          p_irq(soclib::common::alloc_elems<sc_core::sc_out<bool> >("p_irq", channels))
{
    std::cout << "  - Building VciChbufDma : " << name << std::endl;

    r_channel_fifo = (GenericFifo<uint64_t>*)
        malloc(sizeof(GenericFifo<uint64_t>) * m_channels);

    for( size_t i = 0 ; i < m_channels ; i++ )
    {
        std::ostringstream str;
        str << "r_channel_fifo_" << i;
        new(&r_channel_fifo[i])  
            GenericFifo<uint64_t>(str.str(), m_burst_max_length / 8 * m_pipelined_bursts);
    }

    assert( (m_seglist.empty() == false) and 
    "VCI_CHBUF_DMA error : No segment allocated");

    std::list<soclib::common::Segment>::iterator seg;
    for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ ) 
    {
	    assert( ( (seg->baseAddress() & 0xFFF) == 0 ) and 
		"VCI_CHBUF_DMA Error : The segment base address must be multiple of 4 Kbytes"); 

	    assert( ( seg->size() >= (m_channels<<12) ) and 
		"VCI_CHBUF_DMA Error : The segment size cannot be smaller than 4K * channels"); 

        std::cout << "    => segmesnt " << seg->name()
                  << " / base = " << std::hex << seg->baseAddress()
                  << " / size = " << seg->size() << std::endl; 
    }

    assert( (vci_param::T >= 4) and 
    "VCI_CHBUF_DMA error : The VCI TRDID field must be at least 4 bits");

    assert( ((vci_param::B == 4) or ( vci_param::B == 8 ) ) and 
    "VCI_CHBUF_DMA error : The VCI DATA field must be 32 or 64 bits");

    assert( (burst_max_length < (1<<vci_param::K)) and 
    "VCI_CHBUF_DMA error : The VCI PLEN size is too small for requested burst length");

    assert( (((burst_max_length==4)or(burst_max_length==8)or(burst_max_length==16)or 
             (burst_max_length==32)or(burst_max_length==64))) and
    "VCI_CHBUF_DMA error : The burst length must be 4, 8, 16, 32, 64 bytes");

    assert( (pipelined_bursts <= 4) and
            "VCI_CHBUF_DMA error : The number of pipelined bursts cannot be larger than 4");
    
    assert( (channels <= 8)  and
    "VCI_CHBUF_DMA error : The number of channels cannot be larger than 8");

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();
}


tmpl(/**/)::~VciChbufDma() {
    soclib::common::dealloc_elems<sc_signal<int> >(r_channel_fsm, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_run, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_buf_size, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint64_t> >(r_channel_src_desc, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_src_nbufs, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_src_buf_addr, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_src_sts_addr, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_src_ext, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_src_index, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_src_full, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint64_t> >(r_channel_dst_desc, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_dst_nbufs, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_dst_buf_addr, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_dst_sts_addr, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_dst_ext, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_dst_index, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_dst_full, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_timer, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_period, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_todo_bytes, m_channels);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_bytes, m_channels, m_pipelined_bursts);
    soclib::common::dealloc_elems<sc_signal<uint32_t> >(r_channel_burst_id, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_vci_req, m_channels);
    soclib::common::dealloc_elems<sc_signal<int> >(r_channel_vci_req_type, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_vci_rsp, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_rsp_read, m_channels, m_pipelined_bursts);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_rsp_write, m_channels, m_pipelined_bursts);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_vci_error, m_channels);
    soclib::common::dealloc_elems<sc_signal<bool> >(r_channel_last, m_channels);
    delete r_channel_fifo;
    soclib::common::dealloc_elems<sc_core::sc_out<bool> >(p_irq, m_channels);
}



}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

