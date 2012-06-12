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
 *         Alain Greiner <alain.greiner@lip6.fr> Juin 2012
 */

//////////////////////////////////////////////////////////////////////////////////
//  This component is a multi-channels, GMII compliant, NIC controller.
//  This component makes the assumption that the VCI RDATA & WDATA fields
//  have 32 bits. The number of channels is a constructor parameter  
//  and cannot be larger than 8.
//
//  Thic component has no DMA capability: All data transfers mut be performed
//  by software, or by an external DMA engine.
//  The data transfer unit between software and the NIC is a container. 
//  A container is a 4K bytes buffer, containing an integer number 
//  of ariable size packets. Packet length is between 64 to 1538 bytes.
//
//  The first 32 words of a container are the container descriptor:
//      word0:       | NB_WORDS          | NB_PACKETS        |
//      word1:       | PLEN[1]           | PLEN[0]           |
//                   | ...               | ...               |
//      word31:      | PLEN[61]          | PLEN[60]          |
//
// There is at most 62 packets in a container.
// - NB_PACKETS is the actual number of packets in the container
// - NB_WORDS is the number of useful words in the container
// - PLEN[i] is the number of bytes for packet [i].  
//
// The packets are stored in the (1024-32) following words,
// and the packets are word-aligned.
//
//  In order to support various protection mechanisms, each channel takes
//  a segment of 8 Kbytes in the address space:
//  - The first 4K bytes contain the configuration and status registers
//  - The second 4K bytes define the current container. 
//
//  There is two IRQ lines for each channel:
//  - RX_IRQ[k] is activated as soon as ther is at least one RX_container 
//    containing data for channel (k).
//  - TX_IRQ[k] is activated as soon a there is at least one TX_container
//    empty for chnnel (k).
/////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <cassert>

#include "alloc_elems.h"
#include "../include/vci_multi_nic.h"
#include "../../../include/soclib/multi_nic.h"

namespace soclib { namespace caba {

#define tmpl(t) template<typename vci_param> t VciMultiNic<vci_param>

/////////////////////////
tmpl(void)::transition()
{
    if (!p_resetn) 
    {
        r_vci_fsm          = VCI_IDLE;
        r_vci_ptr          = 0;
        r_vci_ptw          = 0;

        r_rx_g2s_fsm       = RX_G2S_IDLE;
        r_rx_des_fsm       = RX_DES_READ_FIRST;
        r_rx_dispatch_fsm  = RX_DISPATCH_IDLE;
        r_tx_dispatch_fsm  = TX_DISPATCH_IDLE;
        r_tx_s2g_fsm       = TX_S2G_IDLE;

        r_rx_fifo_stream.init();
        r_rx_fifo_multi.reset();
        r_tx_fifo_stream.init();
        r_bp_fifo_multi.reset();

        r_gmii_rx.reset();
        r_gmii_tx.reset();

        for ( size_t k = 0 ; k < m_channels ; k++ )
        {
            r_rx_channel[k]->reset();
            r_tx_channel[k]->reset();
        }
        return;
    }

    ///////////////////////////////////////////////////////////////////////////
    // This VCI_FSM controls the VCI TARGET port
    // It is inspired from the model of the vci_simple_ram.
    // The VCI PLEN field must be multiple of 4, and the BE field is not used.
    // We acknowledge the VCI command, and decod it in the IDLE state. 
    // - All configuration (write) and status (read) accesses must be one flit.
    // - The read data transfers (from RX channel container), or the 
    //   write data transfers (to TX channel container) can be split 
    //   into several bursts, at contiguous addresses, as the container
    //   behave as FIFOs.
    ///////////////////////////////////////////////////////////////////////////

    // default values for rx_channel and tx_channel commands
    rx_channel_rcmd_t rx_channel_rcmd  = RX_CHANNEL_RCMD_NOP;
    tx_channel_wcmd_t tx_channel_wcmd  = TX_CHANNEL_WCMD_NOP;
    uint32_t          tx_channel_wdata = 0;

    switch(r_vci_fsm.read()) 
    {
        //////////////
        case VCI_IDLE:  // decode the VCI command
        {
            if (p_vci.cmdval.read() )
            {
                typename vci_param::addr_t	address = p_vci.address.read();
                typename vci_param::cmd_t	cmd     = p_vci.cmd.read();
                typename vci_param::plen_t  plen    = p_vci.plen.read();
               
                assert ( ((plen & 0x3) == 0) and
                "ERROR in VCI_MULTI_NIC : PLEN field must be multiple of 4 bytes");

                assert ( (m_segment.contains(address)) and
                "ERROR in VCI_MULTI_NIC : ADDRESS is out of segment");

                r_vci_srcid	   = p_vci.srcid.read();
                r_vci_trdid	   = p_vci.trdid.read();
                r_vci_pktid	   = p_vci.pktid.read();
                r_vci_wdata    = p_vci.wdata.read();

                size_t channel = (size_t)((address & 0x0000E000) >> 13);
                size_t cell    = (size_t)((address & 0x00000FFF) >> 2);
                bool   burst   = (address & 0x00001000);

                r_vci_channel  = channel;

                assert( (channel < m_channels) and 
                "VCI_MULTI_NIC error : The channel index (ADDR[15:13] is too large");

	            if ( burst and (cmd == vci_param::CMD_WRITE) )      // TX_BURST transfer
                {
                    if ( p_vci.eop.read() ) r_vci_fsm = VCI_WRITE_TX_BURST;
                    else                           r_vci_fsm = VCI_WRITE_TX_BURST;
                }
                else if ( burst  and (cmd == vci_param::CMD_READ) ) // RX_BURST transfer
                {
                    r_vci_nwords  = (size_t)(plen >> 2);
                    r_vci_fsm     = VCI_READ_RX_BURST;
                }
                else if ( not burst and (cmd == vci_param::CMD_READ) and
                          (cell == NIC_TX_WOK ) )           // TX_WOK read access
                {
                    assert( p_vci.eop.read() and
                    "ERROR in VCI_MULTI_NIC : TX_WOK read access must be one flit");
                    r_vci_fsm = VCI_READ_TX_WOK;
                }
                else if ( not burst and (cmd == vci_param::CMD_READ) and
                          (cell == NIC_RX_ROK ) )           // RX_ROK read access
                {
                    assert( p_vci.eop.read() and
                    "ERROR in VCI_MULTI_NIC : RX_ROK read access must be one flit");
                    r_vci_fsm = VCI_READ_RX_ROK;
                }
                else if ( not burst and (cmd == vci_param::CMD_WRITE) and
                          (cell == NIC_TX_CLOSE) )          // TX_CLOSE write access
                {
                    assert( p_vci.eop.read() and
                    "ERROR in VCI_MULTI_NIC : TX_CLOSE write access must be one flit");
                    r_vci_fsm = VCI_WRITE_TX_CLOSE;
                }
                else if ( not burst and (cmd == vci_param::CMD_WRITE) and
                          (cell == NIC_RX_RELEASE) )         // RX_RELEASE write access
                {
                    assert( p_vci.eop.read() and
                    "ERROR in VCI_MULTI_NIC : RX_RELEASE write access must be one flit");
                    r_vci_fsm = VCI_WRITE_RX_RELEASE;
                }
                else if ( not burst and (cmd == vci_param::CMD_WRITE) and
                          (cell == NIC_MAC_4) )             // MAC_4 write access
                {
                    assert( p_vci.eop.read() and
                    "ERROR in VCI_MULTI_NIC : MAC_4 write access must be one flit");
                    r_vci_fsm = VCI_WRITE_MAC_4;
                }
                else if ( not burst and (cmd == vci_param::CMD_WRITE) and
                          (cell == NIC_MAC_2) )             // MAC_2 write access
                {
                    assert( p_vci.eop.read() and
                    "ERROR in VCI_MULTI_NIC : MAC_2 write access must be one flit");
                    r_vci_fsm = VCI_WRITE_MAC_2;
                }
                else
                {
                    assert( false and
                    "ERROR in VCI_MULTI_NIC : illegal VCI command");
                }
            }
            break;
        }
        ///////////////////////
        case VCI_WRITE_TX_BURST: // write data[i-1] in tx_channel[k]
                                 // and check if data[i] matches write pointer 
        {
            size_t channel = r_vci_channel.read();

            assert ( r_tx_channel[channel]->wok() and
            "ERROR in VCI_MULTI_NIC : tx_channel should not be full in VCI_WRITE_TX_BURST");
            
            if ( p_vci.cmdval.read() )
            {
                // data[i-1] 
                tx_channel_wcmd  = TX_CHANNEL_WCMD_WRITE;
                tx_channel_wdata = r_vci_wdata.read();

                // data[i]
                typename vci_param::addr_t	address = p_vci.address.read();

                assert( (((address & 0x00000FFF) >> 2) == r_vci_ptw.read()) and
                "ERROR in VCI_MULTI_NIC : address must be contiguous in VCI_WRITE_TX_BURST");
   
                r_vci_wdata      = p_vci.wdata.read();
                r_vci_ptw        = r_vci_ptw.read() + 1;
                if ( p_vci.eop.read() ) r_vci_fsm = VCI_WRITE_TX_LAST;
            }
            break;
        }
        ///////////////////////
        case VCI_WRITE_TX_LAST: // write last word of the burst in tx_channel
                                // and send VCI write response
        {
            if ( p_vci.rspack.read() )
            {
                tx_channel_wcmd  = TX_CHANNEL_WCMD_WRITE;
                tx_channel_wdata = r_vci_wdata.read();
                r_vci_fsm = VCI_IDLE;
            }
            break;
        }
        /////////////////////
        case VCI_READ_TX_WOK:   // send tx_channnel WOK in VCI response
        {
            if ( p_vci.rspack.read() )
            {
                r_vci_fsm = VCI_IDLE;
            }
            break;
        }
        ////////////////////////
        case VCI_WRITE_TX_CLOSE: // close tx_channel, reset r_vci_ptw pointer
                                 // and send VCI write response
        {
            if ( p_vci.rspack.read() )
            {
                tx_channel_wcmd  = TX_CHANNEL_WCMD_CLOSE;
                r_vci_ptw        = 0;
                r_vci_fsm        = VCI_IDLE;
            }
            break;
        }
        ///////////////////////
        case VCI_READ_RX_BURST: // check pointer and send data in VCI response
        {
            size_t channel = r_vci_channel.read();

            assert ( r_rx_channel[channel]->rok() and
            "ERROR in VCI_MULTI_NIC : rx_channel should not be empty in VCI_READ_RX_BURST");
            
            if ( p_vci.rspack.read() )
            {
                typename vci_param::addr_t	address = p_vci.address.read();

                assert( (((address & 0x00000FFF) >> 2) == r_vci_ptr.read()) and
                "ERROR in VCI_MULTI_NIC : address must be contiguous in VCI_WRITE_TX_BURST");

                r_vci_nwords     = r_vci_nwords.read() - 1;
                r_vci_ptr        = r_vci_ptr.read() + 1;
                if ( r_vci_nwords.read() == 1 ) r_vci_fsm = VCI_IDLE;
            }
            break;
        }
        /////////////////////
        case VCI_READ_RX_ROK:   // send rx_channel ROK in VCI response
        {
            if ( p_vci.rspack.read() )
            {
                r_vci_fsm = VCI_IDLE;
            }
            break;
        }
        //////////////////////////
        case VCI_WRITE_RX_RELEASE:   // release tx_channel, reset r_vci_ptr,
                                    // and send VCI write response
        {
            if ( p_vci.rspack.read() )
            {
                rx_channel_rcmd = RX_CHANNEL_RCMD_RELEASE;
                r_vci_ptr       = 0;
                r_vci_fsm       = VCI_IDLE;
            }
            break;
        }
        /////////////////////
        case VCI_WRITE_MAC_4:       // set r_channel_mac_4 for selected channel,
                                    // and send VCI write response
        {
            if ( p_vci.rspack.read() )
            {
                size_t channel = r_vci_channel.read();
                r_channel_mac_4[channel] = r_vci_wdata.read();
                r_vci_fsm       = VCI_IDLE;
            }
            break;
        }
        /////////////////////
        case VCI_WRITE_MAC_2:       // set r_channel_mac_2 for selected channel,
                                    // and send VCI write response
        {
            if ( p_vci.rspack.read() )
            {
                size_t channel = r_vci_channel.read();
                r_channel_mac_2[channel] = r_vci_wdata.read();
                r_vci_fsm       = VCI_IDLE;
            }
            break;
        }
    } // end switch tgt_fsm

    ///////////////////////////////////////////////////////////////////////////
    // This RX_G2S module makes the GMII to STREAM format conversion.
    // It cheks the checksum, ans signals a possible error.
    // The input is the gmii_in module.
    // The output is the rx_fifo_stream, but this fifo is only used for
    // clock boundary handling, and should never be full,
    // as the consumer (RX_DES module) read all available bytes. 
    ///////////////////////////////////////////////////////////i////////////////
    
    // get data from PHY component
    bool    gmii_rx_dv;
    bool    gmii_rx_er;
    uint8_t gmii_rx_data;

    r_gmii_rx.get( &gmii_rx_dv, 
                   &gmii_rx_er, 
                   &gmii_rx_data );

    // default values for fifo commands
    bool              rx_fifo_stream_write = false;
    uint16_t          rx_fifo_stream_wdata;
    
    assert( r_rx_fifo_stream.wok() and
    "ERROR in VCI_MULTI_NIC : the rs_fifo_stream should never be full");

    r_rx_g2s_dt0 = gmii_rx_data;
    r_rx_g2s_dt1 = r_rx_g2s_dt0.read();
    r_rx_g2s_dt2 = r_rx_g2s_dt1.read();
    r_rx_g2s_dt3 = r_rx_g2s_dt2.read();
    r_rx_g2s_dt4 = r_rx_g2s_dt3.read();
    r_rx_g2s_dt5 = r_rx_g2s_dt4.read();

    ///////////////////////////
    switch(r_rx_g2s_fsm.read()) 
    {
        /////////////////
        case RX_G2S_IDLE:   // waiting start of packet
        {
            if ( gmii_rx_dv and not gmii_rx_er ) // start of packet / no error
            {
                r_rx_g2s_fsm   = RX_G2S_DELAY; 
                r_rx_g2s_delay = 0;
            }
            break;
        }
        //////////////////
        case RX_G2S_DELAY:  // entering bytes in the pipe (4 cycles)
        {
            if ( not gmii_rx_dv or gmii_rx_er ) // data invalid or error
            {
                r_rx_g2s_fsm = RX_G2S_IDLE;
            }
            else if ( r_rx_g2s_delay.read() == 3 ) 
            {
                r_rx_g2s_fsm = RX_G2S_LOAD;
            }
            else
            {
                r_rx_g2s_delay = r_rx_g2s_delay.read() + 1;
            }
            break;
        }
        /////////////////
        case RX_G2S_LOAD:   // load first in checksum accu
        {
            if ( gmii_rx_dv and not gmii_rx_er ) // data valid / no error
            {
                r_rx_g2s_checksum = r_rx_g2s_dt4.read();
                r_rx_g2s_fsm      = RX_G2S_SOS;
            }
            else
            {
                r_rx_g2s_fsm      = RX_G2S_IDLE;
            }
            break;
        }
        ////////////////
        case RX_G2S_SOS:    // write first byte in fifo_stream: SOS
        {
            if ( gmii_rx_dv and not gmii_rx_er ) // data valid / no error
            {
                r_rx_g2s_checksum = r_rx_g2s_checksum.read() + r_rx_g2s_dt4.read();
                r_rx_g2s_fsm      = RX_G2S_LOOP;
                rx_fifo_stream_write = true;
                rx_fifo_stream_wdata = r_rx_g2s_dt5.read() | (STREAM_TYPE_SOS << 8);
            }
            else
            {
                r_rx_g2s_fsm      = RX_G2S_IDLE;
            }
            break;
        }
        /////////////////   
        case RX_G2S_LOOP:   // write one byte in fifo_stream : NEV 
        {
            rx_fifo_stream_write = true;
            rx_fifo_stream_wdata = r_rx_g2s_dt5.read() | (STREAM_TYPE_NEV << 8);

            if ( not gmii_rx_dv and not gmii_rx_er ) // end of paquet
            {
                r_rx_g2s_checksum = r_rx_g2s_checksum.read() + r_rx_g2s_dt4.read();
                r_rx_g2s_fsm = RX_G2S_END;
            }
            else if ( gmii_rx_dv and gmii_rx_er ) // error
            {
                r_rx_g2s_fsm = RX_G2S_FAIL;
            }
            else if ( not gmii_rx_dv and gmii_rx_er ) // error extend
            {
                r_rx_g2s_fsm = RX_G2S_FAIL;
            }
            break;
        }
        ////////////////
        case RX_G2S_END:    // signal end of packet: EOS or ERR depending on checksum
        {
            uint32_t check = (uint32_t)r_rx_g2s_dt4.read()       |
                             (uint32_t)r_rx_g2s_dt3.read() << 8  | 
                             (uint32_t)r_rx_g2s_dt2.read() << 16 | 
                             (uint32_t)r_rx_g2s_dt1.read() << 24 ; 
            if ( r_rx_g2s_checksum.read() == check )
            {
                rx_fifo_stream_write = true;
                rx_fifo_stream_wdata = r_rx_g2s_dt5.read() | (STREAM_TYPE_EOS << 8);
            }
            else
            {
                rx_fifo_stream_write = true;
                rx_fifo_stream_wdata = r_rx_g2s_dt5.read() | (STREAM_TYPE_ERR << 8);
            }

            if ( gmii_rx_dv and not gmii_rx_er ) // start of packet / no error
            {
                r_rx_g2s_fsm   = RX_G2S_DELAY;
                r_rx_g2s_delay = 0;
            }
            else 
            {
                r_rx_g2s_fsm   = RX_G2S_IDLE;
            }
            break;
        }
        /////////////////
        case RX_G2S_EXTD:   // waiting end of extend to signal error
        {
            if ( not gmii_rx_er ) 
            {
                r_rx_g2s_fsm   = RX_G2S_ERR;
            }
            break;
        }
        ////////////////
        case RX_G2S_ERR:  // signal error: ERR
        {
            rx_fifo_stream_write = true;
            rx_fifo_stream_wdata = r_rx_g2s_dt5.read() | (STREAM_TYPE_ERR << 8);
 
            if ( gmii_rx_dv and not gmii_rx_er ) // start of packet / no error
            {
                r_rx_g2s_fsm   = RX_G2S_DELAY;
                r_rx_g2s_delay = 0;
            }
            else 
            {
                r_rx_g2s_fsm   = RX_G2S_IDLE;
            }
            break;
        }
        /////////////////
        case RX_G2S_FAIL: // waiting end of paquet to signal error
        {
            if ( not gmii_rx_dv )
            {
                r_rx_g2s_fsm   = RX_G2S_ERR;
            }
            break;
        }
    } // end switch rx_g2s_type_fsm
    
    ///////////////////////////////////////////////////////////////////////////
    // This RX_DES module is in charge of deserialisation (4 bytes -> 1 word).
    // - The input is the rx_fifo_stream, respecting the stream format: 
    //   8 bits data + 2 bits type.
    // - The output is the rx_fifo_multi that can store a full paquet.
    // It is also charge of discarding input packets in two cases:
    // - if a checksum error is reported by the RS_G2S FSM
    // - if there not space in the rx_fifo_multi
    ///////////////////////////////////////////////////////////i////////////////
    
    // default values for fifo commands
    bool              rx_fifo_stream_read   = true;
    fifo_multi_wcmd_t rx_fifo_multi_wcmd    = FIFO_MULTI_WCMD_NOP;
    uint32_t          rx_fifo_multi_wdata   = 0;
    uint32_t          rx_fifo_multi_padding = 0;  // only used for the last word
    
    switch(r_rx_des_fsm.read()) 
    {
        ///////////////////////
        case RX_DES_READ_FIRST:    // read first 4 bytes in fifo_stream
        {
            size_t   index = r_rx_des_byte_index.read();
            uint16_t data;
            uint32_t type;
            if ( r_rx_fifo_stream.rok() ) // do nothing if we cannot read
            {
                data                = r_rx_fifo_stream.read();
                type                = (data >> 8) & 0x3;
                if ( ((index == 0) and (type != STREAM_TYPE_SOS)) or
                     ((index != 0) and (type != STREAM_TYPE_NEV)) )  // illegal byte type
                {
                    r_rx_des_byte_index     = 0;
                }
                else              
                {
                    r_rx_des_data[index]    = (uint8_t)(data & 0xFF);
                    if ( index == 3 )                               // last byte in word
                    {
                        r_rx_des_byte_index = 0;
                        r_rx_des_fsm        = RX_DES_READ_WRITE;
                    }
                    else
                    {
                        r_rx_des_byte_index  = index + 1;
                    }
                }
            }
            break;
        }
        ///////////////////////
        case RX_DES_READ_WRITE:     // read next 4 bytes in fifo_stream
                                    // and write one word in fifo_multi
                                    // when reading the first byte in fifo_stream
        {
            size_t   index = r_rx_des_byte_index.read();
            uint16_t data;
            uint32_t type;

            if ( index == 0 )       // accessing both fifo_stream & fifo_multi  
            {
                if ( r_rx_fifo_stream.rok() )     // do nothing if we cannot read
                {
                    if ( r_rx_fifo_multi.wok() )  // both fifos OK => read & write
                    {
                        rx_fifo_stream_read = true;
                        data                = r_rx_fifo_stream.read();
                        type                = (data >> 8) & 0x3;
                        if ( (type == STREAM_TYPE_SOS) or 
                             (type == STREAM_TYPE_ERR) )    // illegal type => drop packet
                        {
                            if(r_rx_des_dirty.read()) rx_fifo_multi_wcmd = FIFO_MULTI_WCMD_CLEAR;
                            rx_fifo_multi_wcmd  = FIFO_MULTI_WCMD_CLEAR;
                            r_rx_des_fsm         = RX_DES_READ_FIRST;
                            r_rx_des_byte_index  = 0;
                            r_rx_des_dirty       = false;
                        }
                        else // legal types : STREAM_TYPE_NEV or STREAM_TYPE_EOS
                        {
                            // write previous word into fifo_multi
                            rx_fifo_multi_wcmd  = FIFO_MULTI_WCMD_WRITE;
                            rx_fifo_multi_wdata = (uint32_t)(r_rx_des_data[0].read()      ) |
                                                  (uint32_t)(r_rx_des_data[1].read() << 8 ) |
                                                  (uint32_t)(r_rx_des_data[2].read() << 16) |
                                                  (uint32_t)(r_rx_des_data[3].read() << 24) ;
                            r_rx_des_dirty       = true;

                            // register read data
                            r_rx_des_data[index] = (uint8_t)(data & 0xFF);
                            if ( type == STREAM_TYPE_EOS ) 
                            {
                                r_rx_des_fsm = RX_DES_WRITE_LAST;
                            }
                            else
                            {
                                r_rx_des_byte_index  = index + 1;
                            }
                        }
                    }
                    else    // drop packet if we cannot write
                    {
                        if ( r_rx_des_dirty.read() ) rx_fifo_multi_wcmd = FIFO_MULTI_WCMD_CLEAR;
                        r_rx_des_fsm        = RX_DES_READ_FIRST;
                        r_rx_des_byte_index = 0;
                        r_rx_des_dirty      = false;
                    }
                }
            }
            else  // index > 0  => accessing fifo_stream without accessing fifo_multi
            {
                if ( r_rx_fifo_stream.rok() )  // do nothing if we cannot read
                {
                    rx_fifo_stream_read = true;
                    data                = r_rx_fifo_stream.read();
                    type                = (data >> 8) & 0x3;
                    if ( (type == STREAM_TYPE_SOS) or (type == STREAM_TYPE_ERR) )
                    {
                        rx_fifo_multi_wcmd  = FIFO_MULTI_WCMD_CLEAR;
                        r_rx_des_fsm         = RX_DES_READ_FIRST;
                        r_rx_des_byte_index  = 0;
                        r_rx_des_dirty       = false;
                    }
                    else // type == STREAM_TYPE_NEV or type == STREAM_TYPE_EOS
                    {
                        r_rx_des_data[index] = (uint8_t)(data & 0xFF);
                        if ( type == STREAM_TYPE_EOS ) 
                        {
                            r_rx_des_fsm = RX_DES_WRITE_LAST;
                        }
                        else if (index == 3)
                        {
                            r_rx_des_data[index] = (uint8_t)(data & 0xFF);
                            r_rx_des_byte_index  = 0;
                        }
                        else
                        {
                            r_rx_des_byte_index  = index + 1;
                        }
                    }
                }
            }
            break;
        }
        ///////////////////////
        case RX_DES_WRITE_LAST:     // write last word in rx_fifo_multi
                                    // or drop packet if fifo full
                                    // and read first byte in fifo_stream if possible
        {
            // access fifo_multi
            if ( r_rx_fifo_multi.wok() )
            {
                rx_fifo_multi_wdata = (uint32_t)(r_rx_des_data[0].read()      ) |
                                      (uint32_t)(r_rx_des_data[1].read() << 8 ) |
                                      (uint32_t)(r_rx_des_data[2].read() << 16) |
                                      (uint32_t)(r_rx_des_data[3].read() << 24) ;
                rx_fifo_multi_wcmd  = FIFO_MULTI_WCMD_LAST;
                rx_fifo_multi_padding = 4 - r_rx_des_byte_index.read();
            }
            else
            {
                rx_fifo_multi_wcmd  = FIFO_MULTI_WCMD_CLEAR;
            }

            // access fifo_stream
            size_t   index = 0;
            uint16_t data;
            uint32_t type;
            if ( r_rx_fifo_stream.rok() )
            {
                data                = r_rx_fifo_stream.read();
                type                = (data >> 8) & 0x3;
                if ( type == STREAM_TYPE_SOS )
                {
                    r_rx_des_data[0] = (uint8_t)(data & 0xFF);
                    index = 1;
                }
            }
            r_rx_des_byte_index = index;
            r_rx_des_fsm        = RX_DES_READ_FIRST;
            r_rx_des_dirty      = false;
            break;
        }
    } // end swich rx_des_fsm


    ///////////////////////////////////////////////////////////////////
    // The RX_DISPATCH FSM performs the actual transfer of
    // a packet from the rx_fifo_multi or bp_fifo_multi to 
    // the rx_channel container selected by the MAC address decoding. 
    // Packets with unexpected MAC address are discarded.
    // It imlements a round-robin priority between the two fifos.
    // Allocation is only done when a complete packet has been
    // transfered, and the DISPATCH FSM is IDLE.
    ///////////////////////////////////////////////////////////////////

    // default values for channel & fifos commands
    fifo_multi_rcmd_t    rx_fifo_multi_rcmd    = FIFO_MULTI_RCMD_NOP;
    fifo_multi_rcmd_t    bp_fifo_multi_rcmd    = FIFO_MULTI_RCMD_NOP;
    rx_channel_wcmd_t    rx_channel_wcmd       = RX_CHANNEL_WCMD_NOP;
    uint32_t             rx_channel_wdata      = 0;
    uint32_t             rx_channel_padding    = 0;

    switch( r_rx_dispatch_fsm.read() )
    {
        case RX_DISPATCH_IDLE:  // ready to start a new packet transfer
        {
            if ( r_rx_dispatch_bp.read() )  // previously allocated to bp_fifo
            {
                if ( r_rx_fifo_multi.rok() ) r_rx_dispatch_bp = false;
            }
            else                            // previously allocated to rx_fifo
            {
                if ( r_bp_fifo_multi.rok() ) r_rx_dispatch_bp = true;
            }
      
            if ( r_bp_fifo_multi.rok() or r_rx_fifo_multi.rok() ) // packet available
            {
                r_rx_dispatch_fsm = RX_DISPATCH_GET_PLEN;
            }
            break;
        }
        case RX_DISPATCH_GET_PLEN: // get packet length from fifo
        {   
            uint32_t plen;
            if ( r_rx_dispatch_bp.read() ) plen = r_bp_fifo_multi.plen();  
            else                           plen = r_rx_fifo_multi.plen();  
            r_rx_dispatch_plen = plen;
            if ( (plen & 0x3) == 0 ) r_rx_dispatch_words = plen >> 2;
            else                     r_rx_dispatch_words = (plen >> 2) + 1;
            r_rx_dispatch_fsm  = RX_DISPATCH_READ_FIRST;
            break;
        }
        case RX_DISPATCH_READ_FIRST: // read first word from fifo
        {
            if ( r_rx_dispatch_bp.read() )
            {
                bp_fifo_multi_rcmd       = FIFO_MULTI_RCMD_READ;
                r_rx_dispatch_data = r_bp_fifo_multi.data();  
            }
            else
            {
                rx_fifo_multi_rcmd       = FIFO_MULTI_RCMD_READ;
                r_rx_dispatch_data = r_rx_fifo_multi.data();
            }
            r_rx_dispatch_words = r_rx_dispatch_words.read() - 1;
            r_rx_dispatch_fsm   = RX_DISPATCH_CHANNEL_SELECT;
            break;
        }
        case RX_DISPATCH_CHANNEL_SELECT:  // check MAC address
        {
            // we read the second data word, without modifying the fifo state
            unsigned int    data_ext;
            bool            found = false;
            if ( r_rx_dispatch_bp.read() )  data_ext = r_bp_fifo_multi.data() & 0x0000FFFF;  
            else                            data_ext = r_rx_fifo_multi.data() & 0x0000FFFF;
            for ( size_t k = 0 ; (k < m_channels) and not found ; k++ )
            {
                if ( (r_channel_mac_4[k].read() == r_rx_dispatch_data.read() ) and
                     (r_channel_mac_2[k].read() == data_ext) )
                {
                    found                 = true;
                    r_rx_dispatch_channel = k;
                }
            }
            if (found) r_rx_dispatch_fsm = RX_DISPATCH_GET_WOK;
            else       r_rx_dispatch_fsm = RX_DISPATCH_PACKET_SKIP;
            break;
        }
        case RX_DISPATCH_PACKET_SKIP:	// clear an unexpected packet in source fifo
        {
            if ( r_rx_dispatch_bp.read() )  bp_fifo_multi_rcmd = FIFO_MULTI_RCMD_SKIP;
            else                            rx_fifo_multi_rcmd = FIFO_MULTI_RCMD_SKIP;
            r_rx_dispatch_fsm = RX_DISPATCH_IDLE;
            break;
        }
        case RX_DISPATCH_GET_WOK: // test if there is an open container in selected channel
        {
            uint32_t channel = r_rx_dispatch_channel.read();
            bool wok         = r_rx_channel[channel]->wok();
            if ( wok )  r_rx_dispatch_fsm = RX_DISPATCH_GET_SPACE;
            break;
        }
        case RX_DISPATCH_GET_SPACE: // test available space in selected channel
        {
            uint32_t channel = r_rx_dispatch_channel.read();
            uint32_t space   = r_rx_channel[channel]->space();
            uint32_t plen    = r_rx_dispatch_plen.read();
            if ( plen < space ) r_rx_dispatch_fsm = RX_DISPATCH_READ_WRITE;
            else                r_rx_dispatch_fsm = RX_DISPATCH_CLOSE_CONT;
            break;
        }
        case RX_DISPATCH_CLOSE_CONT:  // Not enough space : close container
        {
            rx_channel_wcmd   = RX_CHANNEL_WCMD_CLOSE;
            r_rx_dispatch_fsm = RX_DISPATCH_GET_WOK;
            break; 
        }
        case RX_DISPATCH_READ_WRITE:    // read a new word from fifo and
                                        // write previous word to channel
        {
            rx_channel_wcmd     = RX_CHANNEL_WCMD_WRITE;
            rx_channel_wdata    = r_rx_dispatch_data.read();
            if ( r_rx_dispatch_bp.read() )
            {
                if ( r_rx_dispatch_words.read() == 1 ) bp_fifo_multi_rcmd = FIFO_MULTI_RCMD_LAST;
                else                                   bp_fifo_multi_rcmd = FIFO_MULTI_RCMD_READ;
                r_rx_dispatch_data = r_bp_fifo_multi.data();  
            }
            else
            {
                if ( r_rx_dispatch_words.read() == 1 ) rx_fifo_multi_rcmd = FIFO_MULTI_RCMD_LAST;
                else                                   rx_fifo_multi_rcmd = FIFO_MULTI_RCMD_READ;
                r_rx_dispatch_data = r_rx_fifo_multi.data();
            }
            r_rx_dispatch_words = r_rx_dispatch_words.read() - 1;
            r_rx_dispatch_fsm   = RX_DISPATCH_WRITE_LAST;
            break;
            
            break;
        }
        case RX_DISPATCH_WRITE_LAST:  // write last word to selected channel
        {
            uint32_t plen = r_rx_dispatch_plen.read();
            if ( (plen & 0x3) == 0 )    rx_channel_padding  = 0;
            else                        rx_channel_padding = 4 - (plen & 0x3);
            rx_channel_wcmd     = RX_CHANNEL_WCMD_LAST;
            rx_channel_wdata    = r_rx_dispatch_data.read();
            r_rx_dispatch_fsm   = RX_DISPATCH_IDLE;
            break;
        }
    } // end switch r_rx_dispatch_fsm
                
    /////////////////////////////////////////////////////////////////////
    // The TX_DISPATCH FSM performs the actual transfer of
    // all packet from a tx_channel[k] container to tx_fifo or bp_fifo,
    // depending on the MAC address.
    // - the by-pass fifo is a multi-buffer fifo (typically 2 Kbytes).
    // - the tx_fifo is a simple fifo (10 bits width / é slots depth).
    // It implements a round-robin priority between the channels.
    // A new allocation is only done when a complete container has been
    // transmitted, and the TX_DISPATCH FSM is IDLE.
    /////////////////////////////////////////////////////////////////////

    // default values for channel & fifos commands and data
    bool                 tx_fifo_stream_write  = false;
    fifo_multi_wcmd_t    bp_fifo_multi_wcmd    = FIFO_MULTI_WCMD_NOP;
    tx_channel_rcmd_t    tx_channel_rcmd       = TX_CHANNEL_RCMD_NOP;
    uint32_t             tx_fifo_stream_wdata  = 0;
    uint32_t             bp_fifo_multi_padding = 0;
    uint32_t             bp_fifo_multi_wdata   = 0;

    switch( r_tx_dispatch_fsm.read() )
    {
        //////////////////////
        case TX_DISPATCH_IDLE:  // ready to start a new packet transfer
        {
            for ( size_t x = 0 ; x < m_channels ; x++ )
            {
                size_t k = (x + 1 + r_tx_dispatch_channel.read()) % m_channels;
                if ( r_tx_channel[k]->rok() ) 
                {
                    r_tx_dispatch_channel = k;
                    r_tx_dispatch_fsm     = TX_DISPATCH_GET_NPKT;
                    break;
                }
            }
            break;
        }
        //////////////////////////
        case TX_DISPATCH_GET_NPKT: // get packet number from tx_channel
        {   
            uint32_t    channel   = r_tx_dispatch_channel.read();
            r_tx_dispatch_packets = r_tx_channel[channel]->npkt();
            break;
        }
        case TX_DISPATCH_GET_PLEN: // get packet length from tx_channel
        {   
            uint32_t channel      = r_tx_dispatch_channel.read();
            uint32_t plen         = r_tx_channel[channel]->plen();
            r_tx_dispatch_bytes   = plen & 0x3;
            if ( (plen & 0x3) == 0 ) r_tx_dispatch_words = plen >> 2;
            else                     r_tx_dispatch_words = (plen >> 2) + 1;
            r_tx_dispatch_fsm  = TX_DISPATCH_READ_FIRST;
            break;
        }
        case TX_DISPATCH_READ_FIRST: // read first word from tx_channel
        {
            uint32_t channel    = r_tx_dispatch_channel.read();
            tx_channel_rcmd     = TX_CHANNEL_RCMD_READ;
            r_tx_dispatch_data  = r_tx_channel[channel]->data();
            r_tx_dispatch_words = r_rx_dispatch_words.read() - 1;
            r_tx_dispatch_fsm   = TX_DISPATCH_FIFO_SELECT;
            break;
        }
        case TX_DISPATCH_FIFO_SELECT: // check MAC address
        {
            // we read the second data word, without modifying the channel state
            uint32_t        channel  = r_tx_dispatch_channel.read();
            unsigned int    data_ext = r_tx_channel[channel]->data() & 0x0000FFFF;  
            bool            found    = false;
            for ( size_t k = 0 ; (k < m_channels) and not found ; k++ )
            {
                if ( (r_channel_mac_4[k].read() == r_tx_dispatch_data.read() ) and
                     (r_channel_mac_2[k].read() == data_ext) ) found = true;
            }
            if ( found )    r_tx_dispatch_fsm = TX_DISPATCH_READ_WRITE_BP;
            else            r_tx_dispatch_fsm = TX_DISPATCH_WRITE_B0_TX;
            break;
        }
        case TX_DISPATCH_READ_WRITE_BP: // write previous data in bp_fifo
                                        // and read a new data from selected channel
        {
            uint32_t  channel = r_tx_dispatch_channel.read();
            uint32_t  words   = r_tx_dispatch_words.read();  

            if ( r_bp_fifo_multi.wok() )
            {     
                bp_fifo_multi_wcmd    = FIFO_MULTI_WCMD_WRITE;
                bp_fifo_multi_wdata   = r_tx_dispatch_data.read();
                r_tx_dispatch_data    = r_tx_channel[channel]->data();
                r_tx_dispatch_words   = words - 1;
                if ( words == 1 )       // read last word
                {
                    tx_channel_rcmd   = TX_CHANNEL_RCMD_LAST;
                    r_tx_dispatch_fsm = TX_DISPATCH_WRITE_LAST_BP;
                }
                else
                {
                    tx_channel_rcmd   = TX_CHANNEL_RCMD_READ;
                }
            }
            break;
        }
        case TX_DISPATCH_WRITE_LAST_BP:  // write last word in bp_fifo
        {
            uint32_t  packets = r_tx_dispatch_packets.read();
            uint32_t  bytes   = r_tx_dispatch_bytes.read();

            if ( r_bp_fifo_multi.wok() )
            {
                bp_fifo_multi_wcmd    = FIFO_MULTI_WCMD_LAST;
                bp_fifo_multi_wdata   = r_tx_dispatch_data.read();
                if ( bytes == 0 ) bp_fifo_multi_padding = 0;
                else              bp_fifo_multi_padding = 4 - bytes;
                r_tx_dispatch_packets = packets - 1;
                if ( packets == 1 ) r_tx_dispatch_fsm = TX_DISPATCH_RELEASE_CONT; 
                else                r_tx_dispatch_fsm = TX_DISPATCH_GET_PLEN; 
            }
            break;
        }
        case TX_DISPATCH_WRITE_B0_TX: // write byte 0 in tx_fifo
        {
            uint32_t words   = r_tx_dispatch_words.read();
            uint32_t bytes   = r_tx_dispatch_bytes.read();
            uint32_t packets = r_tx_dispatch_packets.read();
            if ( r_tx_fifo_stream.wok() )
            {
                tx_fifo_stream_write = true;
                tx_fifo_stream_wdata = r_tx_dispatch_data.read() & 0x000000FF;
                if ( (words == 0) and (bytes == 1) )   // last byte to write in tx_fifo
                {
                    tx_fifo_stream_wdata         = tx_fifo_stream_wdata | 0x00000100;
                    r_tx_dispatch_packets = packets - 1;
                    if ( packets == 1 ) r_tx_dispatch_fsm = TX_DISPATCH_RELEASE_CONT;
                    else                r_tx_dispatch_fsm = TX_DISPATCH_GET_PLEN;
                }
                else
                {
                    r_tx_dispatch_fsm = TX_DISPATCH_WRITE_B1_TX;
                }
            }
            break;
        }
        case TX_DISPATCH_WRITE_B1_TX: // write byte 1 in tx_fifo
        {
            uint32_t    words = r_tx_dispatch_words.read();
            uint32_t    bytes = r_tx_dispatch_bytes.read(); 
            uint32_t packets = r_tx_dispatch_packets.read();
            if ( r_tx_fifo_stream.wok() )
            {
                tx_fifo_stream_write = true;
                tx_fifo_stream_wdata = (r_tx_dispatch_data.read() >> 8) & 0x000000FF;
                if ( (words == 0) and (bytes == 2) )   // last byte to write in tx_fifo
                {
                    tx_fifo_stream_wdata = tx_fifo_stream_wdata | 0x00000100;
                    r_tx_dispatch_packets = packets - 1;
                    if ( packets == 1 ) r_tx_dispatch_fsm = TX_DISPATCH_RELEASE_CONT;
                    else                r_tx_dispatch_fsm = TX_DISPATCH_GET_PLEN;
                }
                else
                {
                    r_tx_dispatch_fsm = TX_DISPATCH_WRITE_B2_TX;
                }
            }
            break;
        }
        case TX_DISPATCH_WRITE_B2_TX: // write byte 2 in tx_fifo
        {
            uint32_t    words = r_tx_dispatch_words.read();
            uint32_t    bytes = r_tx_dispatch_bytes.read(); 
            uint32_t packets = r_tx_dispatch_packets.read();
            if ( r_tx_fifo_stream.wok() )
            {
                tx_fifo_stream_write = true;
                tx_fifo_stream_wdata = (r_tx_dispatch_data.read() >> 16) & 0x000000FF;
                if ( (words == 0) and (bytes == 3) )   // last byte to write in tx_fifo
                {
                    tx_fifo_stream_wdata = tx_fifo_stream_wdata | 0x00000100;
                    r_tx_dispatch_packets = packets - 1;
                    if ( packets == 1 ) r_tx_dispatch_fsm = TX_DISPATCH_RELEASE_CONT;
                    else                r_tx_dispatch_fsm = TX_DISPATCH_GET_PLEN;
                }
                else
                {
                    r_tx_dispatch_fsm = TX_DISPATCH_READ_WRITE_TX;
                }
            }
            break;
        }
        case TX_DISPATCH_READ_WRITE_TX: // write byte 3 in tx_fifo
                                        // and read word from selected channel
                                        // if the current word is not the last
        {
            uint32_t channel = r_tx_dispatch_channel.read();
            uint32_t words   = r_tx_dispatch_words.read();
            uint32_t packets = r_tx_dispatch_packets.read();
            if ( r_tx_fifo_stream.wok() )
            {
                tx_fifo_stream_write = true;
                tx_fifo_stream_wdata = (r_tx_dispatch_data.read() >> 24) & 0x000000FF;
                if ( words == 0 )       // last byte to write in tx_fifo
                {
                    tx_fifo_stream_wdata = tx_fifo_stream_wdata | 0x00000100;
                    r_tx_dispatch_packets = packets - 1;
                    if ( packets == 1 ) r_tx_dispatch_fsm = TX_DISPATCH_RELEASE_CONT;
                    else                r_tx_dispatch_fsm = TX_DISPATCH_GET_PLEN;
                }
                else if ( words == 1 )  // last word to read in tx_channel
                {
                    tx_channel_rcmd     = TX_CHANNEL_RCMD_LAST;
                    r_tx_dispatch_data  = r_tx_channel[channel]->data();
                    r_tx_dispatch_words = words - 1;
                    r_tx_dispatch_fsm   = TX_DISPATCH_WRITE_B0_TX;
                }
                else if ( words > 1 )   // not the last word to read in tx_channel
                {
                    tx_channel_rcmd     = TX_CHANNEL_RCMD_READ;
                    r_tx_dispatch_data  = r_tx_channel[channel]->data();
                    r_tx_dispatch_words = words - 1;
                    r_tx_dispatch_fsm   = TX_DISPATCH_WRITE_B0_TX;
                }
            }
            break;
        }
        case TX_DISPATCH_RELEASE_CONT: // release the container in tx_channel
        {
            tx_channel_rcmd   = TX_CHANNEL_RCMD_RELEASE;
            r_tx_dispatch_fsm = TX_DISPATCH_IDLE;
            break;
        }
    } // end switch tx_dispatch_fsm   
    
    ////////////////////////////////////////////////////////////////////////////
    // This TX_S2G FSM performs the STREAM to GMII format conversion,
    // computes the checksum, and append this checksum to the packet.
    // The input is the tx_fifo_stream. 
    // The output is the r_gmii_tx module.
    ////////////////////////////////////////////////////////////////////////////

    // default value for fifo command
    bool    tx_fifo_stream_read = false;

    switch(r_tx_s2g_fsm.read()) 
    {
        /////////////////
        case TX_S2G_IDLE:           // read one byte from stream fifo 
        {
            if ( r_tx_fifo_stream.rok() )
            {
                uint32_t data = r_tx_fifo_stream.read();
                uint32_t type = (data >> 8) & 0x3;

                assert ( (type == STREAM_TYPE_SOS) and
                "ERROR in VCI_MULTI_NIC : illegal type received in TX_S2G_IDLE");

                tx_fifo_stream_read = true;
                r_tx_s2g_fsm        = TX_S2G_WRITE_DATA;
                r_tx_s2g_checksum   = data & 0xFF;
                r_tx_s2g_data       = data & 0xFF;
            } 
            r_gmii_tx.put(0, false, false);
            break;
        }
        //////////////////////
        case TX_S2G_WRITE_DATA:     // write one data byte into gmii_tx
                                    // and read next byte from fifo_stream
        {
            if ( r_tx_fifo_stream.rok() )
            {
                uint32_t data = r_tx_fifo_stream.read();
                uint32_t type = (data >> 8) & 0x3;

                assert ( (type != STREAM_TYPE_SOS) and (type != STREAM_TYPE_ERR) and
                "ERROR in VCI_MULTI_NIC : illegal type received in TX_S2G_WRITE_DATA");
                    
                if ( type == STREAM_TYPE_EOS ) r_tx_s2g_fsm = TX_S2G_WRITE_CS; 

                tx_fifo_stream_read = true;
                r_tx_s2g_data       = data & 0xFF;
                r_tx_s2g_checksum   = r_tx_s2g_checksum.read() + (data & 0xFF);
            }
            else
            {
                assert (false and  
                "ERROR in VCI_MULTI_NIC : tx_fifo should not be empty");
            } 
            r_gmii_tx.put( r_tx_s2g_data.read(), true, false);
            break;
        }
        ////////////////////
        case TX_S2G_WRITE_CS:       // write one cs byte into gmii_out
        {
            uint8_t gmii_data;
            if ( r_tx_s2g_index.read() == 0 ) 
            {
                gmii_data      = r_tx_s2g_checksum.read() & 0xFF;
                r_tx_s2g_index = 1;
            }
            else if ( r_tx_s2g_index.read() == 1 ) 
            {
                gmii_data      = (r_tx_s2g_checksum.read() >> 8) & 0xFF;
                r_tx_s2g_index = 2;
            }
            else if ( r_tx_s2g_index.read() == 2 ) 
            {
                gmii_data      = (r_tx_s2g_checksum.read() >> 16) & 0xFF;
                r_tx_s2g_index = 3;
            }
            else // r_tx_s2g_index == 3 
            {
                gmii_data      = (r_tx_s2g_checksum.read() >> 24) & 0xFF;
                r_tx_s2g_index = 0;
                r_tx_s2g_fsm   = TX_S2G_IDLE;
            }    
            r_gmii_tx.put( gmii_data, true, false);
            break;
        }
    } // end switch tx_s2g_fsm
    
    ////////////////////////////////////////////////////////////////////////////
    // update multi_fifos
    ////////////////////////////////////////////////////////////////////////////
    r_rx_fifo_multi.update( rx_fifo_multi_wcmd, 
                            rx_fifo_multi_rcmd, 
                            rx_fifo_multi_wdata, 
                            rx_fifo_multi_padding );

    r_bp_fifo_multi.update( bp_fifo_multi_wcmd, 
                            bp_fifo_multi_rcmd, 
                            bp_fifo_multi_wdata, 
                            bp_fifo_multi_padding );

    ////////////////////////////////////////////////////////////////////////////
    // update stream_fifos
    ////////////////////////////////////////////////////////////////////////////
    r_rx_fifo_stream.update( rx_fifo_stream_write,
                             rx_fifo_stream_read, 
                             rx_fifo_stream_wdata );

    r_tx_fifo_stream.update( tx_fifo_stream_write,
                             tx_fifo_stream_read, 
                             tx_fifo_stream_wdata );

    ////////////////////////////////////////////////////////////////////////////
    // update rx_channels
    ////////////////////////////////////////////////////////////////////////////
    for ( size_t k = 0 ; k < m_channels ; k++ )
    {
        rx_channel_rcmd_t read;
        rx_channel_wcmd_t write;

        if ( r_rx_dispatch_channel.read() == k )    write = rx_channel_wcmd;
        else                                        write = RX_CHANNEL_WCMD_NOP;
        if ( r_vci_channel.read() == k )            read  = rx_channel_rcmd;
        else                                        read  = RX_CHANNEL_RCMD_NOP;

        r_rx_channel[k]->update( write,
                                read, 
                                rx_channel_wdata, 
                                rx_channel_padding );
    }

    ////////////////////////////////////////////////////////////////////////////
    // update tx_channels
    ////////////////////////////////////////////////////////////////////////////
    for ( size_t k = 0 ; k < m_channels ; k++ )
    {
        tx_channel_rcmd_t read;
        tx_channel_wcmd_t write;

        if ( r_tx_dispatch_channel.read() == k )    read  = tx_channel_rcmd;
        else                                        read  = TX_CHANNEL_RCMD_NOP;
        if ( r_vci_channel.read() == k )            write = tx_channel_wcmd;
        else                                        write = TX_CHANNEL_WCMD_NOP;

        r_tx_channel[k]->update( write, 
                                read, 
                                tx_channel_wdata );
    }

} // end transition

//////////////////////
tmpl(void)::genMoore()
{
    ///////////  Interrupts ////////
    for ( size_t k = 0 ; k < m_channels ; k++ )
    {
        p_rx_irq[k] = r_rx_channel[k]->rok();
        p_tx_irq[k] = r_tx_channel[k]->wok();
    }

    ////// VCI TARGET port /////// 
    size_t channel = r_vci_channel.read();
    
    switch( r_vci_fsm.read() ) {
        case VCI_IDLE:
        case VCI_WRITE_TX_BURST:
        {
            p_vci.cmdack = true;
            p_vci.rspval = false;
            break;
        }
        case VCI_WRITE_TX_LAST:
        case VCI_WRITE_TX_CLOSE:
        case VCI_WRITE_RX_RELEASE:
        case VCI_WRITE_MAC_4:
        case VCI_WRITE_MAC_2:
        {
            p_vci.cmdack = false;
            p_vci.rspval = true;
            p_vci.rdata  = 0;
            p_vci.rerror = vci_param::ERR_NORMAL;
            p_vci.rsrcid = r_vci_srcid.read();
            p_vci.rtrdid = r_vci_trdid.read();
            p_vci.rpktid = r_vci_pktid.read();
            p_vci.reop   = true;
            break;
        }
        case VCI_READ_RX_BURST:
        {
            p_vci.cmdack = false;
            p_vci.rspval = true;
            p_vci.rdata  = r_rx_channel[channel]->data();
            p_vci.rerror = vci_param::ERR_NORMAL;
            p_vci.rsrcid = r_vci_srcid.read();
            p_vci.rtrdid = r_vci_trdid.read();
            p_vci.rpktid = r_vci_pktid.read();
            if ( r_vci_nwords == 1 ) p_vci.reop = true;
            else                     p_vci.reop = false;
            break;
        }
        case VCI_READ_RX_ROK:
        {
            p_vci.cmdack = false;
            p_vci.rspval = true;
            p_vci.rdata  = r_rx_channel[channel]->rok();
            p_vci.rerror = vci_param::ERR_NORMAL;
            p_vci.rsrcid = r_vci_srcid.read();
            p_vci.rtrdid = r_vci_trdid.read();
            p_vci.rpktid = r_vci_pktid.read();
            p_vci.reop   = true;
            break;
        }
        case VCI_READ_TX_WOK:
        {
            p_vci.cmdack = false;
            p_vci.rspval = true;
            p_vci.rdata  = r_tx_channel[channel]->wok();
            p_vci.rerror = vci_param::ERR_GENERAL_DATA_ERROR;
            p_vci.rsrcid = r_vci_srcid.read();
            p_vci.rtrdid = r_vci_trdid.read();
            p_vci.rpktid = r_vci_pktid.read();
            p_vci.reop   = true;
            break;
        }
    } // end switch vci_fsm

} // end genMore()

/////////////////////////
tmpl(void)::print_trace()
{
    const char* vci_state_str[] = 
    {
        "VCI_IDLE",
        "VCI_READ_TX_WOK",
        "VCI_WRITE_TX_BURST",
        "VCI_WRITE_TX_LAST",
        "VCI_WRITE_TX_CLOSE",
        "VCI_READ_RX_ROK",
        "VCI_READ_RX_BURST",
        "VCI_WRITE_RX_RELEASE",
        "VCI_WRITE_MAC_4",
        "VCI_WRITE_MAC_2",
    };
    const char* rx_g2s_state_str[] = 
    {
        "RX_G2S_IDLE",
        "RX_G2S_DELAY",
        "RX_G2S_LOAD",
        "RX_G2S_SOS",
        "RX_G2S_LOOP",
        "RX_G2S_END",
        "RX_G2S_EXTD",
        "RX_G2S_ERR",
        "RX_G2S_FAIL",
    };
    const char* rx_des_state_str[] = 
    {
        "RX_DES_READ_FIRST",
        "RX_DES_READ_WRITE",
        "RX_DES_WRITE_LAST",
    };
    const char* rx_dispatch_state_str[] =
    {
        "RX_DISPATCH_IDLE",
        "RX_DISPATCH_GET_PLEN",
        "RX_DISPATCH_READ_FIRST",
        "RX_DISPATCH_CHANNEL_SELECT",
        "RX_DISPATCH_PACKET_SKIP",
        "RX_DISPATCH_GET_WOK",
        "RX_DISPATCH_CLOSE_CONT",
        "RX_DISPATCH_GET_SPACE",
        "RX_DISPATCH_READ_WRITE",
        "RX_DISPATCH_WRITE_LAST",
    };
    const char* tx_dispatch_state_str[] =
    {
        "TX_DISPATCH_IDLE",
        "TX_DISPATCH_GET_NPKT",
        "TX_DISPATCH_GET_PLEN",
        "TX_DISPATCH_READ_FIRST",
        "TX_DISPATCH_FIFO_SELECT",
        "TX_DISPATCH_READ_WRITE_BP",
        "TX_DISPATCH_WRITE_LAST_BP",
        "TX_DISPATCH_WRITE_B0_TX",
        "TX_DISPATCH_WRITE_B1_TX",
        "TX_DISPATCH_WRITE_B2_TX",
        "TX_DISPATCH_READ_WRITE_TX",
        "TX_DISPATCH_RELEASE_CONT",
    };
    const char* tx_s2g_state_str[] =
    {
        "TX_S2G_IDLE",
        "TX_S2G_WRITE_DATA",
        "TX_S2G_WRITE_CS",
    };

    std::cout << "MULTI_NIC " << name() << " : " 
              << vci_state_str[r_vci_fsm.read()]                 << " | "
              << rx_g2s_state_str[r_rx_g2s_fsm.read()]           << " | "
              << rx_des_state_str[r_rx_des_fsm.read()]           << " | "
              << rx_dispatch_state_str[r_rx_dispatch_fsm.read()] << " | "
              << tx_dispatch_state_str[r_tx_dispatch_fsm.read()] << " | "
              << tx_s2g_state_str[r_tx_s2g_fsm.read()]           << std::endl;
} // end print_trace()

////////////////////////////////////////////////////////////////////
tmpl(/**/)::VciMultiNic( sc_core::sc_module_name 		        name,
                         const soclib::common::IntTab 		    &tgtid,
                         const soclib::common::MappingTable 	&mt,
                         const size_t 				            channels,
                         const char*                            rx_file_pathname,
                         const char*                            tx_file_pathname, 
                         const size_t 				            timeout )
	    : caba::BaseModule(name),

          r_vci_fsm("r_vci_fsm"),
          r_vci_srcid("r_vci_srcid"),
          r_vci_trdid("r_vci_trdid"),
          r_vci_pktid("r_vci_pktid"),
          r_vci_wdata("r_vci_wdata"),
          r_vci_ptw("r_vci_ptw"),
          r_vci_ptr("r_vci_ptr"),
          r_vci_nwords("r_vci_nwords"),

          r_rx_g2s_fsm("r_rx_g2s_fsm"),
          r_rx_g2s_checksum("r_rx_g2s_checksum"),
          r_rx_g2s_dt0("r_rx_g2s_dt0"),
          r_rx_g2s_dt1("r_rx_g2s_dt1"),
          r_rx_g2s_dt2("r_rx_g2s_dt2"),
          r_rx_g2s_dt3("r_rx_g2s_dt3"),
          r_rx_g2s_dt4("r_rx_g2s_dt4"),
          r_rx_g2s_dt5("r_rx_g2s_dt5"),
          r_rx_g2s_delay("r_rx_g2s_delay"),

          r_rx_des_fsm("r_rx_des_fsm"),
          r_rx_des_data(soclib::common::alloc_elems<sc_signal<uint8_t> >("r_rx_des_data", 4)),
          r_rx_des_byte_index("r_rx_des_byte_index"),
          r_rx_des_dirty("r_rx_des_dirty"),

          r_rx_dispatch_fsm("r_rx_dispatch_fsm"),
          r_rx_dispatch_channel("r_rx_dispatch_channel"),
          r_rx_dispatch_bp("r_rx_dispatch_bp"),
          r_rx_dispatch_plen("r_rx_dispatch_plen"),
          r_rx_dispatch_data("r_rx_dispatch_data"),
          r_rx_dispatch_words("r_rx_dispatch_words"),

          r_tx_dispatch_fsm("r_tx_dispatch_fsm"),
          r_tx_dispatch_channel("r_tx_dispatch_channel"),
          r_tx_dispatch_data("r_tx_dispatch_data"),
          r_tx_dispatch_packets("r_tx_dispatch_packets"),
          r_tx_dispatch_words("r_tx_dispatch_words"),
          r_tx_dispatch_bytes("r_tx_dispatch_bytes"),

          r_tx_s2g_fsm("r_tx_s2g_fsm"),
          r_tx_s2g_checksum("r_tx_s2g_checksum"),
          r_tx_s2g_data("r_tx_s2g_data"),
          r_tx_s2g_index("r_tx_s2g_index"),

          r_rx_fifo_stream("r_rx_fifo_stream", 2),      // 2 slots of one byte
          r_rx_fifo_multi("r_rx_fifo_multi", 32, 32),   // 1024 slots of one word
          r_tx_fifo_stream("r_tx_fifo_stream", 2),      // 2 slots of one byte
          r_bp_fifo_multi("r_bp_fifo_multi", 32, 32),   // 1024 slots of one word

          r_gmii_rx("r_gmii_rx", "rx_file_pathname", 1),  // one cycle between packets
          r_gmii_tx("r_gmii_tx", "tx_file_pathname"),

          m_segment(mt.getSegment(tgtid)),
          m_channels(channels),

          p_clk("p_clk"),
          p_resetn("p_resetn"),
          p_vci("p_vci"),
          p_rx_irq(soclib::common::alloc_elems<sc_core::sc_out<bool> >("p_rx_irq", channels)),
          p_tx_irq(soclib::common::alloc_elems<sc_core::sc_out<bool> >("p_tx_irq", channels))
{
    assert( (vci_param::B == 4) and 
    "VCI_MULTI_NIC error : The VCI DATA field must be 32 bits");

    assert( (channels <= 8)  and
    "VCI_MULTI_NIC error : The number of channels cannot be larger than 8");

    r_rx_channel = new NicRxChannel*[channels];
    r_tx_channel = new NicTxChannel*[channels];

    for ( size_t k=0 ; k<channels ; k++)
    {
        r_rx_channel[k] = new NicRxChannel("r_rx_channel", timeout);
        r_tx_channel[k] = new NicTxChannel("r_tx_channel");
    }

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

