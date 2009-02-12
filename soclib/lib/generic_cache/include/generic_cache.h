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
 * Maintainers: alain yang
 */

//////////////////////////////////////////////////////////////////////////////////
// This object is a generic, set associative, cache providing read(), write(),
// inval() and update() access primitives. The replacement policy is pseudo-LRU.
// The DATA part is implemented as an int32_t array.
// The DIRECTORY part is implemented as an uint32_t array.
//
// Constructor parameters are :
// - std::string    &name
// - size_t         nways   : number of associativity levels 
// - size_t         nsets   : number of sets
// - size_t         nwords  : number of words in a cache line
// The nways, nsets, nwords parameters must be power of 2
// The nsets parameter cannot be larger than 1024
// The nways parameter cannot be larger than 16
// The nwords parameter cannot be larger than 64
//
// Template parameter is :
// - addr_t : address format to access the cache 
// The address can be larger than 32 bits, but the TAG field 
// cannot be larger than 32 bits.
// 
/////////////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_GENERIC_CACHE_H
#define SOCLIB_GENERIC_CACHE_H

#include <systemc>
#include <cassert>
#include "arithmetics.h"
#include "static_assert.h"
#include "mapping_table.h"
#include <cstring>

namespace soclib { 

//////////////////////////
template<typename addr_t>
class GenericCache 
//////////////////////////
{
    typedef uint32_t    data_t;
    typedef uint32_t    tag_t;

    data_t              *r_data ;
    tag_t               *r_tag ;
    bool                *r_val ;
    bool                *r_lru ;

    size_t              m_ways;	
    size_t              m_sets;	
    size_t              m_words;

    const soclib::common::AddressMaskingTable<addr_t>  m_x ;
    const soclib::common::AddressMaskingTable<addr_t>  m_y ;
    const soclib::common::AddressMaskingTable<addr_t>  m_z ;

    inline data_t &cache_data(size_t way, size_t set, size_t word)
    {
        return r_data[(way*m_sets*m_words)+(set*m_words)+word];
    }

    inline tag_t &cache_tag(size_t way, size_t set)
    {
        return r_tag[(way*m_sets)+set];
    }

    inline bool &cache_val(size_t way, size_t set)
    {
        return r_val[(way*m_sets)+set];
    }

    inline bool &cache_lru(size_t way, size_t set)
    {
        return r_lru[(way*m_sets)+set];
    }

public:

    GenericCache(   const std::string   &name,
                    size_t              nways, 
                    size_t              nsets, 
                    size_t              nwords)
        : m_ways(nways),
          m_sets(nsets),
          m_words(nwords),

#define l2 soclib::common::uint32_log2

          m_x( l2(nwords), l2(sizeof(data_t))),
          m_y( l2(nsets), l2(nwords) + l2(sizeof(data_t))),
          m_z( 8*sizeof(addr_t) - l2(nsets) - l2(nwords) - l2(sizeof(data_t)),
               l2(nsets) + l2(nwords) + l2(sizeof(data_t)))

#undef l2
    {
        assert(IS_POW_OF_2(nways));
        assert(IS_POW_OF_2(nsets));
        assert(IS_POW_OF_2(nwords));
        assert(nwords);
        assert(nsets);
        assert(nways);
        assert(nwords <= 64);
        assert(nsets <= 1024);
        assert(nways <= 16);

#ifdef GENERIC_CACHE_DEBUG
        std::cout
            << " m_x: " << m_x
            << " m_y: " << m_y
            << " m_z: " << m_z
            << std::endl;
#endif

        r_data = new data_t[nways*nsets*nwords];
        r_tag  = new tag_t[nways*nsets];
        r_val  = new bool[nways*nsets];
        r_lru  = new bool[nways*nsets];
    }

    ~GenericCache()
    {
        delete [] r_data;
        delete [] r_tag;
        delete [] r_val;
        delete [] r_lru;
    }

    inline void reset( )
    {
        std::memset(r_data, 0, sizeof(*r_data)*m_ways*m_sets*m_words);
        std::memset(r_tag, 0, sizeof(*r_tag)*m_ways*m_sets);
        std::memset(r_val, 0, sizeof(*r_val)*m_ways*m_sets);
        std::memset(r_lru, 0, sizeof(*r_lru)*m_ways*m_sets);
    }

    inline bool read( addr_t ad, data_t* dt)
    {
        const tag_t       tag  = m_z[ad];
        const size_t      set  = m_y[ad];
        const size_t      word = m_x[ad];
#ifdef GENERIC_CACHE_DEBUG
        std::cout << "Reading data at " << ad << ", "
                  << " s/t=" << set << '/' << tag
                  << ", ";
#endif
        for ( size_t way = 0; way < m_ways; way++ ) {
            if ( (tag == cache_tag(way, set)) && cache_val(way, set) ) {
                *dt = cache_data(way, set, word);
                cache_lru(way, set) = true;
#ifdef GENERIC_CACHE_DEBUG
                std::cout << "hit"
                          << " w/s/t=" << way << '/' << set << '/' << tag
                          << " = " << *dt << std::endl;
#endif
                return true;
            }
        }
#ifdef GENERIC_CACHE_DEBUG
        std::cout << "miss" << std::endl;
#endif
        return false;
    }

    inline bool write( addr_t ad, data_t dt )
    {
        const tag_t       tag  = m_z[ad];
        const size_t      set  = m_y[ad];
        const size_t      word = m_x[ad];
#ifdef GENERIC_CACHE_DEBUG
        std::cout << "Writing data at " << ad << ", "
                  << " s/t=" << set << '/' << tag
                  << ", ";
#endif
        for ( size_t way = 0; way < m_ways; way++ ) {
            if ( (tag == cache_tag(way, set)) && cache_val(way, set) ) {
                cache_data(way, set, word) = dt;
                cache_lru(way, set) = true;
#ifdef GENERIC_CACHE_DEBUG
                std::cout << "hit"
                          << " w/s/t=" << way << '/' << set << '/' << tag
                          << std::endl;
#endif
                return true;
            }
        }
#ifdef GENERIC_CACHE_DEBUG
        std::cout << "miss" << std::endl;
#endif
        return false;
    }

    inline bool inval( addr_t ad )
    {
        bool        hit = false;
        const tag_t       tag = m_z[ad];
        const size_t      set = m_y[ad];
#ifdef GENERIC_CACHE_DEBUG
        std::cout << "Invalidating data at " << ad << ", "
                  << " s/t=" << set << '/' << tag
                  << ", ";
#endif
        for ( size_t way = 0 ; way < m_ways && !hit ; way++ ) {
            if ( (tag == cache_tag(way, set)) && cache_val(way, set) ) {
                hit     = true;
                cache_val(way, set) = false;
                cache_lru(way, set) = false;
#ifdef GENERIC_CACHE_DEBUG
                std::cout << "hit"
                          << " w/s/t=" << way << '/' << set << '/' << tag
                          << " ";
#endif
            }
        }
#ifdef GENERIC_CACHE_DEBUG
        std::cout << std::endl;
#endif
        return hit;
    }

  	/// This function implements a pseudo LRU policy:
    // 1 - First we search an invalid way
    // 2 - If all ways are valid, we search  the first "non recent" way
    // 3 - If all ways are recent, they are all transformed to "non recent"
    //   and we select the way with index 0. 
    // This function returns true, il a cache line has been removed,
    // and the victim line index is returned in the victim parameter.
    inline bool update( addr_t ad, data_t* buf, addr_t* victim )
    {
        tag_t       tag     = m_z[ad];
        size_t      set     = m_y[ad];
        bool        found   = false;
        bool        cleanup = false;
        size_t      selway  = 0;

        for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
            if ( !cache_val(way, set) ) {
                found   = true;
                cleanup = false;
                selway  = way;
            }
        }
        if ( !found ) { // No invalid way
            for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
                if ( !cache_lru(way, set) ) {
                    found   = true;
                    cleanup = true;
                    selway  = way;
                }
            }
        }
        if ( !found ) { // No old way => all ways become recent
            for ( size_t way = 0; way < m_ways; way++ ) {
                cache_lru(way, set) = false;
            }
            cleanup = true;
            selway  = 0;
        }

        *victim = (addr_t)((cache_tag(selway, set) * m_sets) + set);
        cache_tag(selway, set) = tag;
        cache_val(selway, set) = true;
        cache_lru(selway, set) = true;
        for ( size_t word = 0 ; word < m_words ; word++ ) {
            cache_data(selway, set, word) = buf[word] ;
        }
        return cleanup;
    }

};

} // namespace soclib

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

