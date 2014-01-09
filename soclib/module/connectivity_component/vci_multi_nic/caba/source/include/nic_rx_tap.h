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
 * This object implements a packet receiver, acting as a PHY component.
 * It reads packets from a TAP interface named by the "ifname" constructor argument.
 *************************************************************************
 * This object has 3 constructor parameters:
 * - string   name    : module name
 * - uint32_t gap     : number of cycles between packets
 * - string   ifname  : TAP interface name (optionnal)
 *************************************************************************/

#ifndef SOCLIB_CABA_NIC_RX_TAP_H
#define SOCLIB_CABA_NIC_RX_TAP_H

#if !defined(__APPLE__) || !defined(__MACH__)

#include <inttypes.h>
#include <systemc>
#include <assert.h>

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <errno.h>

// #include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include "nic_rx_backend.h"

namespace soclib {
namespace caba {

using namespace sc_core;

////////////////
class NicRxTap : public NicRxBackend
{
    // structure constants
    const std::string   m_name;
    const uint32_t	    m_gap;
    // std::ifstream       m_ifp;          // named path | NOT USED HERE |
    int                 m_tap_fd;       // File descriptor for the TAP interface
    struct ifreq        *m_tap_ifr;      // TAP interface

    // registers
    bool                r_fsm_gap;      // inter_packet state when true
    int32_t             r_counter;      // cycles counter (used for both gap and plen)
    uint8_t*	        r_buffer;       // local buffer containing one packet
    // uint8_t*	        r_buffer_tmp;   // local buffer containing one packet
    int32_t             r_plen;         // packet length (in bytes)


    ///////////////////////////////////////////////////////////////////
    // This function is used to read one packet from the input file
    // It must update the r_buffer and the r_plen variables.
    ///////////////////////////////////////////////////////////////////
    void read_one_packet()
    {
        if (m_tap_fd > 0)
            {
#ifdef SOCLIB_NIC_DEBUG
                // printf("[NIC][%s] reading from fd : %d\n", __func__, m_tap_fd);
#endif
                r_plen = read(m_tap_fd, r_buffer, 2048);
#ifdef SOCLIB_NIC_DEBUG
                if (errno != EAGAIN)
                    {
                        if (r_plen < 0) // Error during reading
                            {
                                std::cerr << "[NIC]"
                                          << "["
                                          << m_name
                                          << "]["
                                          << __func__
                                          << "] Error reading from TAP";
                                perror(":");
                            }
                        else // Read OK
                            printf("[NIC][NicRxTap][%s] reading plen = %d\n", __func__, r_plen);
                    }
#endif
                // uint32_t cpt = 0;
                // uint32_t data = 0;
                // uint32_t nb_words = 0;
                // uint32_t nb_bytes_available;
                // //string contains a packet
                // std::string string;
                // uint32_t i = 0;

                // m_ifp >> r_plen >> string;
                // nb_bytes_available = r_plen-4;

                // // check end of file and restart it
                // if (m_ifp.eof())
                //     {
                //         m_ifp.clear();
                //         m_ifp.seekg(0, std::ios::beg);
                //         m_ifp >> r_plen >> string;
                //     }
                // convert all the char in string into a hexa
                // for (cpt = 0; cpt < (r_plen << 1) ; cpt++)
                //     {
                //         string[cpt] = atox(string[cpt]);
                //         data = (data << 4)|string[cpt];
                //         if(cpt%2)
                //             {
                //                 r_buffer_tmp[cpt>>1]    = data;
                //                 data = 0;
                //             }
                //     }
                // cpt = 0;


                // while (nb_bytes_available)
                //     {
                //         if (nb_bytes_available > 3)
                //             i = ((nb_words + 1)<<2) - 1;
                //         else
                //             i = ((nb_words + 1)<<2) - (4 - nb_bytes_available) - 1;
                //         while ( i >= (nb_words << 2) )
                //             {
                //                 r_buffer[i] = r_buffer_tmp[cpt];
                //                 nb_bytes_available -- ;
                //                 cpt ++ ;
                //                 if ( i%4 == 0 )
                //                     {
                //                         nb_words ++ ;
                //                         break;
                //                     }
                //                 else i--;
                //             }
                //     }

                // // Return CRC32 because of reverse endianness
                // r_buffer[r_plen-4] = r_buffer_tmp[r_plen-1];
                // r_buffer[r_plen-3] = r_buffer_tmp[r_plen-2];
                // r_buffer[r_plen-2] = r_buffer_tmp[r_plen-3];
                // r_buffer[r_plen-1] = r_buffer_tmp[r_plen-4];
            }
    }

public:

    ///////////////////////////////////////////////////////////////////
    // This function is used to set the value of the TAP file descriptor
    ///////////////////////////////////////////////////////////////////
    void set_fd(int     fd)
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][NicRxTap][%s] fd is : %d\n", __func__, fd);
#endif
        m_tap_fd = fd;
    }

    ///////////////////////////////////////////////////////////////////
    // This function is used to set the value of the TAP file descriptor
    ///////////////////////////////////////////////////////////////////
    int get_fd()
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][NicRxTap][%s]\n", __func__);
#endif
        return m_tap_fd;
    }

    ///////////////////////////////////////////////////////////////////
    // This function is used to set the value of the TAP file descriptor
    ///////////////////////////////////////////////////////////////////
    void set_ifr(struct ifreq     *ifr)
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][NicRxTap][%s]\n", __func__);
#endif
        m_tap_ifr = ifr;
    }

    /////////////
    virtual void reset()
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][NicRxTap][%s] resetting\n", __func__);
#endif
        r_fsm_gap   = true;
        r_counter   = m_gap;
        memset(r_buffer,0,2048);
        // memset(r_buffer_tmp,0,2048);
    }

    ///////////////////////////////////////////////////////////////////
    // To reach the 1 Gbyte/s throughput, this method must be called
    // at all cycles of a 125MHz clock.
    // It is therefore written as a transition and contains a
    // two states FSM to introduce the Inter-Frame Gap waiting cycles.
    ///////////////////////////////////////////////////////////////////
    virtual void get(bool*     dv,         // data valid
                     bool*     er,         // data error
                     uint8_t*  dt)        // data value
    {
        if (r_fsm_gap)    // inter-packet state (IFG or waiting for the next trame)
            {
                *dv = false;
                *er = false;
                *dt = 0;

                if (r_counter == 0) // end of gap - we now wait for the trame to be available
                    {
                        read_one_packet();
                        if (r_plen <= 0) // no trame available on the media
                            r_fsm_gap = true;
                        else
                            r_fsm_gap = false;

                    }
                else
                    r_counter--;
            }
        else    // start or running packet
            {
                *dv = true;
                *er = false;
                *dt = r_buffer[r_counter];

                r_counter++;

                if ( r_counter == r_plen ) // end of packet
                    {
                        r_counter   = m_gap;
                        r_fsm_gap   = true;
                    }
            }
    } // end get()

    /*!
     * \brief This method returns true if the RX chain is to be frozen.
     * \attention This function is only here to reflect the VHDL version of the NIC
     */
    virtual bool frz()
    {
        return false;
    }

    //////////////////////////////////////////////////////////////
    // Constructor
    // Create the TAP interface with name specified by user
    NicRxTap(const std::string  &name,
             uint32_t           gap)
        : m_name(name),
          m_gap(gap),
          m_tap_fd(-1)
    {
#ifdef SOCLIB_NIC_DEBUG
        printf("[NIC][%s] Entering constructor\n", __func__);
#endif
        // r_buffer_tmp    = new uint8_t[2048];
        r_buffer        = new uint8_t[2048];
    } // end constructor

    //////////////////
    // destructor
    //////////////////
    virtual ~NicRxTap()
    {
        delete [] r_buffer;
        // delete [] r_buffer_tmp;
        // m_ifp.close();
    }

}; // end NicRxTap

}}

#endif /* !defined(__darwin__) */

#endif /* SOCLIB_CABA_NIC_RX_TAP_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



