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
/////////////////////////////////////////////////////////////////////////////////
// History :
// 05/01/2010
// Two new methods select_before_update(), and update_after_select() have
// been introduced to support update in two cycles, when the cache directory
// is implemented as a simple port RAM.
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
    size_t              m_next_way;

    const soclib::common::AddressMaskingTable<addr_t>  m_x ;
    const soclib::common::AddressMaskingTable<addr_t>  m_y ;
    const soclib::common::AddressMaskingTable<addr_t>  m_z ;

    //////////////////////////////////////////////////////////////
    inline data_t &cache_data(size_t way, size_t set, size_t word)
    {
        return r_data[(way*m_sets*m_words)+(set*m_words)+word];
    }

    //////////////////////////////////////////////
    inline tag_t &cache_tag(size_t way, size_t set)
    {
        return r_tag[(way*m_sets)+set];
    }

    //////////////////////////////////////////////
    inline bool &cache_val(size_t way, size_t set)
    {
        return r_val[(way*m_sets)+set];
    }

    //////////////////////////////////////////////
    inline bool &cache_lru(size_t way, size_t set)
    {
        return r_lru[(way*m_sets)+set];
    }

    //////////////////////////////////////////////
    inline void cache_set_lru(size_t way, size_t set)
    {
	size_t way2;

        cache_lru(way, set) = true;
	for (way2 = 0; way2 < m_ways; way2++ ) {
	    if (cache_lru(way2, set) == false) return;
	}
	/* all lines are new -> they all become old */
	for (way2 = 0; way2 < m_ways; way2++ ) {
	    cache_lru(way2, set) = false;
	}
    }

public:

    //////////////////////////////////////////////
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
        m_next_way = 0;
    }

    ////////////////
    ~GenericCache()
    {
        delete [] r_data;
        delete [] r_tag;
        delete [] r_val;
        delete [] r_lru;
    }

    ////////////////////
    inline void reset( )
    {
        std::memset(r_data, 0, sizeof(*r_data)*m_ways*m_sets*m_words);
        std::memset(r_tag, 0, sizeof(*r_tag)*m_ways*m_sets);
        std::memset(r_val, 0, sizeof(*r_val)*m_ways*m_sets);
        std::memset(r_lru, 0, sizeof(*r_lru)*m_ways*m_sets);
    }

    ////////////////////////////////////////////////////////
    inline bool flush(size_t way, size_t set, addr_t* nline)
    {
        if ( cache_val(way,set) ) 
        {
            cache_val(way,set) = false;
            *nline = (data_t)cache_tag(way,set)* m_sets + set;
            return true;
        }
        return false;
    }

    /////////////////////////////////////////
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
                cache_set_lru(way, set);

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

    /////////////////////////////////////////////////////////////////////
    inline bool read( addr_t ad, data_t* dt, size_t* n_way, size_t* n_set)
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
                cache_set_lru(way, set);
                *n_way = way;
                *n_set = set;

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

    //////////////////////////////////////////////////////
    inline bool setinbit( addr_t ad, bool* buf, bool val )
    {
        const tag_t       tag  = m_z[ad];
        const size_t      set  = m_y[ad];

        for ( size_t way = 0; way < m_ways; way++ ) {
            if ( (tag == cache_tag(way, set)) && cache_val(way, set) ) {
                buf[m_sets*way+set] = val;

#ifdef GENERIC_CACHE_DEBUG
                std::cout << "hit"
                          << " w/s=" << way << '/' << set 
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

    ////////////////////////////////////////////
    inline tag_t get_tag(size_t way, size_t set)
    {
        return cache_tag(way, set);
    }

    /////////////////////////////////////////
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
                cache_set_lru(way, set);

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

    /////////////////////////////////////////////////////////////////////
    inline bool write( addr_t ad, data_t dt, size_t* nway, size_t* nset )
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
                cache_set_lru(way, set);
                *nway = way;
                *nset = set;

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

    inline void write(size_t way, size_t set, size_t word, data_t data)
    {
        cache_data(way, set, word) = data;
    }

    //////////////////////////////
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

    ////////////////////////////////////////////////////////////
    inline bool inval( addr_t ad, size_t* n_way, size_t* n_set )
    {
        bool    hit = false;
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
                *n_way = way;
                *n_set = set;

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

    ///////////////////////////////////////////////////////////////////
    // This function implements a pseudo LRU policy for a one cycle
    // line replacement. It can be used if the directory is implemented
    // as a dual port RAM.
    // 1 - First we search an invalid way
    // 2 - Il all ways are valid, we search the first old way
    // 3 - If all ways are recent, they are all transformed to old.
    //     and we select the way with index 0.
    // The victim line index is returned in the victim parameter.
    // This function returns true, if the victim line is valid.
    ///////////////////////////////////////////////////////////////////
    inline bool update( addr_t ad, data_t* buf, addr_t* victim )
    {
        size_t set, way;
        bool   cleanup = victim_select(ad, victim, &way, &set);
        victim_update_tag (ad, way, set);

        for ( size_t word = 0 ; word < m_words ; word++ ) {
            cache_data(way, set, word) = buf[word] ;
        }

        return cleanup;
    }

    inline bool victim_select( addr_t ad, addr_t* victim, size_t * way, size_t * set )
    {
        bool   found   = false;
        bool   cleanup = false;
        *set = m_y[ad];
        *way = 0;

        // Schearch and invalid slot
        for ( size_t _way = 0 ; _way < m_ways && !found ; _way++ )
        {
            if ( !cache_val(_way, *set) )
            {
                found   = true;
                cleanup = false;
                *way    = _way;
            }
        }

        // No invalid way, scearch the lru
        if ( !found )
        { 
            for ( size_t _way = 0 ; _way < m_ways && !found ; _way++ )
            {
                if ( !cache_lru(_way, *set) )
                {
                    cache_val (_way, *set) = false;
                    found   = true;
                    cleanup = true;
                    *way    = _way;
                }
            }
        }

        assert(found && "all ways can't be new at the same time");

        *victim = (addr_t)((cache_tag(*way,*set) * m_sets) + *set);

        return cleanup;
    }

    inline void victim_update_tag( addr_t ad, size_t way, size_t set )
    {
        tag_t  tag     = m_z[ad];

        cache_tag    (way, set) = tag;
        cache_val    (way, set) = true;
        cache_set_lru(way, set);
    }

/*
    //////////////////////////////////////////////////////////////////////////////
    // The two functions select_before_update() & update_after_select()
    // can be used to perform a line replacement in two cycles
    // (when the directory is implemented as a single port RAM)
    //
    // The select_before_update function implements a pseudo LRU
    // policy and and returns the victim line index and the selected 
    // way in the return arguments. The selected cache line is invalidated.
    // This function returns true, il the victim line is valid,
    ////////////////////////////////////////////////////////////////////////////////
    inline bool select_before_update( addr_t ad, size_t* selway, addr_t* victim )
    {
        size_t      set     = m_y[ad];
        // search an empty slot (using valid bit)
        for ( size_t way = 0 ; way < m_ways ; way++ ) 
        {
            if ( !cache_val(way, set) ) 
            {
                *selway = way;
                *victim = (addr_t)((cache_tag(way, set) * m_sets) + set);
                return  false;
            }
        }
        // search an old line (using lru bit)
        for ( size_t way = 0 ; way < m_ways ; way++ ) 
        {
            if ( !cache_lru(way, set) ) 
            {
                cache_val(way, set) = false;
                *selway = way;
                *victim = (addr_t)((cache_tag(way, set) * m_sets) + set);
                return  true;
            }
        }
        assert("all lines can't be new at the same time");
        return true;
    }

    /////////////////////////////////////////////////////////////////////
    // The two functions select_before_update() & update_after_select()
    // can be used to perform a line replacement in two cycles
    // (when the directory is implemented as a single port RAM)
    //
    // The update_after select() function performs the actual
    // update of the cache line.
    /////////////////////////////////////////////////////////////////////
    inline void update_after_select( data_t* buf, size_t way, addr_t ad)
    {
        tag_t       tag     = m_z[ad];
        size_t      set     = m_y[ad];

        cache_tag(way, set) = tag;
        cache_val(way, set) = true;
        cache_set_lru(way, set);
        for ( size_t word = 0 ; word < m_words ; word++ ) cache_data(way, set, word) = buf[word] ;
    }
*/
    ///////////////////////////
    inline bool find( addr_t ad, 
                      bool* itlb_buf, bool* dtlb_buf,
                      size_t* n_way, size_t* n_set, 
                      addr_t* victim )
    {
        size_t      set     = m_y[ad];
        size_t      selway  = 0;
        bool        found   = false;
        bool        cleanup = false;

        for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
            if ( !cache_val(way, set) ) {
                found   = true;
                cleanup = false;
                selway  = way;
            }
        }
        if ( !found ) {
	    /* No invalid way, look for an old way, in priotity order:
	     * an old way which is not refereced by any tlb
	     * an old way which is not referenced by the itlb
	     * an old way which is not referenced by the dtlb
	     * an old way 
	     */
            for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
                if ( !cache_lru(way, set) && !itlb_buf[m_sets*way+set]
		    && !dtlb_buf[m_sets*way+set]) {
                    found   = true;
                    cleanup = true;
                    selway  = way;
                }
            }
            for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
                if ( !cache_lru(way, set) && !itlb_buf[m_sets*way+set]) {
                    found   = true;
                    cleanup = true;
                    selway = way;
                }
            }
            for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
                if ( !cache_lru(way, set) && !dtlb_buf[m_sets*way+set]) {
                    found   = true;
                    cleanup = true;
                    selway = way;
                }
            }
            for ( size_t way = 0 ; way < m_ways && !found ; way++ ) {
                if ( !cache_lru(way, set)) {
                    found   = true;
                    cleanup = true;
                    selway = way;
                }
            }
        }
	assert(found && "all ways can't be new at the same time");
        *victim = (addr_t)((cache_tag(selway, set) * m_sets) + set);
        cache_val(selway, set) = false;
        *n_way = selway;
        *n_set = set;
        return cleanup;
    }

    /////////////////////////////////////////////////////////////////////////
    inline void update( addr_t ad, size_t n_way, size_t n_set, data_t* buf )
    {
        tag_t tag = m_z[ad];

        cache_tag(n_way, n_set) = tag;
        cache_val(n_way, n_set) = true;
        cache_set_lru(n_way, n_set);
        for ( size_t word = 0 ; word < m_words ; word++ ) {
            cache_data(n_way, n_set, word) = buf[word] ;
        }
    }

    ///////////////////////////////////////////////////////////////////
    // cleanupcheck function is used for checking whether a line exists
    inline bool cleanupcheck( addr_t ad )
    {
        const tag_t       tag  = m_z[ad];
        const size_t      set  = m_y[ad];

        for ( size_t way = 0; way < m_ways; way++ ) {
            if ( (tag == cache_tag(way, set)) && cache_val(way, set) ) {
                return true;
            }
        }
        return false;
    }

    ///////////////////////////
    void fileTrace(FILE* file)
    {
        for( size_t nway = 0 ; nway < m_ways ; nway++) 
        {
            for( size_t nset = 0 ; nset < m_sets ; nset++) 
            {
                if( cache_val(nway, nset) ) fprintf(file, " V / ");
                else                        fprintf(file, "   / ");
                fprintf(file, "way %d / ", (int)nway);
                fprintf(file, "set %d / ", (int)nset);
                fprintf(file, "@ = %08X / ", ((cache_tag(nway, nset)*m_sets+nset)*m_words*4));
                for( size_t nword = m_words ; nword > 0 ; nword--) 
                {
                    unsigned int data = cache_data(nway, nset, nword-1);
                    fprintf(file, "%08X ", data );
                }
                fprintf(file, "\n");
            }
        }
    }

    ////////////////////////
    inline void printTrace()
    {
        for ( size_t way = 0; way < m_ways ; way++ ) 
        {
            for ( size_t set = 0 ; set < m_sets ; set++ )
            {
                if ( cache_val(way,set) ) std::cout << "  * " ;
                else                      std::cout << "    " ;
                std::cout << std::dec << "way " << way << std::hex << " | " ;
                std::cout << "@ " << (cache_tag(way,set)*m_words*m_sets+m_words*set)*4 ;
                for ( size_t word = 0 ; word < m_words ; word++ )
                {
                    std::cout << " | " << cache_data(way,set,word) ;
                }
                std::cout << std::endl ;
            }
        }
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

