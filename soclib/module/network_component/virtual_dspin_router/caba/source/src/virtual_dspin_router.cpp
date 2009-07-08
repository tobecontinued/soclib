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

#include "virtual_dspin_router.h"
#include <cstdlib>
#include <cassert>
#include <sstream>
#include "alloc_elems.h"
#include <new>

// Set to 1 to active debugging
#define VIRTUAL_DSPIN_ROUTER_DEBUG 0

#define my_print std::cout << sc_time_stamp() << " -- " << name() << " : " <<
#define input_print "input infsm [" << k << "][" << i << "] : " <<
#define output_print "output infsm [" << k << "][" << j << "] : " <<

namespace soclib 
{ 
	namespace caba 
	{
		#define tmpl(x) template<int data_size, int io_mask_offset, int io_mask_size, int io_number_offset, int io_number_size, int x_addressing_offset, int x_addressing_size, int y_addressing_offset, int y_addressing_size, int eop_offset, int broadcast_offset, int in_fifo_size, int out_fifo_size, int x_min_offset, int x_max_offset, int y_min_offset, int y_max_offset> x VirtualDspinRouter<data_size, io_mask_offset, io_mask_size, io_number_offset, io_number_size,  x_addressing_offset, x_addressing_size, y_addressing_offset, y_addressing_size, eop_offset, broadcast_offset, in_fifo_size, out_fifo_size, x_min_offset, x_max_offset, y_min_offset, y_max_offset>

		//////////////////////////////////////////////////////
		// Direct routing function
		// As this function implements the X-first algorithm,
		// the input port index is not used.
		//////////////////////////////////////////////////////

		#define IO_MASK		(0x7FFFFFFF >> (31 - io_mask_size))
		#define IO_SHIFT	(data_size - io_mask_size - io_mask_offset)

		#define IO_NUMBER_MASK	(0x7FFFFFFF >> (31 - io_number_size))
		#define IO_NUMBER_SHIFT	(data_size - io_number_offset - io_number_size)

		#define EOP_MASK	0x1 				
		#define EOP_SHIFT	(data_size - 1 - eop_offset)

		#define BROADCAST_MASK	0x1
		#define BROADCAST_SHIFT	(data_size - 1 - broadcast_offset)			

		#define XMASK 		(0x7FFFFFFF >> (31 - x_addressing_size))
		#define XSHIFT 		(data_size - x_addressing_offset - x_addressing_size)

		#define YMASK		(0x7FFFFFFF >> (31 - y_addressing_size))
		#define YSHIFT 		(data_size - y_addressing_offset - y_addressing_size)

		#define X_MIN_SHIFT	(data_size - x_min_offset - x_addressing_size)
		#define X_MAX_SHIFT	(data_size - x_max_offset - x_addressing_size)
		#define Y_MIN_SHIFT	(data_size - y_min_offset - y_addressing_size)
		#define Y_MAX_SHIFT	(data_size - y_max_offset - y_addressing_size)

		
		// Auxiliar function definitions /////////////////////////////////////////////////////////////////////////

		tmpl(int)::decode(sc_uint<data_size> adata, int ASHIFT, int AMASK)
		{
			return (int)((adata >> ASHIFT) & AMASK);
		}

		tmpl(int)::route(sc_uint<data_size> data, bool aio)
		{
			int xdest;
			int ydest;
			// Priority to IO
			if((aio)&&(decode(data, IO_SHIFT, IO_MASK) == IO_MASK)){
				// IO adressing
				int io_number = decode(data, IO_NUMBER_SHIFT, IO_NUMBER_MASK);
				xdest = m_IO_table[io_number].x;
				ydest = m_IO_table[io_number].y;
			}else{
				// Non IO (most common) adressing
				xdest = decode(data, XSHIFT, XMASK);
				ydest = decode(data, YSHIFT, YMASK);
			}
			return (xdest < m_local_x ? WEST : (xdest > m_local_x ? EAST : (ydest < m_local_y ? SOUTH : (ydest > m_local_y ? NORTH : LOCAL))));
		}

		tmpl(bool)::f_bound_check(int dir, int x_min, int y_min, int x_max, int y_max)
		{
			switch(dir){
				case LOCAL	: return (true);
				case NORTH	: return (m_local_y < y_max);
				case SOUTH	: return (m_local_y > y_min);
				case EAST	: return (m_local_x < x_max);
				case WEST	: return (m_local_x > x_min);
			}
		}

		tmpl(int)::f_input_req(int target, int i, sc_uint<data_size> data)
		{
				bool b = broadcast_parity(data);
				int __11 = r_o(_11, b);
				int __12 = r_o(_12, b);
				int __21 = r_o(_21, b);
				int __22 = r_o(_22, b);
				int x_min = decode(data, X_MIN_SHIFT, XMASK);
				int y_min = decode(data, Y_MIN_SHIFT, YMASK);
				int x_max = decode(data, X_MAX_SHIFT, XMASK);
				int y_max = decode(data, Y_MAX_SHIFT, YMASK);
				if((m_c[__11])&&(i!=__11)&&(target<=_11)&&f_bound_check(__11, x_min, y_min, x_max, y_max)&&(i!=__21)&&(i!=__22))		// Ok to _11
					return r_o(_11,b);
				if((m_c[__12])&&(i!=__12)&&(target<=_12)&&f_bound_check(__12, x_min, y_min, x_max, y_max)&&(i!=__21)&&(i!=__22))		// Ok to _12
					return r_o(_12,b);
				if((m_c[__21])&&(i!=__21)&&(target<=_21)&&f_bound_check(__21, x_min, y_min, x_max, y_max))					// Ok to _21
					return r_o(_21,b);
				if((m_c[__22])&&(i!=__22)&&(target<=_22)&&f_bound_check(__22, x_min, y_min, x_max, y_max))					// Ok to _22
					return r_o(_22,b);
				if((m_c[LOCAL])&&(i!=LOCAL)&&(target<=_LOCAL)&&f_bound_check(LOCAL, x_min, y_min, x_max, y_max))				// Ok to _local
					return LOCAL;
		}

		tmpl(int)::f_infsm(int target, int i, sc_uint<data_size> data)
		{
			bool b = broadcast_parity(data);
			int __12 = r_o(_12, b);
			int __21 = r_o(_21, b);
			int __22 = r_o(_22, b);
			int x_min = decode(data, X_MIN_SHIFT, XMASK);
			int y_min = decode(data, Y_MIN_SHIFT, YMASK);
			int x_max = decode(data, X_MAX_SHIFT, XMASK);
			int y_max = decode(data, Y_MAX_SHIFT, YMASK);
			if((m_c[__12])&&(i!=__12)&&(target<=_12)&&f_bound_check(__12, x_min, y_min, x_max, y_max)&&(i!=__21)&&(i!=__22))
				return FSM_REQ_12;
			if((m_c[__21])&&(i!=__21)&&(target<=_21)&&f_bound_check(__21, x_min, y_min, x_max, y_max))
				return FSM_REQ_21;
			if((m_c[__22])&&(i!=__22)&&(target<=_22)&&f_bound_check(__22, x_min, y_min, x_max, y_max))
				return FSM_REQ_22;
			if((m_c[LOCAL])&&(i!=LOCAL)&&(target<=_LOCAL)&&f_bound_check(LOCAL, x_min, y_min, x_max, y_max))
				return FSM_REQ_LOCAL;
			return FSM_REQ;
		}

		tmpl(bool)::is_eop(sc_uint<data_size> data)
		{
			return decode(data, EOP_SHIFT, EOP_MASK);
		}

		tmpl(bool)::is_broadcast(sc_uint<data_size> data)
		{
			return (decode(data, BROADCAST_SHIFT, BROADCAST_MASK) != 0);
		}

		tmpl(int)::r_o(int target, bool b)
		{
			if(b){
				switch(target){
					case _11 : return EAST;
					case _12 : return WEST;
					case _21 : return NORTH;
					case _22 : return SOUTH;
					case _LOCAL : return LOCAL;
				}
			}else{
				switch(target){
					case _11 : return NORTH;
					case _12 : return SOUTH;
					case _21 : return EAST;
					case _22 : return WEST;
					case _LOCAL : return LOCAL;
				}
			}
		}

		tmpl(bool)::broadcast_parity(sc_uint<data_size> data)
		{
			return (bool)(((decode(data, XSHIFT, XMASK) + decode(data, YSHIFT, YMASK)) & 1) == 0);
		}

		template<typename elem_t>
		elem_t** alloc_elems2(const std::string &prefix, bool * m)
		{
		    elem_t **elem = (elem_t**)malloc(sizeof(elem_t*)*2);
		    for(int k=0; k<2; k++){
			elem[k] = (elem_t*)malloc(sizeof(elem_t)*5);
			for(int i=0; i<5; i++){
				if(m[i]){
					std::ostringstream o;
					o << prefix << "[" << k << "]" << "[" << i << "]";
					new(&elem[k][i]) elem_t(o.str()) ;
				}
			}
		    }
	 	   return elem;
		}

		template<typename elem_t>
		void dealloc_elems2(elem_t **elems, bool * m)
		{
			for(int k = 0; k<2;k ++){
				for(int i=0; i<5; i++)
					if(m[i]) 
						elems[k][i].~elem_t();
				free(elems[k]);
		    	}
			free(elems);
		}

		// End of auxiliar functions /////////////////////////////////////////////////////////////////////////

		////////////////////////////////
		//      constructor
		////////////////////////////////
		tmpl(/**/)::VirtualDspinRouter(sc_module_name insname, 
			int x,
			int y,
			bool		n,
			bool		s,
			bool		e,
			bool		w,
			bool broadcast0,
			bool broadcast1,
			bool		io0,
			bool		io1,
			clusterCoordinates<x_addressing_size, y_addressing_size> * aIO_table)
			: soclib::caba::BaseModule(insname),
			p_clk("clk"),
      			p_resetn("resetn")
		{
			SC_METHOD (transition);
			dont_initialize();
			sensitive << p_clk.pos();

			SC_METHOD (genMoore);
			dont_initialize();
			sensitive  << p_clk.neg();

			m_c[LOCAL] = true;
			m_c[NORTH] = n;
			m_c[SOUTH] = s;
			m_c[EAST] = e;
			m_c[WEST] = w;

			p_in  = alloc_elems2<DspinInput<data_size> >("p_in", m_c);
			p_out = alloc_elems2<DspinOutput<data_size> >("p_out", m_c);

			
			in_fifo = (soclib::caba::GenericFifo<sc_uint<data_size> >**) malloc(sizeof(soclib::caba::GenericFifo<sc_uint<data_size> >*)*2);
			out_fifo = (soclib::caba::GenericFifo<sc_uint<data_size> >**) malloc(sizeof(soclib::caba::GenericFifo<sc_uint<data_size> >*)*2);

			for(int k=0; k<2; k++){
				in_fifo[k] = (soclib::caba::GenericFifo<sc_uint<data_size> >*) malloc(sizeof(soclib::caba::GenericFifo<sc_uint<data_size> >)*5);
				out_fifo[k] = (soclib::caba::GenericFifo<sc_uint<data_size> >*) malloc(sizeof(soclib::caba::GenericFifo<sc_uint<data_size> >)*5);
				for(int i=0; i<5; i++) if(m_c[i]){
					std::ostringstream o;
					o << name() << "_" << k << i;
					new(&in_fifo[k][i]) soclib::caba::GenericFifo<sc_uint<data_size> >(std::string("IN_FIFO_")+o.str(), in_fifo_size) ;
		    			new(&out_fifo[k][i]) soclib::caba::GenericFifo<sc_uint<data_size> >(std::string("OUT_FIFO_")+o.str(), out_fifo_size) ;
				}
			}

			m_local_x 	= x;
			m_local_y 	= y;
			m_broadcast[0] 	= broadcast0;
			m_broadcast[1] 	= broadcast1;
			m_io[0] = io0;
			m_io[1] = io1;
			m_IO_table = aIO_table;			

		} //  end constructor

		tmpl(/**/)::~VirtualDspinRouter()
		{
			dealloc_elems2(p_out, m_c);
			dealloc_elems2(p_in, m_c);
			dealloc_elems2(out_fifo, m_c);
			dealloc_elems2(in_fifo, m_c);
		}

		////////////////////////////////
		//      transition
		////////////////////////////////
		tmpl(void)::transition()
		{
			if(p_resetn.read()){

				// internal variables used in each input port module
				bool	        	in_fifo_read[2][5];	// for each channel & each input : consume data in fifo 
				bool	        	in_fifo_write[2][5];	// for each channel & each input : write data in fifo
				bool 			put[2][5];		// for each channel, the input port wishes to transmit data
				sc_uint<data_size> 	data[2][5]; 		// for each channel, data to be transmitted
				bool			get[2][5];
				bool			sending[2][5];

				// internal variables used in each output port module
				bool	        	out_fifo_write[2][5]; 	// for each channel & each output : write data in fifo 
				bool	        	out_fifo_read[2][5]; 	// for each channel & each output : consume data in fifo 
				sc_uint<data_size>	output_data[2][5];    	// for each channel & each output : data written into fifo

				// signals supporting the communications between input port modules & output port modules
				bool            	output_get[2][5][5];  // output j consume data from input i in channel k 
				int	        	input_req[2][5];      // for each channel & each input : requested output port (the NOP value means no request)
				sc_uint<data_size>	final_data[5];        // for each input : data value 
				bool            	final_write[2][5];    // for each input : data valid for channel 0 & 1

				////////////////////////////////////////////////
				// Updating internal signals
				////////////////////////////////////////////////

				////////////////////////////////////////////////
				// Output FSM 
				// List of parameters to change
				// 	output_get
				////////////////////////////////////////////////

				for(int k=0; k<2; k++){
					for(int i=0; i<5; i++) if(m_c[i]) {
						for(int j=0; j<5; j++) if(m_c[j]){
							output_get[k][i][j] = (r_output_index[k][j].read() == i) && out_fifo[k][j].wok();
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								if(!out_fifo[k][j].wok())
									my_print output_print "Fifo de sortie pleine." << std::endl;
							#endif
						}
					}
				}

				////////////////////////////////////////////////
				// Input FSM 
				// List of parameters to change
				// 	data, put, final_write, final_data, get, sending
				////////////////////////////////////////////////

				for(int i=0; i<5; i++) if(m_c[i]){
					for(int k=0; k<2; k++){
						if((r_infsm[k][i].read()==FSM_REQ_12)||(r_infsm[k][i].read()==FSM_REQ_21)||(r_infsm[k][i].read()==FSM_REQ_22)||(r_infsm[k][i].read()==FSM_REQ_LOCAL))
						{
							data[k][i] = r_buf[k][i].read();
							put[k][i] = true;
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								if(put[k][i])
									my_print input_print "Nouvelle donnée dans le buffer à transmettre : " << std::hex << data[k][i] << std::dec << std::endl;
							#endif
						}else{
							data[k][i] = in_fifo[k][i].read();
							put[k][i] = in_fifo[k][i].rok();
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								if(put[k][i])
									my_print input_print "Nouvelle donnée dans la fifo à transmettre : " << std::hex << data[k][i] << std::dec << std::endl;
							#endif
						}
					} // end loop channels 

					// Time multiplexing
					bool tdm;			// channel 1 has priority when tdm is true
					tdm = r_tdm[i].read();
					final_write[0][i] = put[0][i]&&(!tdm||!put[1][i]);
					final_write[1][i] = put[1][i]&&( tdm||!put[0][i]);
					if(final_write[1][i]) 
						final_data[i] = data[1][i];
					else                                
						final_data[i] = data[0][i];

					for(int k=0; k<2; k++){
						get[k][i] = (m_c[0]&&output_get[k][i][0])||(m_c[1]&&output_get[k][i][1])||(m_c[2]&&output_get[k][i][2])||(m_c[3]&&output_get[k][i][3])||(m_c[4]&&output_get[k][i][4]);
						sending[k][i] = get[k][i]&&final_write[k][i];
						input_req[k][i] = NOP; // Default value
						if(!sending[k][i]){
							switch(r_infsm[k][i].read()){
								case FSM_REQ:
									if(put[k][i]){
										if(is_broadcast(data[k][i])){
											if(m_broadcast[k]){
												input_req[k][i] = f_input_req(_11, i, data[k][i]);
												r_buf[k][i] = data[k][i];
											}else
												my_print input_print "broadcast forbidden" << std::endl;
										}else
											input_req[k][i]	= route(data[k][i], m_io[k]);
									}
									break;

								case FSM_REQ_12:	input_req[k][i] = f_input_req(_12, i, r_buf[k][i]);	break;

								case FSM_REQ_21:	input_req[k][i] = f_input_req(_21, i, r_buf[k][i]);	break;

								case FSM_REQ_22:	input_req[k][i] = f_input_req(_22, i, r_buf[k][i]);	break;

								case FSM_REQ_LOCAL:	input_req[k][i] = f_input_req(_LOCAL, i, r_buf[k][i]);break;
							} // end switch infsm
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								if(input_req[k][i]!=NOP) 
									my_print input_print "Demande la sortie : " << input_req[k][i] << std::endl;
							#endif
						}
						#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
							if(r_infsm[k][i].read()!=FSM_REQ) 
								my_print input_print "est dans l'état " << r_infsm[k][i].read() << std::endl;
						#endif
					} // end network loop
				} // end input loop

				////////////////////////////////////////////////
				// Updating internal registers and fifos
				////////////////////////////////////////////////

				////////////////////////////////////////////////
				// Input FSM 
				// List of parameters to change
				// 	Registers :
				// 		r_tdm, r_infsm, , r_buf, 
				//	Other signals :
				//		in_fifo_read, in_fifo_write
				////////////////////////////////////////////////

				for(int i=0; i<5; i++) if(m_c[i]){

					// Time multiplexing
					r_tdm[i] = (r_tdm[i].read() && !put[1][i])||(!r_tdm[i].read() && put[0][i]);

					// For each network
					for(int k=0; k<2; k++){
						in_fifo_write[k][i] = p_in[k][i].write.read();
						in_fifo_read[k][i] = false;  // Default value
						int n_infsm = -1;
						if(sending[k][i]){
							switch(r_infsm[k][i].read()){
								case FSM_REQ:
									in_fifo_read[k][i] = true;				
									if(is_broadcast(data[k][i])){
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "FSM_IDLE : nouveau broadcast" << std::endl;
										#endif
										bool b = broadcast_parity(data[k][i]);
										int __11 = r_o(_11, b);
										int __12 = r_o(_12, b);
										int __21 = r_o(_21, b);
										int __22 = r_o(_22, b);
										int x_min = decode(data[k][i], X_MIN_SHIFT, XMASK);
										int y_min = decode(data[k][i], Y_MIN_SHIFT, YMASK);
										int x_max = decode(data[k][i], X_MAX_SHIFT, XMASK);
										int y_max = decode(data[k][i], Y_MAX_SHIFT, YMASK);
										if((m_c[__11])&&(i!=__11)&&f_bound_check(__11, x_min, y_min, x_max, y_max)&&(i!=__21)&&(i!=__22))
											r_infsm[k][i] = FSM_DT_11;
										else if((m_c[__12])&&(i!=__12)&&f_bound_check(__12, x_min, y_min, x_max, y_max)&&(i!=__21)&&(i!=__22))
											r_infsm[k][i] = FSM_DT_12;
										else if((m_c[__21])&&(i!=__21)&&f_bound_check(__21, x_min, y_min, x_max, y_max))
											r_infsm[k][i] = FSM_DT_21;
										else if((m_c[__22])&&(i!=__22)&&f_bound_check(__22, x_min, y_min, x_max, y_max))	
											r_infsm[k][i] = FSM_DT_22;
										else if((m_c[LOCAL])&&(i!=LOCAL)&&f_bound_check(LOCAL, x_min, y_min, x_max, y_max))
											r_infsm[k][i] = FSM_DT_LOCAL;
									}else{
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "Passe de REQ à DTN" << std::endl;
										#endif
										r_infsm[k][i] = FSM_DTN;
									}
									break;

								case FSM_DTN:
									in_fifo_read[k][i] = true;
									if(is_eop(data[k][i])){
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "Passe de DTN à REQ" << std::endl;
										#endif
										r_infsm[k][i] = FSM_REQ;
									}
									break;

								case FSM_DT_11:		n_infsm = f_infsm(_12, i, r_buf[k][i]);		break;

								case FSM_REQ_12:	r_infsm[k][i] = FSM_DT_12;			break;

								case FSM_DT_12:		n_infsm = f_infsm(_21, i, r_buf[k][i]);		break;

								case FSM_REQ_21:	r_infsm[k][i] = FSM_DT_21;			break;

								case FSM_DT_21:		n_infsm = f_infsm(_22, i, r_buf[k][i]);		break;

								case FSM_REQ_22:	r_infsm[k][i] = FSM_DT_22;			break;

								case FSM_DT_22:		n_infsm = f_infsm(_LOCAL, i, r_buf[k][i]);	break;

								case FSM_REQ_LOCAL:	r_infsm[k][i] = FSM_DT_LOCAL;			break;

								case FSM_DT_LOCAL:	n_infsm = f_infsm(_LOCAL+1, i, r_buf[k][i]);	break;

							} // end switch infsm
						}
						if(n_infsm>=0){
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								my_print input_print "Passe dans l'etat " << n_infsm << std::endl;
							#endif
							r_infsm[k][i] = n_infsm;
							in_fifo_read[k][i] = (n_infsm == FSM_REQ);
						}
						 #if VIRTUAL_DSPIN_ROUTER_DEBUG == 1 
							if(!sending[k][i] && put[k][i]) 
								my_print input_print "get = " << get[k][i] << " ; final_write = " << final_write[k][i] << std::endl; 
						#endif
					} // end network loop
				} // end input loop

				////////////////////////////////////////////////
				// Output FSM 
				// List of parameters to change
				// 	Registers :
				// 		r_output_index , r_input_alloc
				//	Other signals :
				//		out_fifo_read, out_fifo_write
				////////////////////////////////////////////////

				for(int j=0; j<5; j++) if(m_c[j]){
					for(int k=0; k<2; k++){
						out_fifo_write[k][j] = false;
						for(int i=0; i<5; i++) if(m_c[i]){
							if(output_get[k][i][j] && final_write[k][i]){
								output_data[k][j] = final_data[i];
								out_fifo_write[k][j] = true;
								#if VIRTUAL_DSPIN_ROUTER_DEBUG == 2
									my_print output_print "Transfert de la donnée entrant en " << i << std::endl;
								#endif
							}
						}
						out_fifo_read[k][j] = p_out[k][j].read;

						int index = r_output_index[k][j].read();
						if(index>=NOP_LOCAL)
						{ // allocation
							for(int n = index+1; n < index+6; n++){ 
								int i = n % 5;
								if(m_c[i]&&(!r_input_alloc[k][i])&&(input_req[k][i]==j)){
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print output_print "Allocation de la sortie à " << i << " qui demande " << input_req[k][i] << std::endl;
									#endif
									r_output_index[k][j] = i;
									r_input_alloc[k][i] = true;
									break;
								}
							}
						}else if(is_eop(output_data[k][j]) && out_fifo_write[k][j] && out_fifo[k][j].wok())
						{ // Deallocation
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								my_print output_print "Désallocation de " << index << std::endl;
							#endif
							r_input_alloc[k][index] = false;
							r_output_index[k][j] = index + 5;
						}
					} // end network loop
				} // end output loop

				//  Updating fifos
				for(int k=0; k<2; k++){			
					for(int i=0; i<5; i++) if(m_c[i]){
						if((in_fifo_write[k][i])&&(in_fifo_read[k][i]))
							in_fifo[k][i].put_and_get(p_in[k][i].data.read());
						else if((in_fifo_write[k][i])&&(!in_fifo_read[k][i]))
							in_fifo[k][i].simple_put(p_in[k][i].data.read());
						else if((!in_fifo_write[k][i])&&(in_fifo_read[k][i]))
							in_fifo[k][i].simple_get();

						if((out_fifo_write[k][i])&&(out_fifo_read[k][i]))
							out_fifo[k][i].put_and_get(output_data[k][i]);
						else if((out_fifo_write[k][i])&&(!out_fifo_read[k][i]))
							out_fifo[k][i].simple_put(output_data[k][i]);
						else if((!out_fifo_write[k][i])&&(out_fifo_read[k][i]))
							out_fifo[k][i].simple_get();
					}
				}
			}else{	// reseting
				for(int i=0; i<5; i++) if(m_c[i]){
					r_tdm[i] = true;
					for(int k=0; k<2; k++){
						in_fifo[k][i].init();
						out_fifo[k][i].init();
						r_infsm[k][i] = FSM_REQ;
						r_input_alloc[k][i] = false;
						r_output_index[k][i] = NOP;
					}
				}
			}
		} // end transition

		////////////////////////////////
		//      genMoore
		////////////////////////////////
		tmpl(void)::genMoore()
		{
			for(int k=0; k<2; k++){
				for(int i=0; i<5; i++) if(m_c[i]){
					// input ports : read[k][i] signals
					p_in[k][i].read = in_fifo[k][i].wok();
				}

				for(int j=0; j<5; j++) if(m_c[j]){
					// output ports : data[k][j] & write[k][j] signals
					p_out[k][j].write = out_fifo[k][j].rok();
					p_out[k][j].data  = out_fifo[k][j].read();
				}
			}
		} // end genMoore
	} // end namespace caba
} // end namespace soclib
