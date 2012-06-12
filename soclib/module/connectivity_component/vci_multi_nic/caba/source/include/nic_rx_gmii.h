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
 *
 * Maintainers: alain 
 */

/*************************************************************************
 * File         : gmii_tx.h
 * Date         : 01/06/2012
 * Authors      : Alain Greiner
 *************************************************************************
 * This object implements a packet receiver, acting as a PHY component,
 * and respecting the GMII protocol (one byte per cycle).
 * It writes packets in a file defined by the "path" constructor argument.
 * The inter-packet gap is another constructor argument.
 *************************************************************************
 * This object has 3 constructor parameters:
 * - string   name  : module name
 * - string   path  : file pathname     
 * - uint32_t gap   : number of cycles between packets
 *************************************************************************/

#ifndef SOCLIB_CABA_GMII_RX_H
#define SOCLIB_CABA_GMII_RX_H

#include <inttypes.h>
#include <systemc>
#include <assert.h>

namespace soclib { 
namespace caba {

using namespace sc_core;

////////////////
class NicRxGmii
{
    // structure constants
    const std::string   m_name;
    const uint32_t	    m_gap;
//  const FILE          m_file;

    // registers
    bool                r_fsm_gap;      // inter_packet state when true
    uint32_t            r_counter;      // cycles counter (used for both gap and plen)
    uint8_t*	        r_buffer;       // local buffer containing one packet
    uint32_t            r_plen;         // packet length (in bytes)

    ///////////////////////////////////////////////////////////////////
    // This function is used to read one packet from the input file
    // It must update the r_buffer and the r_plen variables.
    ///////////////////////////////////////////////////////////////////
    void read_one_packet()
    {
        assert( false and "function read_paket of GMII_RX not defined");
    }

public:

    /////////////
    void reset()
    {
        r_fsm_gap   = true;
        r_counter   = m_gap;
    }

    ///////////////////////////////////////////////////////////////////
    // To reach the 1 Gbyte/s throughput, this method must be called 
    // at all cycles of a 125MHz clock.
    // It is therefore written as a transition and contains a 
    // two states FSM to introduce the inter-packet waiting cycles.
    ///////////////////////////////////////////////////////////////////
    void get( bool*     dv,          // data valid
              bool*     er,          // data error
              uint8_t*  dt )         // data value
    {
        if ( r_fsm_gap )    // inter-packet state
        {
            *dv = false;
            *er = false;
            *dt = 0;

            r_counter = r_counter - 1;

            if (r_counter == 0 ) // end of gap
            {
                read_one_packet();
                r_fsm_gap = false;
            }
        }
        else    // running packet
        {
            *dv = true;
            *er = false;
            *dt = r_buffer[r_counter];
       
            r_counter = r_counter + 1;
 
            if ( r_counter == r_plen ) // end of packet
            {
                r_counter   = m_gap;
                r_fsm_gap   = true;
            }
        }
    } // end get()
                
    //////////////////////////////////////////////////////////////
    // constructor open the file
    //////////////////////////////////////////////////////////////
    NicRxGmii( const std::string  &name,
               const std::string  &path,
               uint32_t           gap )
    : m_name(name),
      m_gap( gap )
    {
        r_buffer        = new uint8_t[2048];
    } // end constructor

    //////////////////
    // destructor)
    //////////////////
    ~NicRxGmii()
    {
        delete [] r_buffer;
    }

}; // end NicRxGmii

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



