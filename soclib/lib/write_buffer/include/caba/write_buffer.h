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
 *         Alain Greiner <alain.greiner@lip6.fr> 2008
 *
 * Maintainers: alain
 */

#ifndef SOCLIB_WRITE_BUFFER_H
#define SOCLIB_WRITE_BUFFER_H

#include <systemc>

namespace soclib { namespace caba {

using namespace sc_core;

//////////////////////////////
template<   typename addr_t,
            typename type_t,
            typename be_t,
            typename data_t,
            typename iss_t>
class WriteBuffer
//////////////////////////////
{
    sc_signal<addr_t>   r_address;      // cache line base address
    sc_signal<size_t>   r_min;          // smallest valid word index
    sc_signal<size_t>   r_max;          // largest valid word index
    sc_signal<bool>     r_empty;        // buffrer empty
    sc_signal<data_t>   *r_data;        // write data  array
    sc_signal<be_t>     *r_be;          // byte enable array

    size_t              m_nwords;       // buffer size (number of words)
    addr_t              m_mask;         // cache line mask
    
public:
    /////////////
    void reset()
    /////////////
    {
        r_max   = 0 ;
        r_min   = m_nwords - 1 ;
        r_empty = true ;
        for( size_t i = 0 ; i < m_nwords ; i++ ) {
            r_be[i] = 0 ;
            r_data[i] = 0 ;
        }
    } 
    ///////////////////////////////////////////////////
    bool wok(addr_t addr)
    ///////////////////////////////////////////////////
    {
        return ( r_empty || (r_address == (addr & ~m_mask)) ) ;
    }
    ///////////////////////////////////////////////////
    void write(addr_t addr, type_t type , data_t  data)
    ///////////////////////////////////////////////////
    {
        size_t byte = (size_t)(addr & 0x3) ;
        size_t word = (size_t)((addr &  m_mask) >> 2) ;
        addr_t line = addr & ~m_mask ;

        if ( r_empty ) {
            r_address = line ;
        } else {
            assert( r_address == line );
        }
        r_empty   = false ;
        if ( type == iss_t::WRITE_WORD ) {
            r_data[word] = data ;
            r_be[word] = 0xF ;
        } else if ( type == iss_t::WRITE_HALF ) {
            data_t mask = 0xFFFF << (byte * 8) ;
            data_t wdata = data << (byte * 8) ;
            r_data[word] = (r_data[word] & ~mask) | (wdata & mask) ;
            r_be[word]   = r_be[word] | 0x3 << byte ;
        } else if ( type == iss_t::WRITE_BYTE ) {
            data_t mask = 0xFF << (byte * 8) ;
            data_t wdata = data << (byte * 8) ;
            r_data[word] = (r_data[word] & ~mask) | (wdata & mask) ;
            r_be[word]   = r_be[word] | 0x1 << byte ;
        } else {
            assert("this case should not happen in the write buffer");
        }
        if ( r_min > word ) r_min = word;
        if ( r_max < word ) r_max = word;
    }
    ///////////////////////////////////
    size_t inline getMin()
    ///////////////////////////////////
    {
        return  r_min;
    }
    ///////////////////////////////////
    size_t inline getMax()
    ///////////////////////////////////
    {
        return  r_max;
    }
    //////////////////////////////////////
    addr_t inline getAddress(size_t word)
    //////////////////////////////////////
    {
        return ( r_address + (addr_t)(word << 2) );
    } 
    ///////////////////////////////////
    data_t inline getData(size_t word)
    ///////////////////////////////////
    {
        return r_data[word];
    } 
    ///////////////////////////////////
    be_t inline getBe(size_t word)
    ///////////////////////////////////
    {
        return r_be[word];
    } 
    //////////////////////////////////////////////////// 
    WriteBuffer(const std::string &name, size_t nwords)
    //////////////////////////////////////////////////// 
        : r_address((name+"_r_address").c_str()),
          r_min((name+"_r_min").c_str()),
          r_max((name+"_r_max").c_str()),
          r_empty((name+"_r_empty").c_str())
    {
          r_data   = new sc_signal<data_t>[nwords];
          r_be     = new sc_signal<be_t>[nwords];
          m_nwords = nwords;
          m_mask   = (addr_t)((nwords << 2) - 1);
    }
    ////////////////
    ~WriteBuffer()
    ////////////////
    {
        delete [] r_data;
        delete [] r_be;
    }
};

}}

#endif /* SOCLIB_WRITE_BUFFER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

