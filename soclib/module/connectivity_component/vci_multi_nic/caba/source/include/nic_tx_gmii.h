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
 * File         : nic_tx_gmii.h
 * Date         : 01/06/2012
 * Authors      : Alain Greiner
 *************************************************************************
 * This object implements a packet transmitter, acting as a PHY component,
 * and respecting the GMII protocol (one byte per cycle).
 * It writes packets in a file defined by the "path" constructor argument.
 *************************************************************************
 * This object has 3 constructor parameters:
 * - string   name    : module name
 * - string   path    : file pathname.
 * - uint32_t gap     : number of cycles between packets
 *************************************************************************/

#ifndef SOCLIB_CABA_GMII_TX_H
#define SOCLIB_CABA_GMII_TX_H

#include <inttypes.h>
#include <systemc>
#include <assert.h>

namespace soclib { 
namespace caba {

using namespace sc_core;

///////////////
class NicTxGmii
{
    // structure constants
    const std::string   m_name;
//  const FILE          m_file;

    // registers
    uint32_t            r_counter;      // cycles counter (used for both gap and plen)
    uint8_t*	        r_buffer;       // local buffer containing one packet

    ///////////////////////////////////////////////////////////////////
    // This function is used to write one packet to the input file
    ///////////////////////////////////////////////////////////////////
    void write_one_packet()
    {
        assert( false and "function write_paket of GMII_TX not defined");
    }

public:

    /////////////
    void reset()
    {
        r_counter = true;
    }

    ///////////////////////////////////////////////////////////////////
    // To reach the 1 Gbyte/s throughput, this method must be called 
    // at all cycles of a 125MHz clock.
    ///////////////////////////////////////////////////////////////////
    void put( bool     dv,          // data valid
              bool     er,          // data error
              uint8_t  dt )         // data value
    {
        if ( not dv and (r_counter != 0) )    // end of packet
        {
            write_one_packet();
            r_counter = 0;
        }
        else if ( dv )    // running packet
        {
            r_buffer[r_counter] = dt;
            r_counter           = r_counter + 1;
        }
    } // end put()
                
    //////////////////////////////////////////////////////////////
    // constructor open the file
    //////////////////////////////////////////////////////////////
    NicTxGmii( const std::string  &name,
               const std::string  &path )
    : m_name(name)
    {
        r_buffer        = new uint8_t[2048];
    } 

    //////////////////
    // destructor
    //////////////////
    ~NicTxGmii()
    {
        delete [] r_buffer;
    }

}; // end GmiiTx

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



