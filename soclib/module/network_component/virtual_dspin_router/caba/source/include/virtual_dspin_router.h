/* -*- c++ -*-
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
 * Authors  : alain.greiner@lip6.fr noe.girand@polytechnique.org
 * Date     : july 2010
 * Copyright: UPMC - LIP6
 */

#ifndef VIRTUAL_DSPIN_ROUTER_H
#define VIRTUAL_DSPIN_ROUTER_H

#include <systemc>
#include "caba_base_module.h"
#include "generic_fifo.h"
#include "dspin_interface.h"

namespace soclib { namespace caba {

enum{  // port indexing
	NORTH	= 0,
	SOUTH	= 1,
	EAST	= 2,
	WEST	= 3,
	LOCAL	= 4,
};

enum{   // request type (six values, can be encoded on 3 bits)
        REQ_NORTH,
	REQ_SOUTH,
	REQ_EAST,
	REQ_WEST,
	REQ_LOCAL,
        REQ_NOP,
};

enum {  // step index for a broadcast (all broadcast have 4 steps)
	FIRST,
	SECOND,
	THIRD,
	FOURTH,
};

enum{	// INFSM States : In the REQ states, the request and the corresponding
        // data are sent simultaneously, which means at least two cycles in this state
	INFSM_REQ_FIRST,
	INFSM_DTN,
	INFSM_DT_FIRST,
	INFSM_REQ_SECOND,
	INFSM_DT_SECOND,
	INFSM_REQ_THIRD,
	INFSM_DT_THIRD,
	INFSM_REQ_FOURTH,
	INFSM_DT_FOURTH,
	};

template<int flit_width>
class VirtualDspinRouter: public soclib::caba::BaseModule
{			

protected:
	SC_HAS_PROCESS(VirtualDspinRouter);

public:

	// ports
	sc_core::sc_in<bool>             	p_clk;
	sc_core::sc_in<bool>             	p_resetn;
	soclib::caba::DspinOutput<flit_width>	**p_out;
	soclib::caba::DspinInput<flit_width>	**p_in;

	// constructor 
	VirtualDspinRouter( 	sc_module_name  insname,
				int	x,			// x coordinate in the mesh
				int	y,			// y coordinate in the mesh 
                                int	x_size,			// number of bits for x field
				int	y_size,			// number of bits for y field
                                int	in_fifo_depth,		// input fifo depth
				int	out_fifo_depth);	// output fifo depth

	// destructor 
	~VirtualDspinRouter();

private:

	// input port registers & fifos
	sc_core::sc_signal<bool>        		r_tdm[5];		// Time Multiplexing
	sc_core::sc_signal<int>				r_input_fsm[2][5];	// FSM state
	sc_core::sc_signal<sc_uint<flit_width> >		r_buf[2][5];		// fifo extension
	soclib::caba::GenericFifo<sc_uint<flit_width> > 	**in_fifo;		// input fifos

	// output port registers & fifos
	sc_core::sc_signal<int>				r_output_index[2][5];	// allocated input index 
	sc_core::sc_signal<bool>			r_output_alloc[2][5];	// allocation status 
	soclib::caba::GenericFifo<sc_uint<flit_width> > 	**out_fifo;		// output fifos

	// structural variables
	int	m_local_x;					// router x coordinate
	int	m_local_y;					// router y coordinate
        int	m_x_size;					// number of bits for x field
        int	m_y_size;					// number of bits for y field
        int	m_x_shift;					// number of bits to shift for x field
        int	m_x_mask;					// number of bits to mask for x field
        int	m_y_shift;					// number of bits to shift for y field
        int	m_y_mask;					// number of bits to mask for y field

	// methods 
	void transition();
	void genMoore();

	// Utility functions
        void	printTrace();
	int 	xfirst_route(sc_uint<flit_width> data);			
	int 	broadcast_route(int dst, int src, sc_uint<flit_width> data);	
	bool 	is_eop(sc_uint<flit_width> data);			
	bool 	is_broadcast(sc_uint<flit_width> data);

}; // end class VirtualDspinRouter
	
}} // end namespace

#endif // end VIRTUAL_DSPIN_ROUTER_H_
