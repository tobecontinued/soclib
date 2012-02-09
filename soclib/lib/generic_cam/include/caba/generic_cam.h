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
 * Copyright (c) UPMC, Lip6, Asim
 *         Alain Greiner <alain.greiner@lip6.fr>, 2011
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_GENERIC_CAM_H
#define SOCLIB_CABA_GENERIC_CAM_H

#include <systemc>

namespace soclib { namespace caba {

using namespace sc_core;

///////////////////////////////////////////////////////////////
// This hardware component implements a fully associative 
// registration buffer, providing two access functions:
// - register_value()
// - cancel_value()
////////////////////////////////////////////////////////////////

template<typename DataType>
class GenericCam
{
    DataType*   m_value;
    bool*	    m_valid;
    size_t	    m_max;

public:

    //////////////////////////////////////////////////////////////////////////
    // This function try to register a new value in the registration buffer.
    // It checks two conditions, requiring to scan all entries:
    // - the value argument is not already registered 
    // - the registration buffer is not full (at least one empty slot)
    // If both conditions are true, the value argument is registered
    // It returns true, and the slot index in case of success.
    bool    register_value( DataType value,
                            size_t*  index )
    {
        bool    full  = true;
        bool    hit   = false;

        // checks buffer not full
        for ( size_t i=0 ; i<m_max ; i++ )
        {
            if ( not m_valid[i] ) 
            {
                full  = false;
                *index = i;
                break;
            }
        }

        // cheks not hit
        for ( size_t i=0 ; i<m_max ; i++ )
        {
            if ( m_valid[i] and (m_value[i] == value) )
            {
                hit = true;
                break;
            }
        }

        // makes the registration if it's posssible
        if ( not full and not hit ) 
        {
            m_value[*index] = value;
            m_valid[*index] = true;
            return true;
        }
        else
        {
            return false;
        }
    } // end register_value()
    
    ////////////////////////////////////////////////////////////////////
    // this function checks if the index argument is compatible
    // with the buffer size and invalidate the corresponding slot.
    // It returns true in case of success.
    bool cancel_index( size_t index )
    {
        if ( index < m_max )
        {
            m_valid[index] = false;
            return true;
        }
        return false;
    } // end cancel_index()

    ////////////
    void reset()
    {
        for ( size_t i=0 ; i<m_max ; i++ ) m_valid[i] = false;
    }

    ////// constructor //////////////////////////////
    GenericCam( size_t max)
        : m_max(max)
    {
        m_valid = new bool     [m_max];
        m_value = new DataType [m_max];
        for ( size_t i=0 ; i<m_max ; i++ ) m_valid[i] = false;
    }

    ///////////////
    ~GenericCam()
    {
        delete [] m_valid;
        delete [] m_value;
    }
};

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

