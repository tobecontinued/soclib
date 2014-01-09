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
 *         Sylvain Leroy <sylvain.leroy@lip6.fr>
 *
 * Maintainers: sylvain
 */

/*************************************************************************
 * This object implements a packet transmitter, acting as a PHY component,
 * and respecting the GMII protocol (one byte per cycle).
 * It writes packets in a file defined by the "path" constructor argument.
 *************************************************************************
 * This object has 3 constructor parameters:
 * - string   name    : module name
 * - string   path    : file pathname.
 * - uint32_t gap     : number of cycles between packets
 *************************************************************************/

#ifndef SOCLIB_CABA_TX_TAP_H
#define SOCLIB_CABA_TX_TAP_H

#if !defined(__APPLE__) || !defined(__MACH__)

#include <inttypes.h>
#include <systemc>
#include <assert.h>

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>

#include "nic_tx_backend.h"

namespace soclib { 
namespace caba {

using namespace sc_core;


///////////////
#define NIC_TX_TAP_BUFSIZE      2048

// If this define is set to true, NO CRC32 will be sent through the TAP interface
// If this define is set to false, CRC32 will be sent through the TAP interface
#define NIC_TX_NO_CRC32         false

#ifdef NIC_TX_NO_CRC32
#define NIC_TX_CRC32_SIZE     4
#else
#define NIC_TX_CRC32_SIZE     0
#endif

///////////////
class NicTxTap : public NicTxBackend
{
    // structure constants
    const std::string   m_name;
    // std::ofstream       m_file;
    int32_t             m_tap_fd;       // File descriptor for the TAP interface
    struct ifreq        *m_tap_ifr;      // TAP interface

    // registers
    uint32_t            r_counter;      // cycles counter (used for both gap and plen)
    uint8_t*	        r_buffer;       // local buffer containing one packet
    

    ///////////////////////////////////////////////////////////////////
    // This function is used to write one packet to the input file
    ///////////////////////////////////////////////////////////////////
    void write_one_packet()
    {
        if (m_tap_fd)
            {
#ifdef SOCLIB_NIC_DEBUG
                printf("[NIC][NicTxTap][%s] writing 1 packet of %u bytes on TAP fd %d :\n", __func__, r_counter, m_tap_fd);
#endif
#ifdef SOCLIB_NIC_DEBUG
                // Printing the actuel buffer internals values
                // -4 is there to remove the Ethernet CRC32
                for (size_t i = 0; i < r_counter - NIC_TX_CRC32_SIZE; i++) // NIC_TX_TAP_BUFSIZE here if we want to see full buffer
                    {
                        if (i != 0)
                            {
                                if ((i % 4) == 0)
                                    printf(" ");
                                if ((i % 72) == 0)
                                    printf("\n");
                            }
                        printf("%02x", r_buffer[i]);
                    }
                printf("\n");
#endif
                // r_buffer[r_counter - 1] = 0x00;
                // r_buffer[r_counter - 2] = 0x00;
                // r_buffer[r_counter - 3] = 0x00;
                // r_buffer[r_counter - 4] = 0x00;
                // Writing to the TAP interface
                write(m_tap_fd, r_buffer, r_counter - NIC_TX_CRC32_SIZE);
            }
    }

public:

    ///////////////////////////////////////////////////////////////////
    // This function is used to set the value of the TAP file descriptor
    ///////////////////////////////////////////////////////////////////
    void set_fd(int     fd)
    {
        m_tap_fd = fd;
    }

    ///////////////////////////////////////////////////////////////////
    // This function is used to set the value of the TAP file descriptor
    ///////////////////////////////////////////////////////////////////
    void set_ifr(struct ifreq     *ifr)
    {
        m_tap_ifr = ifr;
    }

    /////////////
    virtual void reset()
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][NicTxTap][%s] resetting\n", __func__);
#endif
        r_counter = 0;
        memset(r_buffer, 0, NIC_TX_TAP_BUFSIZE);
    }

    ///////////////////////////////////////////////////////////////////
    // To reach the 1 Gbyte/s throughput, this method must be called 
    // at all cycles of a 125MHz clock.
    ///////////////////////////////////////////////////////////////////
    virtual void put(bool     dv,          // data valid
                     uint8_t  dt)         // data value
    {
        if (not dv and (r_counter != 0))    // end of packet
            {
                write_one_packet();
                r_counter = 0;
            }
        else
            {
                if (dv)    // start or running packet
                    {
#ifdef SOCLIB_NIC_DEBUG
                        printf("[NIC][NicTxTap][%s] writing 0x%02x in r_buffer[%u]\n", __func__, dt, r_counter);
#endif
                        r_buffer[r_counter] = dt;
                        r_counter           = r_counter + 1;
                    }
            }
    } // end put()

    /*!
     * \brief This method returns true if the RX chain is to be frozen.
     * \attention This function is only here to reflect the VHDL version of the NIC
     */
    virtual bool frz()
    {
        return false;
    }
                
    //////////////////////////////////////////////////////////////
    // constructor
    //////////////////////////////////////////////////////////////
    NicTxTap( const std::string  &name)
        : m_name(name)
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][%s] Entering constructor\n", __func__);
#endif
        r_buffer        = new uint8_t[NIC_TX_TAP_BUFSIZE];
    } 

    //////////////////
    // destructor
    //////////////////
    virtual ~NicTxTap()
    {
        delete [] r_buffer;
    }

}; // end NicTxTap

}}

#endif /* !defined(__darwin__) */
#endif /* SOCLIB_CABA_TX_TAP_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



