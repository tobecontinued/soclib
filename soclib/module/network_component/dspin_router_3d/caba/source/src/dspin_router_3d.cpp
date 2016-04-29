/* -*- c++ -*-
  *
  * File : dspin_router_3d.cpp
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner, Abbas Sheibanyrad, Ivan Miro, Zhen Zhang
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
  */

    ///////////////////////////////////////////////////////////////////////////
    // Implementation Note :
    // The xfirst_route(), broadcast_route() and is_broadcast() functions
    // defined below are used to decode the DSPIN first flit format:
    // - In case of a non-broadcast packet :
    //  |   X     |   Y     |   Z     |------------------------------------------------|BC |
    //  | x_width | y_width | z_width | flit_width - (x_width + y_width + z_width + 2) | 0 |
    //
    //  - In case of a broacast
    //  |  XMIN   |  XMAX   |  YMIN   |  YMAX   |-------------------|BC |
    //  |   5     |   5     |   5     |   5     | flit_width - 22   | 1 |
    //  note that XMIN, XMAX, YMIN, YMAX are ignored
    ///////////////////////////////////////////////////////////////////////////

#include "../include/dspin_router_3d.h"
#include <assert.h>

namespace soclib { namespace caba {

using namespace soclib::common;
using namespace soclib::caba;

#define tmpl(x) template<int flit_width> x DspinRouter3d<flit_width>

    ////////////////////////////////////////////////
    //	      constructor
    ////////////////////////////////////////////////
    tmpl(/**/)::DspinRouter3d( sc_module_name name, 
			     const size_t   x,
			     const size_t   y,
			     const size_t   z,
			     const size_t   x_width,
			     const size_t   y_width,
			     const size_t   z_width,
			     const size_t   elevator_x,
			     const size_t   elevator_y,
			     const size_t   in_fifo_depth,
			     const size_t   out_fifo_depth,
			     const bool     broadcast_supported )
	: soclib::caba::BaseModule(name),

      m_local_x( x ),
      m_local_y( y ),
      m_local_z( z ),
      m_x_width( x_width ),
      m_x_shift( flit_width - x_width ),
      m_x_mask( (0x1 << x_width) - 1 ),
      m_y_width( y_width ),
      m_y_shift( flit_width - x_width - y_width ),
      m_y_mask( (0x1 << y_width) - 1 ),
      m_z_width( z_width ),
      m_z_shift( flit_width - x_width - y_width - z_width ),
      m_z_mask( (0x1 << z_width) - 1 ),
      m_elevator_x (elevator_x),
      m_elevator_y (elevator_y),
      m_broadcast_supported( broadcast_supported ),
      m_is_elevator ((x == elevator_x && y == elevator_y)),
      // m_nports (( m_is_elevator ? N_PORTS : N_PORTS_2D)),
      m_nports (N_PORTS ) ,

      p_clk( "p_clk" ),
      p_resetn( "p_resetn" ),
      // the Z ports always exists, even if they're unused
      p_in( alloc_elems<DspinInput<flit_width> >("p_in", N_PORTS) ),
      p_out( alloc_elems<DspinOutput<flit_width> >("p_out", N_PORTS) ),

      r_alloc_out( alloc_elems<sc_signal<bool> >("r_alloc_out", N_PORTS)),
      r_index_out( soclib::common::alloc_elems<sc_signal<size_t> >("r_index_out", N_PORTS)),
      r_fsm_in( alloc_elems<sc_signal<int> >("r_fsm_in", N_PORTS)),
      r_index_in( alloc_elems<sc_signal<size_t> >("r_index_in", N_PORTS))

    {
	std::cout << "  - Building DspinRouter3d : " << name << std::endl;

	SC_METHOD (transition);
	dont_initialize();
	sensitive << p_clk.pos();

   	SC_METHOD (genMoore);
	dont_initialize();
	sensitive  << p_clk.neg();

	r_fifo_in  = (GenericFifo<internal_flit_t>*)
			malloc(sizeof(GenericFifo<internal_flit_t>) * m_nports);

	r_fifo_out = (GenericFifo<internal_flit_t>*)
			malloc(sizeof(GenericFifo<internal_flit_t>) * m_nports);

	r_buf_in   = (internal_flit_t*)
			malloc(sizeof(internal_flit_t) * m_nports);

	for( size_t i = 0 ; i < m_nports ; i++ )
	{
		std::ostringstream stri;
		stri << "r_in_fifo_" << i;
		new(&r_fifo_in[i])  
		GenericFifo<internal_flit_t >(stri.str(), in_fifo_depth);

		std::ostringstream stro;
		stro << "r_out_fifo_" << i;
		new(&r_fifo_out[i]) 
		GenericFifo<internal_flit_t >(stro.str(), out_fifo_depth);
	}
    } //  end constructor

    ///////////////////////////////////////////////////
    tmpl(int)::xfirst_route( sc_uint<flit_width> data )
    {
	size_t zdest = (size_t)(data >> m_z_shift) & m_z_mask;
	size_t xdest;
	size_t ydest;
	if (zdest != m_local_z) {
		// route to elevator, or up if we are the elevator
		if (m_is_elevator) {
			assert(m_nports > 5);
			return (zdest > m_local_z) ? REQ_UP : REQ_DOWN;
		} else {
			xdest = m_elevator_x;
			ydest = m_elevator_y;
		}
	} else {
		xdest = (size_t)(data >> m_x_shift) & m_x_mask;
		ydest = (size_t)(data >> m_y_shift) & m_y_mask;
	}
	return (xdest < m_local_x ? REQ_WEST : 
	       (xdest > m_local_x ? REQ_EAST : 
	       (ydest < m_local_y ? REQ_SOUTH : 
	       (ydest > m_local_y ? REQ_NORTH : REQ_LOCAL))));
    }

    //////////////////////////////////////////////////////////////////////////
    tmpl(int)::broadcast_route(int step, int source, sc_uint<flit_width> data)
    {
	int    sel  = REQ_NOP;
	const size_t xmin = 0;
	const size_t xmax = 0x1F;
	const size_t ymin = 0;
	const size_t ymax = 0x1F;

	switch(source) {
	case REQ_LOCAL :
	    if (m_is_elevator) {
		    assert(m_nports > 5);
		    if      ( step == 1 )	sel = REQ_UP;
		    else if ( step == 2 )	sel = REQ_DOWN;
		    else if ( step == 3 )	sel = REQ_NORTH;
		    else if ( step == 4 )	sel = REQ_SOUTH;
		    else if ( step == 5 )	sel = REQ_EAST;
		    else if ( step == 6 )	sel = REQ_WEST;
	    } else {
		    assert(step <= 4 && "wrong REQ_LOCAL step for non-elevator");
		    if      ( step == 1 )	sel = REQ_NORTH;
		    else if ( step == 2 )	sel = REQ_SOUTH;
		    else if ( step == 3 )	sel = REQ_EAST;
		    else if ( step == 4 )	sel = REQ_WEST;
	     }
	break;
	case REQ_NORTH :
	    if (m_is_elevator) {
		    assert(m_nports > 5);
		    if      ( step == 1 )	sel = REQ_UP;
		    else if ( step == 2 )	sel = REQ_DOWN;
		    else if ( step == 3 )	sel = REQ_SOUTH;
		    else if ( step == 4 )	sel = REQ_LOCAL;
		    else 			sel = REQ_NOP;
	    } else {
		    assert(step <= 4 && "wrong REQ_NORTH step for non-elevator");
		    if      ( step == 1 )	sel = REQ_SOUTH;
		    else if ( step == 2 )	sel = REQ_LOCAL;
		    else 			sel = REQ_NOP;
	    }
	break;
	case REQ_SOUTH :
	    if (m_is_elevator) {
		    assert(m_nports > 5);
		    if      ( step == 1 )	sel = REQ_UP;
		    else if ( step == 2 )	sel = REQ_DOWN;
		    else if ( step == 3 )	sel = REQ_NORTH;
		    else if ( step == 4 )	sel = REQ_LOCAL;
		    else 			sel = REQ_NOP;
	    } else {
		    assert(step <= 4 && "wrong REQ_SOUTH step for non-elevator");
		    if      ( step == 1 )	sel = REQ_NORTH;
		    else if ( step == 2 )	sel = REQ_LOCAL;
		    else 			sel = REQ_NOP;
	    }
	break;
	case REQ_EAST :
	    if (m_is_elevator) {
		    assert(m_nports > 5);
		    if      ( step == 1 )	sel = REQ_UP;
		    else if ( step == 2 )	sel = REQ_DOWN;
		    else if ( step == 3 )	sel = REQ_WEST;
		    else if ( step == 4 )	sel = REQ_NORTH;
		    else if ( step == 5 )	sel = REQ_SOUTH;
		    else if ( step == 6 )	sel = REQ_LOCAL;
	    } else {
		    assert(step <= 4 && "wrong REQ_EAST step for non-elevator");
		    if      ( step == 1 )	sel = REQ_WEST;
		    else if ( step == 2 )	sel = REQ_NORTH;
		    else if ( step == 3 )	sel = REQ_SOUTH;
		    else if ( step == 4 )	sel = REQ_LOCAL;
	    }
	break;
	case REQ_WEST :
	    if (m_is_elevator) {
		    assert(m_nports > 5);
		    if      ( step == 1 )	sel = REQ_UP;
		    else if ( step == 2 )	sel = REQ_DOWN;
		    else if ( step == 3 )	sel = REQ_EAST;
		    else if ( step == 4 )	sel = REQ_NORTH;
		    else if ( step == 5 )	sel = REQ_SOUTH;
		    else if ( step == 6 )	sel = REQ_LOCAL;
	    } else {
		    assert(step <= 4 && "wrong REQ_WEST step for non-elevator");
		    if      ( step == 1 )	sel = REQ_EAST;
		    else if ( step == 2 )	sel = REQ_NORTH;
		    else if ( step == 3 )	sel = REQ_SOUTH;
		    else if ( step == 4 )	sel = REQ_LOCAL;
	    }
	break;
	case REQ_UP :
	    assert(m_is_elevator && "request from UP on non-elevator");
	    if      ( step == 1 )	sel = REQ_DOWN;
	    else if ( step == 2 )	sel = REQ_NORTH;
	    else if ( step == 3 )	sel = REQ_SOUTH;
	    else if ( step == 4 )	sel = REQ_EAST;
	    else if ( step == 5 )	sel = REQ_WEST;
	    else if ( step == 6 )	sel = REQ_LOCAL;
	break;
	case REQ_DOWN :
	    assert(m_is_elevator && "request from DOWN on non-elevator");
	    if      ( step == 1 )	sel = REQ_UP;
	    else if ( step == 2 )	sel = REQ_NORTH;
	    else if ( step == 3 )	sel = REQ_SOUTH;
	    else if ( step == 4 )	sel = REQ_EAST;
	    else if ( step == 5 )	sel = REQ_WEST;
	    else if ( step == 6 )	sel = REQ_LOCAL;
	break;
	}
	if      ( (sel == REQ_NORTH) && !(m_local_y < ymax) ) 	sel = REQ_NOP;
	else if ( (sel == REQ_SOUTH) && !(m_local_y > ymin) ) 	sel = REQ_NOP;
	else if ( (sel == REQ_EAST ) && !(m_local_x < xmax) ) 	sel = REQ_NOP;
	else if ( (sel == REQ_WEST ) && !(m_local_x > xmin) ) 	sel = REQ_NOP;

	return sel;
    }

    /////////////////////////////////////////////////////////
    tmpl(inline bool)::is_broadcast(sc_uint<flit_width> data)
    {
	return ( (data & 0x1) != 0);
    }

    /////////////////////////
    tmpl(void)::print_trace()
    {
	const char* port_name[] = 
	{
	    "L",
	    "N",
	    "S",
	    "E",
	    "W",
	    "U",
	    "D"
	};

	const char* infsm_str[] =
	{
	    "IDLE",
	    "REQ",
	    "ALLOC",
	    "REQ_FIRST",
	    "ALLOC_FIRST",
	    "REQ_SECOND",
	    "ALLOC_SECOND",
	    "REQ_THIRD",
	    "ALLOC_THIRD",
	    "REQ_FOURTH",
	    "ALLOC_FOURTH"
	    "REQ_FIFTH",
	    "ALLOC_FIFTH"
	    "REQ_SIXTH",
	    "ALLOC_SIXTH"
	};

	std::cout << "DSPIN_ROUTER " << name();

	for( size_t i = 0 ; i < m_nports ; i++)  // loop on input ports
	{
	    std::cout << " / infsm[" << port_name[i] << "] "
		      << infsm_str[r_fsm_in[i].read()];
	}

	for ( size_t out=0 ; out<m_nports ; out++)  // loop on output ports
	{
	    if ( r_alloc_out[out].read() )
	    {
		int in = r_index_out[out];
		std::cout << " / " << port_name[in] << " -> " << port_name[out] ;
	    }   
	}
	std::cout << std::endl;
    }

    ////////////////////////
    tmpl(void)::transition()
    {
	// Long wires connecting input and output ports
	size_t	       req_in[m_nports];   // input ports  -> output ports
	size_t	      get_out[m_nports];   // output ports -> input ports
	bool	       put_in[m_nports];   // input ports  -> output ports
	internal_flit_t data_in[m_nports]; // input ports  -> output ports

	// control signals for the input fifos
	bool		fifo_in_write[m_nports];
	bool		fifo_in_read[m_nports];	
	internal_flit_t     fifo_in_wdata[m_nports];

	// control signals for the output fifos
	bool		fifo_out_write[m_nports];
	bool		fifo_out_read[m_nports];
	internal_flit_t     fifo_out_wdata[m_nports];

	// Reset 
	if ( p_resetn == false ) 
	{
		for(size_t i = 0 ; i < m_nports ; i++) 
		{
			r_alloc_out[i] = false;
			r_index_out[i] = 0;
			r_index_in[i]  = 0;
			r_fsm_in[i]    = INFSM_IDLE;
			r_fifo_in[i].init();
			r_fifo_out[i].init();
		}
	    return;
	}

	// fifos signals default values
	for(size_t i = 0 ; i < m_nports ; i++) 
	{
		fifo_in_read[i]	= false;
		fifo_in_write[i]       = p_in[i].write.read();
		fifo_in_wdata[i].data  = p_in[i].data.read();
		fifo_in_wdata[i].eop   = p_in[i].eop.read();
	 
		fifo_out_read[i]       = p_out[i].read.read();
		fifo_out_write[i]      = false;
	}

	// loop on the output ports:
	// compute get_out[j] depending on the output port state
	// and combining fifo_out[j].wok and r_alloc_out[j]
	for ( size_t j = 0 ; j < m_nports ; j++ )
	{
		if( r_alloc_out[j].read() and (r_fifo_out[j].wok()) ) 
		{
			get_out[j] = r_index_out[j].read();
		}
		else
		{		       
			get_out[j] = 0xFFFFFFFF;  
		}
	}

	// loop on the input ports :
	// The port state is defined by r_fsm_in[i], r_index_in[i] & r_buf_in[i]
	// The req_in[i] computation implements the X-FIRST algorithm.
	// data_in[i], put_in[i] and req_in[i] depend on the input port state.
	// The fifo_in_read[i] is computed further...

	for ( size_t i = 0 ; i < m_nports ; i++ )
	{
	    switch ( r_fsm_in[i].read() )
	    {
		case INFSM_IDLE:    // no output port allocated
		{
		    put_in[i] = false;
		    if ( r_fifo_in[i].rok() ) // packet available in input fifo
		    {
			if ( is_broadcast( r_fifo_in[i].read().data ) and
			     m_broadcast_supported )	  // broadcast
			{ 
			    fifo_in_read[i] = true;
			    req_in[i]       = broadcast_route(1, i, r_fifo_in[i].read().data);
			    r_buf_in[i]     = r_fifo_in[i].read();
			    r_index_in[i]   = req_in[i];
			    if( req_in[i] == REQ_NOP ) r_fsm_in[i] = INFSM_REQ_SECOND;
			    else		       r_fsm_in[i] = INFSM_REQ_FIRST;
			}
			else				  // unicast
			{
			    req_in[i]       = xfirst_route(r_fifo_in[i].read().data);
			    r_index_in[i]   = req_in[i];
			    r_fsm_in[i]     = INFSM_REQ;
			}
		    }
		    else
		    {
			req_in[i] = REQ_NOP;
		    }
		    break;
		}
		case INFSM_REQ:   // not a broadcast / waiting output port allocation
		{
		    data_in[i]      = r_fifo_in[i].read();
		    put_in[i]       = r_fifo_in[i].rok();
		    req_in[i]       = r_index_in[i];
		    fifo_in_read[i] = (get_out[r_index_in[i].read()] == i);
		    if ( get_out[r_index_in[i].read()] == i ) // first flit transfered
		    {
			if ( r_fifo_in[i].read().eop ) r_fsm_in[i] = INFSM_IDLE;
			else			      r_fsm_in[i] = INFSM_ALLOC;
		    }
		    break;
		}
		case INFSM_ALLOC:   // not a broadcast / output port allocated
		{
		    data_in[i]      = r_fifo_in[i].read();
		    put_in[i]       = r_fifo_in[i].rok();
		    req_in[i]       = REQ_NOP;		 // no request
		    fifo_in_read[i] = (get_out[r_index_in[i].read()] == i);
		    if ( r_fifo_in[i].read().eop and 
			 r_fifo_in[i].rok() and 
			 (get_out[r_index_in[i].read()] == i) ) // last flit transfered 
		    {
			r_fsm_in[i] = INFSM_IDLE;
		    }
		    break;
		}
		case INFSM_REQ_FIRST: // broacast / waiting first output port allocation
		{
		    data_in[i]    = r_buf_in[i];
		    put_in[i]     = true;
		    req_in[i]     = broadcast_route(1, i, r_buf_in[i].data);
		    r_index_in[i] = req_in[i];
		    if ( req_in[i] == REQ_NOP )   // no transfer for this step
		    {
			r_fsm_in[i] = INFSM_REQ_SECOND;
		    }
		    else
		    {
			if( get_out[req_in[i]] == i )  // header flit transfered
			{
			    r_fsm_in[i] = INFSM_ALLOC_FIRST;
			}
		    }
		    break;
		}
		case INFSM_ALLOC_FIRST:  // broadcast / first output port allocated
		{
		    data_in[i] = r_fifo_in[i].read();
		    put_in[i]  = r_fifo_in[i].rok();
		    req_in[i]  = REQ_NOP;
		    if( (get_out[r_index_in[i].read()] == i) 
			 and r_fifo_in[i].rok() )		 // data flit transfered 
		    {
			if ( not r_fifo_in[i].read().eop )
			{
			    std::cout << "ERROR in DSPIN_ROUTER/1 " << name()
				      << " : broadcast packet must be 2 flits" << std::endl;
			}
			r_fsm_in[i] = INFSM_REQ_SECOND;
		    }
		    break;
		}
		case INFSM_REQ_SECOND: // broacast / waiting second output port allocation
		{
		    data_in[i]    = r_buf_in[i];
		    put_in[i]     = true;
		    req_in[i]     = broadcast_route(2, i, r_buf_in[i].data);
		    r_index_in[i] = req_in[i];
		    if ( req_in[i] == REQ_NOP )  // no transfer for this step
		    {
			r_fsm_in[i] = INFSM_REQ_THIRD;
		    }
		    else
		    {
			if( get_out[req_in[i]] == i ) // header flit transfered
			{
			    r_fsm_in[i] = INFSM_ALLOC_SECOND;
			}
		    }
		    break;
		}
		case INFSM_ALLOC_SECOND:  // broadcast / second output port allocated
		{
		    data_in[i] = r_fifo_in[i].read();
		    put_in[i]  = r_fifo_in[i].rok();
		    req_in[i]  = REQ_NOP;
		    if( (get_out[r_index_in[i].read()] == i ) 
			 and r_fifo_in[i].rok() )	       // data flit transfered
		    {
			if ( not r_fifo_in[i].read().eop )
			{
			    std::cout << "ERROR in DSPIN_ROUTER/2 " << name()
				      << " : broadcast packet must be 2 flits" << std::endl;
			}
			r_fsm_in[i] = INFSM_REQ_THIRD;
		    }
		    break;
		}
		case INFSM_REQ_THIRD: // broacast / waiting third output port allocation
		{
		    data_in[i]    = r_buf_in[i];
		    put_in[i]     = true;
		    req_in[i]     = broadcast_route(3, i, r_buf_in[i].data);
		    r_index_in[i] = req_in[i];
		    if ( req_in[i] == REQ_NOP )  // no transfer for this step
		    {
			r_fsm_in[i] = INFSM_REQ_FOURTH;
		    }
		    else
		    {
			if( get_out[req_in[i]] == i ) // header flit transfered
			{
			    r_fsm_in[i] = INFSM_ALLOC_THIRD;
			}
		    }
		    break;
		}
		case INFSM_ALLOC_THIRD:  // broadcast / third output port allocated
		{
		    data_in[i] = r_fifo_in[i].read();
		    put_in[i]  = r_fifo_in[i].rok();
		    req_in[i]  = REQ_NOP;
		    if( (get_out[r_index_in[i].read()] == i ) 
			 and r_fifo_in[i].rok() )	       // data flit transfered
		    {
			if ( not r_fifo_in[i].read().eop )
			{
			    std::cout << "ERROR in DSPIN_ROUTER/3 " << name()
				      << " : broadcast packet must be 2 flits" << std::endl;
			}
			r_fsm_in[i] = INFSM_REQ_FOURTH;
		    }
		    break;
		}
		case INFSM_REQ_FOURTH: // broacast / waiting fourth output port allocation
		{
		    data_in[i]    = r_buf_in[i];
		    put_in[i]     = true;
		    req_in[i]     = broadcast_route(4, i, r_buf_in[i].data);
		    r_index_in[i] = req_in[i];
		    if ( req_in[i] == REQ_NOP )  // no transfer for this step
		    {
			if (m_is_elevator) {
				r_fsm_in[i]     = INFSM_REQ_FIFTH;
			} else {
				fifo_in_read[i] = true;
				r_fsm_in[i]     = INFSM_IDLE;
			}
		    }
		    else
		    {
			if( get_out[req_in[i]] == i )  // header flit transfered
			{
			    r_fsm_in[i] = INFSM_ALLOC_FOURTH;
			}
		    }
		    break;
		}
		case INFSM_ALLOC_FOURTH:  // broadcast / fourth output port allocated
		{
		    data_in[i] = r_fifo_in[i].read();
		    put_in[i]  = r_fifo_in[i].rok();
		    req_in[i]  = REQ_NOP;
		    if( (get_out[r_index_in[i].read()] == i ) 
			 and r_fifo_in[i].rok() )		 // data flit transfered
		    {
			if ( not r_fifo_in[i].read().eop )
			{
			    std::cout << "ERROR in DSPIN_ROUTER/4 " << name()
				      << " : broadcast packet must be 2 flits" << std::endl;
			}
			if (m_is_elevator) {
				r_fsm_in[i]     = INFSM_REQ_FIFTH;
			} else {
				fifo_in_read[i] = true;
				r_fsm_in[i]     = INFSM_IDLE;
			}
		    }
		    break;
		}
		case INFSM_REQ_FIFTH: // broacast / waiting fifth output port allocation
		{
		    data_in[i]    = r_buf_in[i];
		    put_in[i]     = true;
		    req_in[i]     = broadcast_route(5, i, r_buf_in[i].data);
		    r_index_in[i] = req_in[i];
		    if ( req_in[i] == REQ_NOP )  // no transfer for this step
		    {
			r_fsm_in[i]     = INFSM_REQ_SIXTH;
		    }
		    else
		    {
			if( get_out[req_in[i]] == i )  // header flit transfered
			{
			    r_fsm_in[i] = INFSM_ALLOC_FIFTH;
			}
		    }
		    break;
		}
		case INFSM_ALLOC_FIFTH:  // broadcast / fifth output port allocated
		{
		    data_in[i] = r_fifo_in[i].read();
		    put_in[i]  = r_fifo_in[i].rok();
		    req_in[i]  = REQ_NOP;
		    if( (get_out[r_index_in[i].read()] == i ) 
			 and r_fifo_in[i].rok() )		 // data flit transfered
		    {
			if ( not r_fifo_in[i].read().eop )
			{
			    std::cout << "ERROR in DSPIN_ROUTER/5 " << name()
				      << " : broadcast packet must be 2 flits" << std::endl;
			}
			r_fsm_in[i]     = INFSM_REQ_SIXTH;
		    }
		    break;
		}
		case INFSM_REQ_SIXTH: // broacast / waiting fifth output port allocation
		{
		    data_in[i]    = r_buf_in[i];
		    put_in[i]     = true;
		    req_in[i]     = broadcast_route(6, i, r_buf_in[i].data);
		    r_index_in[i] = req_in[i];
		    if ( req_in[i] == REQ_NOP )  // no transfer for this step
		    {
			fifo_in_read[i] = true;
			r_fsm_in[i]     = INFSM_IDLE;
		    }
		    else
		    {
			if( get_out[req_in[i]] == i )  // header flit transfered
			{
			    r_fsm_in[i] = INFSM_ALLOC_SIXTH;
			}
		    }
		    break;
		}
		case INFSM_ALLOC_SIXTH:  // broadcast / fifth output port allocated
		{
		    data_in[i] = r_fifo_in[i].read();
		    put_in[i]  = r_fifo_in[i].rok();
		    req_in[i]  = REQ_NOP;
		    if( (get_out[r_index_in[i].read()] == i ) 
			 and r_fifo_in[i].rok() )		 // data flit transfered
		    {
			if ( not r_fifo_in[i].read().eop )
			{
			    std::cout << "ERROR in DSPIN_ROUTER/6 " << name()
				      << " : broadcast packet must be 2 flits" << std::endl;
			}
			fifo_in_read[i] = true;
			r_fsm_in[i]     = INFSM_IDLE;
		    }
		    break;
		}
	    } // end switch
	} // end for input ports
				   
	// loop on the output ports :
	// The r_alloc_out[j] and r_index_out[j] computation
	// implements the round-robin allocation policy.
	// These two registers implement a 10 states FSM.
	for( size_t j = 0 ; j < m_nports ; j++ ) 
	{
		if( not r_alloc_out[j].read() )  // not allocated: possible new allocation
		{
			for( size_t k = r_index_out[j].read() + 1 ; 
			     k < (r_index_out[j] + m_nports + 1) ; k++) 
			{ 
				size_t i = k % m_nports;
				if( req_in[i] == j ) 
				{
					r_alloc_out[j] = true;
					r_index_out[j] = i;
					break;
				}
			} // end loop on input ports
		} 
		else			    // allocated: possible desallocation
		{
			if ( data_in[r_index_out[j]].eop and
			     r_fifo_out[j].wok() and 
			     put_in[r_index_out[j]] ) 
			{
				r_alloc_out[j] = false;
			}
		}
	} // end loop on output ports

	// loop on the output ports :
	// The fifo_out_write[j] and fifo_out_wdata[j] computation
	// implements the output port mux.
	    for( size_t j = 0 ; j < m_nports ; j++ ) 
	{
		if( r_alloc_out[j] )  // output port allocated
		{
			fifo_out_write[j] = put_in[r_index_out[j]];
			fifo_out_wdata[j] = data_in[r_index_out[j]];
		}
	}  // end loop on the output ports

	//  FIFOS update
	for(size_t i = 0 ; i < m_nports ; i++) 
	{
		r_fifo_in[i].update(fifo_in_read[i],
				fifo_in_write[i],
				fifo_in_wdata[i]);
		r_fifo_out[i].update(fifo_out_read[i],
				fifo_out_write[i],
				fifo_out_wdata[i]);
	}
    } // end transition

    ////////////////////////////////
    //      genMoore
    ////////////////////////////////
    tmpl(void)::genMoore()
    {
	for(size_t i = 0 ; i < m_nports ; i++) 
	{ 
		// input ports : READ signals
		p_in[i].read = r_fifo_in[i].wok();
      
	 	// output ports : DATA & WRITE signals
		p_out[i].data  = r_fifo_out[i].read().data; 
		p_out[i].eop   = r_fifo_out[i].read().eop; 
		p_out[i].write = r_fifo_out[i].rok();
	}
    } // end genMoore

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
