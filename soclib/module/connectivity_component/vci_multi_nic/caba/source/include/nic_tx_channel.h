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
 *         Alain Greiner <alain.greiner@lip6.fr> July 2008
 *         Clement Devigne <clement.devigne@etu.upmc.fr>
 *
 * Maintainers: alain 
 */

/**********************************************************************
 * File         : nic_tx_channel.h
 * Date         : 01/06/2012
 * Authors      : Alain Greiner
 **********************************************************************
 * This object implements an hardware communication channel,
 * to store transmitted packets (from software to NIC), 
 * for a multi-channels, GMII compliant, network controller.
 * It contains two  Kbytes containers, and is acting as
 * a two slots fifo (one container = one slot).
 * 
 * The container descriptor is defined by the first 32 words :
 * word0:       | NB_WORDS          | NB_PACKETS        |
 * word1:       | PLEN[1]           | PLEN[0]           |
 *              | ...               | ...               |
 * word31:      | PLEN[61]          | PLEN[62]          |
 * The packets are stored in the 1024-32) following words,
 * and the packets are word-aligned.
 *
 * The number of containers is not known by the writer or reader:
 * The WOK flag is true if there is a writable container.
 * The ROK flag is true if there is a readable container.
 * Both word and container addressing is handled by the channel itself.
 * - On the writer side, the software writes one full container,
 *   and release the container when this is done.
 * - On the reader side, the NIC reads all paquets in the container, 
 *   after checking the number of packets, and the packet lengths
 *   in the container descriptor (first 32 words).
 *   It releases the container when all packets have been transmitted.
 **********************************************************************
 * This object has 1 constructor parameter:
 * - string   name               : channel name
 **********************************************************************/

#ifndef SOCLIB_CABA_NIC_TX_CHANNEL
#define SOCLIB_CABA_NIC_TX_CHANNEL

#include <inttypes.h>
#include <systemc>
#include <assert.h>

namespace soclib { 
namespace caba {

using namespace sc_core;

#define NIC_CONTAINER_SIZE      1024 // Size in uint32_t
#define NIC_CONTAINER_SIZE_BYTES    NIC_CONTAINER_SIZE*4
#define MAX_PACKET                  ((NIC_CONTAINER_SIZE_BYTES-4)/62)

    // writer commands (software)
    enum tx_channel_wcmd_t {
        TX_CHANNEL_WCMD_NOP,       // no operation             (channel state not modified)
        TX_CHANNEL_WCMD_WRITE,     // write one word           (channel state modified) 
        TX_CHANNEL_WCMD_CLOSE,     // close container          (channel state modified)
    } ;

    // reader commands (NIC)
    enum tx_channel_rcmd_t {
        TX_CHANNEL_RCMD_NOP,       // no operation             (channel state not modified)
        TX_CHANNEL_RCMD_READ,      // read one word            (channel state modified)
        TX_CHANNEL_RCMD_LAST,      // read one word            (channel state modified)
        TX_CHANNEL_RCMD_RELEASE,   // release container        (channel state modified)
        TX_CHANNEL_RCMD_SKIP,      // skip current packet      (channel state modified)
    };

class NicTxChannel
{
    // structure constants
    const std::string   m_name;

    // registers
    uint32_t            r_ptw_word;          // word write pointer (in container)
    uint32_t            r_ptw_cont;          // container write pointer
    uint32_t            r_ptr_word;          // word read pointer (in container)
    uint32_t            r_ptr_cont;          // container read pointer
    uint32_t            r_sts;               // number of filled containers
    uint32_t            r_pkt_index;         // packet index in a container
    uint32_t            r_ptr_first;         // ptr_word at word[0] in current packet

    // containers
    uint32_t**          r_cont;              // data[2][1024]

public:

    /////////////
    void reset()
    {
        uint32_t k;
        r_ptr_word    = (MAX_PACKET/2)+1;
        r_ptr_first   = (MAX_PACKET/2)+1;
        r_ptr_cont    = 0;
        r_ptw_word    = 0;
        r_ptw_cont    = 0;
        r_pkt_index   = 0;
        r_sts         = 0;
        for(k = 0;k<2;k++)
            memset(r_cont[r_ptr_cont], 0,NIC_CONTAINER_SIZE);
    }

    /////////////////////////////////////////////////////
    // This method updates  the internal channel state,
    // depending on both cmd_w and cmd_r commands,
    // and must be called at each cycle.
    /////////////////////////////////////////////////////
    void update( tx_channel_wcmd_t      cmd_w, 
                 tx_channel_rcmd_t      cmd_r,
                 uint32_t               wdata)
    {
        // WCMD registers update (depends only on cmd_w)
        uint32_t    k = r_ptw_cont;

        if ( cmd_w == TX_CHANNEL_WCMD_WRITE )       // write one container word
        {
            assert( (r_ptw_word < NIC_CONTAINER_SIZE) and 
                    "ERROR in NIC_TX_CHANNEL : write pointer overflow" );

            if ( r_sts < 2 )  // at least one empty container
            {
                r_cont[k][r_ptw_word]   = wdata;
                r_ptw_word              = r_ptw_word + 1;
            }
            else
            {
                assert( "ERROR in NIC_TX_CHANNEL : illegal write request" );
            }
        }
        else if ( cmd_w == TX_CHANNEL_WCMD_CLOSE ) // close the current container
        {
            r_ptw_word = 0;
            r_ptw_cont = (r_ptw_cont + 1) % 2;
            r_sts      = r_sts + 1;
        }

        // RCMD register update (depends only on cmd_r)
        
        if ( cmd_r == TX_CHANNEL_RCMD_READ )       // read one packet word
        {
            assert( (r_ptr_word < NIC_CONTAINER_SIZE) and
                    "ERROR in NIC_TX_CHANNEL : read pointer overflow" );

            if ( r_sts > 0 )  // at least one filled container
            {
                r_ptr_word               = r_ptr_word + 1;
            }
            else
            {
                assert( "ERROR in NIC_TX_CHANNEL : illegal read request" );
            }
        }
        else if ( cmd_r == TX_CHANNEL_RCMD_LAST )  // read last word in a packet
                                                   // and updates packet index
        {
            assert( (r_ptr_word < NIC_CONTAINER_SIZE) and 
                    "ERROR in NIC_TX_CHANNEL : read pointer overflow" );

            assert( (r_pkt_index < MAX_PACKET) and
                    "ERROR in NIC_TX_CHANNEL : packet index larger than 61" );

            if ( r_sts > 0 )  // at least one filled container
            {
                r_ptr_word               = r_ptr_word + 1;
                r_pkt_index              = r_pkt_index + 1;
                r_ptr_first              = r_ptr_word;
            }
            else
            {
                assert( "ERROR in NIC_TX_CHANNEL : illegal read request" );
            }
        }
        else if ( cmd_r == TX_CHANNEL_RCMD_RELEASE ) // release the current container
        {
            r_pkt_index = 0;
            r_ptr_word  = (MAX_PACKET/2)+1;
            r_ptr_first  = (MAX_PACKET/2)+1;
            memset(r_cont[r_ptr_cont], 0,NIC_CONTAINER_SIZE);
            r_ptr_cont  = (r_ptr_cont + 1) % 2;
            r_sts       = r_sts - 1;
        }

        else if (cmd_r == TX_CHANNEL_RCMD_SKIP) // skip current packet
        {
            uint32_t plen_tmp = this->plen();
            uint32_t words;
            if ( (plen_tmp & 0x3) == 0 ) words = plen_tmp >> 2;
            else                         words = (plen_tmp >> 2) + 1;

            r_ptr_word = r_ptr_first + words ;
            r_pkt_index = r_pkt_index + 1;
            r_ptr_first              = r_ptr_word;

        }
    } // end update()

    /////////////////////////////////////////////////////////////
    // This method returns the current word value in the 
    // current container. It does not modify the channel state.
    /////////////////////////////////////////////////////////////
    uint32_t data()
    { 
        return r_cont[r_ptr_cont][r_ptr_word];
    }

    /////////////////////////////////////////////////////////////
    // This method returns the number of bytes in a packet.
    // It does not modify the channel state.
    /////////////////////////////////////////////////////////////
    uint32_t plen()
    { 
        bool        odd     = (r_pkt_index & 0x1);
        uint32_t    word    = (r_pkt_index / 2) + 1;
        if ( odd ) return (r_cont[r_ptr_cont][word] >> 16);
        else       return (r_cont[r_ptr_cont][word] & 0x0000FFFF);
    }

    /////////////////////////////////////////////////////////////
    // This method returns the number of packets in the 
    // current container. It does not modify the channel state.
    /////////////////////////////////////////////////////////////
    uint32_t npkt()
    { 
        return r_cont[r_ptr_cont][0] & 0x0000FFFF;
    }

    /////////////////////////////////////////////////////////////
    // This method returns true if there is a full container.
    // It does not modify the channel state.
    /////////////////////////////////////////////////////////////
    bool rok()  
    {
        return (r_sts > 0);
    }

    /////////////////////////////////////////////////////////////
    // This method returns true if there is a free container.
    // It does not modify the channel state.
    /////////////////////////////////////////////////////////////
    bool wok()
    {
        return (r_sts < 2);
    }

    //////////////////////////////////////////////////////////////
    // constructor checks parameters and allocates the memory
    // for the containers.
    //////////////////////////////////////////////////////////////
    NicTxChannel( const std::string  &name)
    : m_name(name)
    {
        r_cont    = new uint32_t*[2];
        r_cont[0] = new uint32_t[NIC_CONTAINER_SIZE];
        r_cont[1] = new uint32_t[NIC_CONTAINER_SIZE];
    } 

    //////////////////
    // destructor
    //////////////////
    ~NicTxChannel()
    {
        delete [] r_cont[0];
        delete [] r_cont[1];
        delete [] r_cont;
    }

}; // end NicTxChannel

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



