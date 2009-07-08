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
 * Authors  : Noé Girand noe.girand@polytechnique.org
 * Date     : june 2009
 * Copyright: UPMC - LIP6
 */

#ifndef VIRTUAL_DSPIN_ROUTER_H_
#define VIRTUAL_DSPIN_ROUTER_H_

#include <systemc>
#include "caba_base_module.h"
#include "generic_fifo.h"
#include "dspin_interface.h"

namespace soclib 
{ 
	namespace caba 
	{
		using namespace sc_core;

		enum{
			LOCAL	= 0,
			NORTH	= 1,
			SOUTH	= 2,
			EAST	= 3,
			WEST	= 4,
			NOP    	= 5,
		};

		enum{	// INFSM States
			FSM_REQ,
			FSM_DTN,
			FSM_DT_LOCAL,
			FSM_REQ_NORTH,
			FSM_DT_NORTH,
			FSM_REQ_SOUTH,
			FSM_DT_SOUTH,
			FSM_REQ_EAST,
			FSM_DT_EAST,
			FSM_REQ_WEST,
			FSM_DT_WEST,
		};

		template<int x_size, int y_size> 
		class clusterCoordinates
		{
			public :
				sc_uint<x_size>	x;
				sc_uint<y_size> y;
		};

		template<	int data_size, 			// Size of flit
				int io_mask_offset, 		// Emplacement of IO checking
				int io_mask_size, 		// Size of IO checking
				int io_number_offset, 		// Emplacement of IO index in IO table
				int io_number_size, 		// Size of IO index
				int x_adressing_offset,		// Emplacement of target x in first flit
				int x_adressing_size, 		// Size of target x
				int y_adressing_offset, 	// Emplacement of target y in first flit
				int y_adressing_size, 		// Size of target y
				int eop_offset,			// Emplacement of eop checking
				int broadcast_offset, 		// Emplacement of broadcast checking
				int in_fifo_size, 		// 
				int out_fifo_size		// 
		>
		class VirtualDspinRouter: public soclib::caba::BaseModule
		{			

		protected:
			SC_HAS_PROCESS(VirtualDspinRouter);

		public:

			// ports
			sc_in<bool>             p_clk;
			sc_in<bool>             p_resetn;

			DspinOutput<data_size>	**p_out;
			DspinInput<data_size>	**p_in;

			// constructor 
			VirtualDspinRouter( sc_module_name  insname, 
				int		x,
				int		y,
				bool		n,							// North connexion enabled
				bool		s,							// South connexion enabled
				bool		e,							// East connexion enabled
				bool		w,							// West connexion enabled
				bool		broadcast0,						// Broadcast activated for channel 0
				bool		broadcast1,						// Broadcast activated for channel 1
				bool		io0,							// IO check for channel 0
				bool		io1,							// IO check for channel 1
				clusterCoordinates<x_adressing_size, y_adressing_size> * aIO_table	// List of IO clusters, NULL if unused.
			);

			// destructor 
			~VirtualDspinRouter();

		private:

			// IO Maptable
			clusterCoordinates<x_adressing_size, y_adressing_size> * m_IO_table;

			// internal registers
			sc_signal<int>			r_output_index[2][5];	// for each channel & each output, input index
			sc_signal<bool>			r_input_alloc[2][5];	// for each channel & each input, alloc  
			sc_signal<bool>           	r_tdm[5];		// for each input, Time Multiplexing
			sc_signal<sc_uint<data_size> >	r_buf[2][5];		// for each channel & each input, fifo extension
			sc_signal<int>			r_infsm[2][5];		// for each channel & each input FSM state

			// input fifos 
			soclib::caba::GenericFifo<sc_uint<data_size> > **in_fifo;
			soclib::caba::GenericFifo<sc_uint<data_size> > **out_fifo;

			// structural variables
			bool	m_broadcast[2];				// broadcast activated (for the two channels)
			bool	m_io[2];				// io activated (for the two channels)
			int	m_local_x;				// router x coordinate
			int	m_local_y;				// router y coordinate

			// Connexion branches
			bool	m_c[5];					// External connexion enabled on i

			// methods 
			void transition();
			void genMoore();

			// Internal function to improve code's lisibility
			int decode(sc_uint<data_size> adata, int ASHIFT, int AMASK);	// Extract information in adata
			int route(sc_uint<data_size> data, bool aio);			// Routin function

			int f_input_req(int target, int i);				// To calculate input_req
			int f_infsm(int target, int i);					// To calculate next infsm

			bool is_eop(sc_uint<data_size> data);				// Is data eop ?
			bool is_broadcast(sc_uint<data_size> data);			// Is data broadcast ?
		};
	}
} // end namespace

#endif // VIRTUAL_DSPIN_ROUTER_H_
