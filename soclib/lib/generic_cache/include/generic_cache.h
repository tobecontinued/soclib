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
#include "mapping_table.h"

namespace soclib { 

using soclib::common::uint32_log2;

//////////////////////////
template<typename addr_t>
class GenericCache 
//////////////////////////
{

private:

#define CACHE_DATA(way, set, word)  r_data[(way*m_sets*m_words)+(set*m_words)+word]
#define CACHE_TAG(way, set)         r_tag[(way*m_sets)+set]
#define CACHE_VAL(way, set)         r_val[(way*m_sets)+set]
#define CACHE_LRU(way, set)         r_lru[(way*m_sets)+set]

typedef uint32_t    data_t;
typedef uint32_t    tag_t;

data_t              *r_data ;
tag_t               *r_tag ;
bool                *r_val ;
bool                *r_lru ;

size_t              m_words;
size_t              m_sets;	
size_t              m_ways;	

const soclib::common::AddressMaskingTable<addr_t>  m_x ;
const soclib::common::AddressMaskingTable<addr_t>  m_y ;
const soclib::common::AddressMaskingTable<addr_t>  m_z ;

public:

//////////////////////////////////////////////
GenericCache(   const std::string   &name,
                size_t              nways, 
                size_t              nsets, 
                size_t              nwords)
//////////////////////////////////////////////
    :
    m_words(nwords),
    m_sets(nsets),
    m_ways(nways),

	m_x( uint32_log2(nwords), 2),
    m_y( uint32_log2(nsets ), uint32_log2(nwords) + 2),
	m_z( 32, uint32_log2(nsets) + uint32_log2(nwords) + 2)
{
    assert(!((nways) & (nways - 1)));
    assert(!((nsets) & (nsets - 1)));
    assert(!((nwords) & (nwords - 1)));
    assert(nwords);
    assert(nsets);
    assert(nways);
    assert(nwords <= 64);
    assert(nsets <= 1024);
    assert(nways <= 16);

    r_data = new data_t[nways*nsets*nwords];
    r_tag  = new tag_t[nways*nsets];
    r_val  = new bool[nways*nsets];
    r_lru  = new bool[nways*nsets];
}

//////////////////
~GenericCache()
//////////////////
{
    delete [] r_data;
    delete [] r_tag;
    delete [] r_val;
    delete [] r_lru;
}

/////////////////////
inline void reset( )
/////////////////////
{
    for ( size_t n = 0 ; n < (m_sets*m_ways) ; n++ ) {
        r_val[n] = false;
        r_lru[n] = false;
    }
}

//////////////////////////////////////////
inline bool read( addr_t ad, data_t* dt)
//////////////////////////////////////////
{
    bool        hit  = false;
    tag_t       tag  = m_z[ad];
    size_t      set  = m_y[ad];
    size_t      word = m_x[ad];
    for ( size_t way = 0 ; way < m_ways && !hit ; way++ ) {
        if ( (tag == CACHE_TAG(way, set)) && CACHE_VAL(way, set) ) {
            hit = true;
            *dt = CACHE_DATA(way, set, word);
            CACHE_LRU(way, set) = true;
        }
     }
    return hit;
}

///////////////////////////////////////////
inline bool write( addr_t ad, data_t dt )
///////////////////////////////////////////
{
    bool        hit  = false;
    tag_t       tag  = m_z[ad];
    size_t      set  = m_y[ad];
    size_t      word = m_x[ad];
    for ( size_t way = 0 ; way < m_ways && !hit ; way++ ) {
        if ( (tag == CACHE_TAG(way, set)) && CACHE_VAL(way, set) ) {
            hit     = true;
            CACHE_DATA(way, set, word) = dt;
            CACHE_LRU(way, set) = true;
        }
     }
    return hit;
}

///////////////////////////////
inline bool inval( addr_t ad )
///////////////////////////////
{
    bool        hit = false;
    tag_t       tag = m_z[ad];
    size_t      set = m_y[ad];
    for ( size_t way = 0 ; way < m_ways && !hit ; way++ ) {
        if ( (tag == CACHE_TAG(way, set)) && CACHE_VAL(way, set) ) {
            hit     = true;
            CACHE_VAL(way, set) = false;
            CACHE_LRU(way, set) = false;
        }
     }
    return hit;
}

//////////////////////////////////////////////////////////////////////
inline bool update( addr_t ad, data_t* buf, data_t* victim )
/////////////////////////////////////////////////////////////////////
// This function implements a pseudo LRU policy:
// 1 - First we search an invalid way
// 2 - If all ways are valid, we search  the first "non recent" way
// 3 - If all ways are recent, they are all transformed to "non recent"
//   and we select the way with index 0. 
// This function returns true, il a cache line has been removed,
// and the victim line index is returned in the victim parameter.
//////////////////////////////////////////////////////////////////////
{
    tag_t       tag     = m_z[ad];
    size_t      set     = m_y[ad];
    bool        found   = false;
    bool        cleanup = false;
    size_t      selway  = 0;

    for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
        if ( !CACHE_VAL(way, set) ) {
            found   = true;
            cleanup = false;
            selway  = way;
        }
    }
    if ( !found ) { // No invalid way
        for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
            if ( !CACHE_LRU(way, set) ) {
                found   = true;
                cleanup = true;
                selway  = way;
            }
        }
    }
    if ( !found ) { // No old way => all ways become recent
        for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
            CACHE_LRU(way, set) = false;
        }
        cleanup = true;
        selway  = 0;
    }

    *victim = (data_t)((CACHE_TAG(selway, set) * m_sets) + set);
    CACHE_TAG(selway, set) = tag;
    CACHE_VAL(selway, set) = true;
    CACHE_LRU(selway, set) = true;
    for ( size_t word = 0 ; word < m_words ; word++ ) {
        CACHE_DATA(selway, set, word) = buf[word] ;
    }
    return cleanup;
}

}; // end GenericCache

} // end namespace soclib

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

