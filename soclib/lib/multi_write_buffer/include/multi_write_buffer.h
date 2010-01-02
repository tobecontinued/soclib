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
// It contains an integer number of buffer lines, and each buffer line 
// contains one or several 32 bits words. This nwords parameter
// must be a power of 2, and  defines the max length of a write burst.
// The nlines parameter defines the max number of concurrent transactions.
// All word adresses in a buffer line are contiguous, and the
// buffer line base address is aligned on a buffer line boundary.
// One buffer line is typically a cache line, or half a cache line.
// Byte enable controlled write after write are supported.
// All write requests in the same buffer line will be transmitted
// in a single VCI transaction.
// A line is "closed" when it is ready to be send: The condition is to
// have more than zero cycles between to successive write request 
// from the processor in the same buffer line.
// - The wok() method returns true when a write request can be accepted.
// - The write() method is used to store a write request in the buffer,
//   and increment the write pointer if necessary.
// - The rok() method returns true when a buffer line can be sent.
// - The getXXX() methods are used to build a write transaction,
//   when a buffer line is ready to be send.
// - The sent() method is used to signal that the last flit of a write burst 
//   transaction has been send, and increment the read pointer.
// - The completed() method is used to signal that a write transaction
//   has been completed, and to reset the corresponding line buffer.
// - The empty() method returns true when all lines are empty.
// - The miss() method returns false if a given adress matches a pending 
//   write request (the word index in the buffer line is ignored).
// User note :
// The write() method must be called at all cycles by the transition 
// function of the hardware component that contains this write buffer
// even if there is no valid transaction, because the internal state
// must be updated at each cycle.
///////////////////////////////////////////////////////////////////////////
// It has three constructor parameters :
// - std::string    name
// - size_t         nwords : buffer width (number of words)
// - size_t	    nlines : buffer depth (number of lines)
//
// It has one template parameter :
// - addr_t defines the address format
////////////////////////////////////////////////////////////////////////////
// Implementation notes :
// The write buffer has a FIFO behavior, with two read & write pointer.
// A buffer line can be in four states:
// - EMPTY  : mofified by the write() method to go to OPEN state.
// - OPEN   : modified by the write() method to go to LOCKED state.
// - LOCKED : modified by the sent() method to go to SENT state.
// - SENT   : modified by the completed() method to go to EMPTY state.
// - The r_ptw register contains the index of the next line to be written.
//   It can only be modified by the producer using the write() method.
// - The r_ptr register contains the index of the next line to be read.
//   It can only be modified by the consumer using the the sent() method.
// A write request is accepted if the line pointed by the write pointer
// is EMPTY, or the line is OPEN and the addresses match, 
// or if the next line is EMPTY.
// There is a read request when the line pointed by the read pointer is
// in LOCKED state.
// It can exist several empty lines, several locked lines,
// several sent lines, but only one open line.
////////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_MULTI_WRITE_BUFFER_H
#define SOCLIB_MULTI_WRITE_BUFFER_H

#include <systemc>
#include <cassert>

namespace soclib { 

using namespace sc_core;

enum line_state_e {
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

    sc_signal<size_t>	r_ptr;		// line to be read
    sc_signal<size_t>	r_ptw;		// line to be written
    sc_signal<int>      *r_state;       // line state array[nlines]
    sc_signal<addr_t>  	*r_address;     // line base address[nlines]
    sc_signal<size_t>  	*r_min;         // smallest valid word index array[nlines]
    sc_signal<size_t>  	*r_max;         // largest valid word index array[nlines]
    data_t   		**r_data;       // write data  array[nlines][nwords]
    be_t     		**r_be;         // byte enable array[nlines][nwords]

    size_t              m_nlines;       // buffer depth (number of lines)
    size_t              m_nwords;       // buffer width (number of words)
    addr_t              m_mask;         // cache line mask
 
public:

    /////////////
    void reset()
    /////////////
    {
        r_ptr = 0 ;
        r_ptw = 0 ;
	for( size_t line = 0 ; line < m_nlines ; line++) {
            r_address[line] 	= 0 ;
            r_max[line]   	= 0 ;
            r_min[line]   	= m_nwords - 1 ;
            r_state[line] 	= EMPTY ;
            for( size_t word = 0 ; word < m_nwords ; word++ ) {
                r_be[line][word] 	= 0 ;
                r_data[line][word] 	= 0 ;
            }
        }
    } 

    ///////////////////////////////////////////////////
    inline void print()
    ///////////////////////////////////////////////////
    {
        const char *wbuf_state_str[] = { "EMPTY ", "OPEN  ", "LOCKED", "SENT  " };

        std::cout << "*** Write Buffer State ***" << std::endl;
        for( size_t i = 0 ; i < m_nlines ; i++ )
        {
            std::cout << "LINE " << i << " : " 
                      << wbuf_state_str[r_state[i]] 
                      << std::hex << " address = " << r_address[i] 
                      << " min = " << r_min[i]
                      << " max = " << r_max[i] << std::endl;
            for( size_t w = 0 ; w < m_nwords ; w++ )
            {
                std::cout << " / D" << std::dec << w << " = " 
                          << std::hex << r_data[i][w] << "  be = " << r_be[i][w] ;
            }
            std::cout << std::endl;
        }
        std::cout << "  ptw = " << r_ptw << "  ptr = " << r_ptr << std::endl;
    }

    ///////////////////////////////////////////////////
    inline bool miss( addr_t addr )
    ///////////////////////////////////////////////////
    {
        for( size_t i = 0 ; i < m_nlines ; i++ )
        {
            if ((r_state[i] != EMPTY) && (r_address[i] == (addr & ~m_mask))) return false;
        }
        return true;
    }
    
    ///////////////////////////////////////////////////
    inline bool wok( addr_t addr )
    ///////////////////////////////////////////////////
    {
	if( r_state[r_ptw] == EMPTY) 					return true;
        else if( (r_state[r_ptw] == OPEN) &&
                  ((addr_t)r_address[r_ptw] == (addr & ~m_mask)) ) 	return true;
        else if( (r_state[r_ptw] == OPEN) &&
                   (r_state[(r_ptw + 1)%m_nlines] == EMPTY) ) 		return true;
        else								return false;
    }

    ///////////////////////////////////////////////////
    inline bool empty( )
    ///////////////////////////////////////////////////
    {
        for( size_t i = 0 ; i < m_nlines ; i++ )
        {
            if ( r_state[i] != EMPTY ) return false;
        }
        return true;
    }

    /////////////////////////////////////////////////////////////////
    void write(bool valid, addr_t addr, be_t be , data_t  data)
    // This method must be called at each cycle.
    // It can modify the write pointer.
    // It can change the pointed line state from EMPTY to OPEN, 
    // or fom OPEN to LOCKED.
    // It can change the state of the next line from EMPTY to OPEN
    // If there is a valid write request, it registers the request
    // in the pointed line if it is EMPTY, or if it is OPEN and the
    // address matches, or in the the next line if it is EMPTY. 
    /////////////////////////////////////////////////////////////////
    {
        size_t 	word = (size_t)((addr &  m_mask) >> 2) ;
        addr_t 	address = addr & ~m_mask ;
	bool	found ;				// line to be written found
        size_t  lw ;				// index of the written line

	if( !valid )  		// no write request
        { 
            // update the write pointer & the line state if OPEN
            if( r_state[r_ptw] == OPEN )  
            {
                r_state[r_ptw] = LOCKED; 
                r_ptw = (r_ptw + 1) % m_nlines;  
            }

	} 
        else 			// valid write request
        {

#ifdef WRITE_BUFFER_DEBUG
std::cout << std::endl;
std::cout << "********** write buffer : write access" << std::endl;
std::cout << "address = " << addr << std::endl;
std::cout << "data    = " << data << std::endl;
std::cout << "be      = " << be   << std::endl;
std::cout << std::endl;
#endif

            // find the line to be written and update r_state & r_ptw
            if( r_state[r_ptw] == EMPTY) 
            {
                found = true ;
                lw = r_ptw ;
                r_state[r_ptw] = OPEN ;
            }
            else if( r_state[r_ptw] == OPEN) 
            {
                if(r_address[r_ptw] == address)  
                {
                    found = true ;
                    lw = r_ptw ;
	        } 
                else   // no convenient open line : take next line if empty
                {
                    r_state[r_ptw] = LOCKED ;
                    lw = (r_ptw + 1) % m_nlines ;
                    r_ptw = lw ;		// increment r_ptw
		    if( r_state[lw] == EMPTY ) 
                    {
                        found = true ;
                        r_state[lw] = OPEN ;
                    }
                    else
                    {
                        found = false;
                    }
                }
            }
            else 
            {
                found = false ;
            }

            if (!found) return ;

            // register the request:  update r_address, r_be, r_data (building a mask from be)
            r_address[lw] = address ;
            r_be[lw][word]   = r_be[lw][word] | be ;
            data_t  data_mask = 0;
            be_t    be_up = (1<<(sizeof(data_t)-1));
            for (size_t i = 0 ; i < sizeof(data_t) ; ++i) 
            {
                data_mask <<= 8;
                if ( be_up & be ) data_mask |= 0xff;
                be <<= 1;
            }
            r_data[lw][word] = (r_data[lw][word] & ~data_mask) | (data & data_mask) ;
            // update r_min & r_max
            if ( r_min[lw].read() > word ) r_min[lw] = word;
            if ( r_max[lw].read() < word ) r_max[lw] = word;
        }
    } // end write()

    ///////////////////////////////////////////////////
    inline bool rok()
    ///////////////////////////////////////////////////
    {
        return ( r_state[r_ptr].read() == LOCKED ) ;
    }

    ///////////////////////////////////
    inline size_t getIndex()
    ///////////////////////////////////
    {
        return  r_ptr ;
    }

    ///////////////////////////////////
    inline size_t getMin()
    ///////////////////////////////////
    {
        return  r_min[r_ptr] ;
    }

    ///////////////////////////////////
    inline size_t getMax()
    ///////////////////////////////////
    {
        return  r_max[r_ptr] ;
    }

    //////////////////////////////////////
    inline addr_t getAddress(size_t word)
    //////////////////////////////////////
    {
        return ( (addr_t)r_address[r_ptr] + (addr_t)(word << 2) ) ;
    } 

    ///////////////////////////////////
    data_t inline getData(size_t word)
    ///////////////////////////////////
    {
        return r_data[r_ptr][word] ;
    } 

    ///////////////////////////////////
    be_t inline getBe(size_t word)
    ///////////////////////////////////
    {
        return r_be[r_ptr][word] ;
    } 

    /////////////////////////////////////////////////////////////
    void inline completed(size_t index)
    // This method reset the corresponding line to empty state.
    // It is intended to be called by the response FSM when
    // the corresponding write transaction is completed.
    /////////////////////////////////////////////////////////////
    {

#ifdef WRITE_BUFFER_DEBUG
std::cout << std::endl;
std::cout << "********** write buffer : line " << index << " completed" << std::endl;
std::cout << std::endl;
#endif

        assert( (index < m_nlines) && (r_state[index] == SENT) &&
             "write buffer error : illegal completed command received");
        r_max[index]   	= 0 ;
        r_min[index]   	= m_nwords - 1 ;
        r_state[index] 	= EMPTY ;
        for( size_t w = 0 ; w < m_nwords ; w++ ) r_be[index][w]	= 0 ;
    } //end completed()

    ///////////////////////////////////////////////////////////////
    void inline sent()  
    // This method set the line  pointed by the read pointer
    // to SENT state and increment the read pointer.
    // It is intended to be called by the commande FSM when
    // the corresponding write command has been fully transmitted.
    //////////////////////////////////////////////////////////////
    {

#ifdef WRITE_BUFFER_DEBUG
std::cout << std::endl;
std::cout << "********** write buffer : line " << r_ptr << " sent" << std::endl;
std::cout << std::endl;
#endif
        assert( (r_state[r_ptr] == LOCKED) &&
             "write buffer error : illegal sent command received");
        r_state[r_ptr] = SENT;
	r_ptr = (r_ptr + 1) % m_nlines;  // increment index
    } 

    /////////////////////////////////////////////////////////////////////// 
    MultiWriteBuffer(const std::string &name, size_t nwords, size_t nlines)
    /////////////////////////////////////////////////////////////////////// 
        :
        m_nlines(nlines),
        m_nwords(nwords),
        m_mask((nwords << 2) - 1)
    {
        r_address = new sc_signal<addr_t>[nlines];
        r_min     = new sc_signal<size_t>[nlines];
        r_max     = new sc_signal<size_t>[nlines];
        r_state   = new sc_signal<int>[nlines];

        r_data    = new data_t*[nlines];
        r_be      = new be_t*[nlines];
        for( size_t i = 0 ; i < nlines ; i++ )
        {
            assert(IS_POW_OF_2(nwords));
            r_data[i] = new data_t[nwords];
            r_be[i]   = new be_t[nwords];
        }
    }
    ///////////////////
    ~MultiWriteBuffer()
    ///////////////////
    {
        for( size_t i = 0 ; i < m_nlines ; i++ )
        {
            delete [] r_data[i];
            delete [] r_be[i];
        }
        delete [] r_data;
        delete [] r_be;
        delete [] r_address;
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

