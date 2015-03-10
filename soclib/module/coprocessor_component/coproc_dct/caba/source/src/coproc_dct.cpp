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
 * WITHOUT ANY WARr_opaNTY; without even the implied warranty of
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
 *         Alain Greiner <alain.greiner@lip6.fr>, 2015
 *         From a prvious work by Nicolas Pouillon (fifo_idct)
 *
 * Maintainers: alain
 */

#include "coproc_dct.h"
#include "alloc_elems.h"
#include <math.h>
#include "soclib_endian.h"

namespace soclib { namespace caba {


////////////////////////////
void CoprocDct::transition()
{
	if ( p_resetn.read() == false ) 
    {
		r_fsm = IDLE;
        return;
	}

    switch ( r_fsm.read() )
    {
        //////////
        case IDLE:   // test configuration register
        {
            if ( p_config.read() != 0 )
            {
                r_ptr = 0;
                r_fsm = REQ_READ;
            }
            break;
        }
        //////////////
        case REQ_READ:   // request read input vector
        {
            if ( (p_in.ack) == true ) 
            {
                r_fsm = LOAD;
            }
            break;
        }
        //////////
        case LOAD:    // load r_bufin from DMA 
        {
            if ( p_in.rok )
            {
                // get one word from input fifo
                r_bufin[r_ptr.read()] = p_in.data.read();
                r_ptr = r_ptr.read() + 1;

                // check load completion
                if ( r_ptr.read() == (VECTOR_SIZE - 1) ) 
                {
                    r_ptr        = 0;
                    r_exec_count = m_exec_latency;
                    r_fsm        = EXEC;
                }
            }
            break;   
        }
        //////////
        case EXEC:    // compute bufout from bufin   
        {
            if r_exec_count.read() > 0 )
            {
                r_exec_count = r_exec_count.read() - 1;
            }
            else
            {
                do_dct_8x8();
                r_fsm = REQ_WRITE;
            }
            break;
        }
        ///////////////
        case REQ_WRITE:   // request write output vector 
        {
            if ( p_out.ack )
            {
                r_fsm = STORE;
            }
            break;
        }
        ///////////
        case STORE:     // store r_bufout to DMA
        {
            // put one word into res fifo
            if ( p_out.wok )
            {
                r_ptr = r_ptr.read() + 1;

                // check store completion
                if ( r_ptr.read() == (VECTOR_SIZE) )
                {
                    r_ptr = 0;
                    r_fsm = IDLE;
                }
            }
            break;
        }
	}  // end switch( r_fsm )
}  // end transition

/////////////////////
CoprocDct::genMoore()
{
    // p_in port
    p_in.req    = (r_fsm.read() == REQ_READ);
    p_in.bursts = m_bursts;
    p_in.r      = (r_fsm.read() == LOAD);

    // p_out port
    p_out.req    = (r_fsm.read() == REQ_WRITE);
    p_out.bursts = m_bursts;
    p_out.w      = (r_fsm.read() == STORE);
    p_out.data   = le_to_machine( r_bufout[r_ptr.read()] );
}

////////////////////////////
void CoprocDct::do_dct_8x8()
{
/* hard value of sqrt(1/8) */
#define SQRT_0125 0.353553391

	double tab_a[8][8], tab_b[8][8];

	for ( int col=0; col<8; ++col )
    {
		for ( int line=0; line<8; ++line )
		{
            tab_a[line][col] = r_bufin[line*8+col] / (double)(1<<14);
        }
    }

	for ( int line=0; line<8; ++line ) 
    {
		for ( int col=0; col<8; ++col ) 
        {
			double s = SQRT_0125 * tab_a[line][0];
			for ( int c=1; c<8; ++c )
				s += .5 * tab_a[line][c] * cos(M_PI/8.*(.5+col)*c);
			tab_b[line][col] = s;
		}
	}

	for ( int col=0; col<8; ++col ) 
    {
		for ( int line=0; line<8; ++line ) 
        {
			double s = SQRT_0125 * tab_b[0][col];
			for ( int l=1; l<8; ++l )
				s += .5 * tab_b[l][col] * cos(M_PI/8.*(.5+line)*l);
			tab_a[line][col] = s;
		}
	}

	for ( int col=0; col<8; ++col )
    {
		for ( int line=0; line<8; ++line ) 
        {
			int32_t tmp = tab_a[line][col]*(double)(1<<14)+128;
			if ( tmp < 0 ) tmp = 0;
			if ( tmp > 255 ) tmp = 255;
			r_bufoutput[line*8+col] = tmp;
		}
    }
}

///////////////////////////////////////////////////
CoprocDct::CoprocDct( sc_core::sc_module_name name,
                      const uint32_t          burst_size,
	                  const uint32_t          exec_latency )
    : soclib::caba::BaseModule(name),

      p_clk( "clk" ),
      p_resetn( "resetn" ),
      p_in( "p_in" ),
      p_out( "p_out" ),
      p_config( "p_config" ),

      m_exec_latency( exec_latency ),
      m_burst_size( burst_size ),
      m_nb_bursts( VECTOR_SIZE / burst_size )
{     
    std::cout << "  - Building CoprocDct : " << name << std::endl;

    assert ( ((burst_size == 4) or (burst_size == 8) or (burst_size == 16)
              or (burst_size == 32) or (burst_size == 64)) and
           "ERROR in COPROC_DCT : illegal burst_size parameter" );

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

}}
