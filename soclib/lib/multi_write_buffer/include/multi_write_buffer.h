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

////////////////////////////////////////////////////////////////////////////
// This Write Buffer, supports several simultaneous write bursts.
// It contains an integer number of slots, and each slot can contain
// one buffer line. The m_nwords parameter defines the slot width.
// (max length of a write burst). It must be a power of 2.
// The m_nslots parameter defines the max number of concurrent transactions.
// All word adresses in a buffer slot are contiguous, and the
// slot base address is aligned on a buffer line boundary.
// One buffer line is typically a cache line, or half a cache line.
// Byte enable controlled "write after write" are supported
// in the same line, and in the same word.
// All write requests in the same buffer slot will be transmitted
// as a single VCI transaction.
// This write buffer can be seen as a set of FSM : one FSM per slot.
// A slot can be in four states : EMPTY, OPEN, LOCKED, SENT.
// Each slot has a local timer that is initialised when the
// slot goes to OPEN. For all OPEN slots, the timer must be decremented 
// at each cycle by the update() method (that must called at all cycles).
// An OPEN slot goes to LOCKED state when the timer reach the 0 value,
// or if a global flush is requested by the update() argument.
// - The write() method is used to store a write request in the buffer.
//   it returns false if the buffer is full. 
//   If previously EMPTY, the selected slot state goes to OPEN.
// - The rok() method returns true when there is at least one LOCKED slot
//   that can be sent, and updates the read pointer.
// - The sent() method must be used when the last flit of a write burst 
//   transaction has been send, to switch the slot state to SENT.
// - The completed() method is used to signal that a write transaction
//   has been completed, and to reset to EMPTY the corresponding slot.
// - The empty() method returns true when all slots are empty.
// - The miss(address) method can be used to chek that a read request does 
//   not match a pending write transaction. The matching criteria is the buffer
//   line : the word index in the buffer slot is ignored.
// - This write buffer enforces the write after write policy :
//   An open slot is locked only if there is no other pending write
//   transaction in the same buffer line.
// - The general write policy is associative (when looking for an empty slot)
//   but the read policy uses a round robin pointer.
// - It can exist several EMPTY slots, several LOCKED slots,
//   several SENT slots, and several OPEN slot.
////////////////////////////////////////////////////////////////////////////
// User note :
// The update() method must be called at all cycles by the transition 
// function of the hardware component that contains this write buffer
// to update the internal state.
////////////////////////////////////////////////////////////////////////////
// It has 4 constructor parameters :
// - std::string    name
// - size_t         nwords  : buffer width (number of words)
// - size_t	    nslots  : buffer depth (number of slots)
// - size_t	    timeout : max life time for an open slot
// It has one template parameter :
// - addr_t defines the address format
/////////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_MULTI_WRITE_BUFFER_H
#define SOCLIB_MULTI_WRITE_BUFFER_H

#include <systemc>
#include <cassert>

namespace soclib { 

using namespace sc_core;

enum slot_state_e {
	EMPTY,
	OPEN,
	LOCKED,
	SENT,
	};

//////////////////////////////
template<typename addr_t>
class MultiWriteBuffer
//////////////////////////////
{
    typedef uint32_t    data_t;
    typedef uint32_t    be_t;

    sc_signal<size_t>	r_ptr;			// next slot to be read
    sc_signal<int>      *r_state;       	// slot state array[nslots]
    sc_signal<addr_t>  	*r_address;     	// slot base address array[nslots]
    sc_signal<size_t>  	*r_tout;        	// slot timeout array[nslots]
    sc_signal<size_t>  	*r_min;         	// smallest valid word index array[nslots]
    sc_signal<size_t>  	*r_max;         	// largest valid word index array[nslots]
    data_t   		**r_data;       	// write data  array[nslots][nwords]
    be_t     		**r_be;         	// byte enable array[nslots][nwords]

    size_t              m_nslots;       	// buffer depth (number of slots)
    size_t              m_nwords;       	// buffer width (number of words)
    size_t		m_timeout;		// max life time for an open slot
    addr_t              m_wbuf_line_mask;       // Mask for a write buffer line
    addr_t              m_cache_line_mask;      // Mask for a cache line
 
public:

    /////////////
    void reset()
    {
        r_ptr = 0 ;
	for( size_t slot = 0 ; slot < m_nslots ; slot++) {
            r_address[slot] 	= 0 ;
            r_tout[slot] 	= 0 ;
            r_max[slot]   	= 0 ;
            r_min[slot]   	= m_nwords - 1 ;
            r_state[slot] 	= EMPTY ;
            for( size_t word = 0 ; word < m_nwords ; word++ ) {
                r_be[slot][word] 	= 0 ;
                r_data[slot][word] 	= 0 ;
            }
        }
    } 

    ////////////////////////
    inline void printTrace()
    {
        const char *wbuf_state_str[] = { "EMPTY ", "OPEN  ", "LOCKED", "SENT  " };

        std::cout << "  Write Buffer : ptr = " << r_ptr << std::endl;
        for( size_t i = 0 ; i < m_nslots ; i++ )
        {
            std::cout << "  LINE " << i << " : " 
                      << wbuf_state_str[r_state[i]] 
                      << std::hex << " address = " << r_address[i].read() 
                      << " min = " << r_min[i].read()
                      << " max = " << r_max[i].read()
                      << " tout = " << r_tout[i].read() << std::endl;
            for( size_t w = 0 ; w < m_nwords ; w++ )
            {
                std::cout << "  / D" << std::dec << w << " = " 
                          << std::hex << r_data[i][w] << "  be = " << r_be[i][w] ;
            }
            std::cout << std::endl;
        }
    }

    /////////////////////////////////////////////////////////
    inline bool miss(addr_t addr)
    // This method is intended to be used by the VCI_CMD FSM
    // to comply with the read after write policy, and
    // decide if a read miss transaction can be launched.
    // There is an hardware cost associated with this service,
    // because all buffer entries must be tested. 
    {
        for( size_t i = 0 ; i < m_nslots ; i++ )
        {
            if ( (r_state[i].read() != EMPTY) && 
                 ((r_address[i].read() & ~m_cache_line_mask) == (addr & ~m_cache_line_mask)) ) 
                    return false;
        }
        return true;
    }
    
    ///////////////////////////////////////////////////
    inline bool empty( )
    {
        for( size_t i=0 ; i<m_nslots ; i++ )
        {
            if ( r_state[i].read() != EMPTY ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////
    inline bool rok(size_t* min, size_t* max)
    // This method is intended to be called by the VCI_CMD FSM,
    // to test if a locked slot is available.
    // It changes the pointer to the next available locked slot, 
    // and returns the min & max indexes when it has been found.
    {
	bool	found = false;	
        size_t  lw;
        for( size_t i=0 ; i<m_nslots ; i++)
        {
            lw = (r_ptr+i)%m_nslots;
            if( r_state[lw] == LOCKED ) 
            { 
                found = true;
                r_ptr = lw;
                *min = r_min[lw];
                *max = r_max[lw];
                break;
            }
        }
        return found;
    } // end rok()

    ////////////////////////////////////////////////////////////////////
    void inline sent()  
    // This method is intended to be used by the VCI_CMD FSM.
    // It can only change a slot state from LOCKED to SENT when
    // the corresponding write command has been fully transmitted.
    {
        assert( (r_state[r_ptr] == LOCKED) &&
             "write buffer error : illegal sent command received");
        r_state[r_ptr] = SENT;
    } 

    /////////////////////////////////////////////////////////////////////
    void update(bool flush)
    // This method is intended to be called at each cycle.
    // It can change slots state from OPEN to LOCKED.
    // If the flush argument is true, all open slots become LOCKED.
    // If not, it has two different actions :
    // - The tout value is decremented for all open slots
    // that have a non zero tout value.
    // - If there is one open slot with tout == 0, and there is 
    // no pending slot (locked or sent) with the same address,
    // this slot is locked : at most one locked slot per cycle,
    // because the hardware cost is high...
    {
        if(flush)
        {
            for(size_t i=0 ; i<m_nslots ; i++)
            {
                if( r_state[i] == OPEN )   r_state[i] = LOCKED;
            }
        }
        else
        {
            bool	found = false;
	    // tout decrement for all open slots
            for(size_t i=0 ; i<m_nslots ; i++)
            {
                if( (r_state[i] == OPEN) && (r_tout[i] > 0)	)  r_tout[i] = r_tout[i] - 1;
            }
            // possible locking for one single open slot
            for(size_t i=0 ; i<m_nslots ; i++)
            {
                if( (r_state[i] == OPEN) && (r_tout[i] == 0) )
                {
                    // searching for a pending request with the same address
                    for( size_t j=0 ; j<m_nslots ; j++ )
                    {
                        if ( (r_state[j] != EMPTY) && 
                             (r_state[j] != OPEN)  &&
                             (r_address[i] == r_address[j]) ) 
                        {
                            found = true;
                            break; 
                        }
                    }
                    // locking the slot if no conflict
                    if ( !found ) r_state[i] = LOCKED;
                }
            }
        }
    } // end update()

    //////////////////////////////////////////////////////////////////////////
    bool write(addr_t addr, be_t be , data_t  data)
    // This method is intended to be used by the DCACHE FSM.
    // It can only change a slot state from EMPTY to OPEN.
    // It can change the slot content : r_address, r_data, r_be, r_min, r_max
    // It searches first an open slot, and then an empty slot.
    // It returns true in case of success.
    {
        size_t 	word = (size_t)((addr &  m_wbuf_line_mask) >> 2) ;
        addr_t 	address = addr & ~m_wbuf_line_mask ;
	bool	found = false;	
        size_t  lw;

        // search a slot to be written
        // scan open slots, then scan empty slots 
        for( size_t i=0 ; i<m_nslots ; i++)
        {
            if( (r_state[i] == OPEN) && (r_address[i] == address) )
            { 
                lw = i;
                found = true;
                break;
            }
        }
        if( !found )
        {
            for( size_t i=0 ; i<m_nslots ; i++)
            {
                if( r_state[i] == EMPTY )
                { 
                    lw = i;
                    found = true;
                    break;
                }
            }
        }
        // register the request when a slot has been found:
        // update r_state, r_address, r_be, r_data, r_min, r_max, r_tout
        if ( found )
        {
            r_state[lw]	= OPEN;
            r_address[lw] = address;
            r_be[lw][word]   = r_be[lw][word] | be;
            data_t  data_mask = 0;
            be_t    be_up = (1<<(sizeof(data_t)-1));
            for (size_t i = 0 ; i < sizeof(data_t) ; ++i) 
            {
                data_mask <<= 8;
                if ( be_up & be ) data_mask |= 0xff;
                be <<= 1;
            }
            r_data[lw][word] = (r_data[lw][word] & ~data_mask) | (data & data_mask) ;
            r_tout[lw] = m_timeout;
            if ( r_min[lw].read() > word ) r_min[lw] = word;
            if ( r_max[lw].read() < word ) r_max[lw] = word;
        }
        return found;
    } // end write()

    //////////////////////////////////////
    inline addr_t getAddress(size_t word)
    {
        return ( (addr_t)r_address[r_ptr.read()].read() + (addr_t)(word << 2) ) ;
    } 

    ///////////////////////////////////
    data_t inline getData(size_t word)
    {
        return r_data[r_ptr.read()][word] ;
    } 

    //////////////////////////////
    be_t inline getBe(size_t word)
    {
        return r_be[r_ptr.read()][word] ;
    } 

    /////////////////////////
    size_t inline getIndex()
    {
        return r_ptr.read();
    } 

    /////////////////////////////////////////////////////////////
    void inline completed(size_t index)
    // This method is intended to be used by the VCI_RSP FSM.
    // It can only change a slot state from SENT to EMPTY when
    // the corresponding write transaction is completed.
    {
        assert( (index < m_nslots) && (r_state[index].read() == SENT) &&
             "write buffer error : illegal completed command received");
        r_max[index]   	= 0 ;
        r_min[index]   	= m_nwords - 1 ;
        r_state[index] 	= EMPTY ;
        for( size_t w = 0 ; w < m_nwords ; w++ ) r_be[index][w]	= 0 ;
    } //end completed()

    /////////////////////////////////////////////////////////////////////// 
    MultiWriteBuffer(const std::string 	&name, 
                     size_t 		wbuf_nwords, 
                     size_t 		wbuf_nslots,
                     size_t		timeout,
                     size_t		cache_nwords)
        :
        m_nslots(wbuf_nslots),
        m_nwords(wbuf_nwords),
        m_timeout(timeout),
        m_wbuf_line_mask((wbuf_nwords << 2) - 1)
        m_cache_line_mask((cache_nwords << 2) - 1)
    {
        r_address = new sc_signal<addr_t>[wbuf_nslots];
        r_tout    = new sc_signal<size_t>[wbuf_nslots];
        r_min     = new sc_signal<size_t>[wbuf_nslots];
        r_max     = new sc_signal<size_t>[wbuf_nslots];
        r_state   = new sc_signal<int>[wbuf_nslots];

        r_data    = new data_t*[wbuf_nslots];
        r_be      = new be_t*[wbuf_nslots];
        for( size_t i = 0 ; i < wbuf_nslots ; i++ )
        {
            assert(IS_POW_OF_2(wbuf_nwords));
            r_data[i] = new data_t[wbuf_nwords];
            r_be[i]   = new be_t[wbuf_nwords];
        }
    }
    ///////////////////
    ~MultiWriteBuffer()
    {
        for( size_t i = 0 ; i < m_nslots ; i++ )
        {
            delete [] r_data[i];
            delete [] r_be[i];
        }
        delete [] r_data;
        delete [] r_be;
        delete [] r_address;
        delete [] r_tout;
        delete [] r_min;
        delete [] r_max;
        delete [] r_state;
    }
};

} // end name space soclib

#endif /* SOCLIB_MULTI_WRITE_BUFFER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

