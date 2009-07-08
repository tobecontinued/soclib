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
		#define tmpl(x) template<int data_size, int io_mask_offset, int io_mask_size, int io_number_offset, int io_number_size, int x_adressing_offset, int x_adressing_size, int y_adressing_offset, int y_adressing_size, int eop_offset, int broadcast_offset, int in_fifo_size, int out_fifo_size> x VirtualDspinRouter<data_size, io_mask_offset, io_mask_size, io_number_offset, io_number_size,  x_adressing_offset, x_adressing_size, y_adressing_offset, y_adressing_size, eop_offset, broadcast_offset, in_fifo_size, out_fifo_size>

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
		#define EOP_SHIFT	(data_size-1-eop_offset)

		#define BROADCAST_MASK	0x3
		#define BROADCAST_SHIFT	(data_size-2-broadcast_offset)			

		#define XMASK 		(0x7FFFFFFF >> (31 - x_adressing_size))
		#define XSHIFT 		(data_size - x_adressing_offset - x_adressing_size)

		#define YMASK		(0x7FFFFFFF >> (31 - y_adressing_size))
		#define YSHIFT 		(data_size - y_adressing_offset - y_adressing_size)
		
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
				int io_number = decode(data, IO_NUMBER_SHIFT, IO_NUMBER_MASK);
				xdest = m_IO_table[io_number].x;
				ydest = m_IO_table[io_number].y;
			}else{
				xdest = decode(data, XSHIFT, XMASK);
				ydest = decode(data, YSHIFT, YMASK);
			}
			return (xdest < m_local_x ? WEST : (xdest > m_local_x ? EAST : (ydest < m_local_y ? SOUTH : (ydest > m_local_y ? NORTH : LOCAL))));
		}

		tmpl(int)::f_input_req(int target, int i)
		{			
			if((m_c[LOCAL])&&(i!=LOCAL)&&(target<=LOCAL))					// Ok to local
				return LOCAL;
			else if((m_c[NORTH])&&(i!=NORTH)&&(target<=NORTH))				// Ok to north
				return NORTH;
			else if((m_c[SOUTH])&&(i!=SOUTH)&&(target<=SOUTH))				// Ok to south
				return SOUTH;
			else if((m_c[EAST])&&(i!=EAST)&&(i!=NORTH)&&(i!=SOUTH)&&(target<=EAST))		// Ok to east
				return EAST;
			else if((m_c[WEST])&&(i!=WEST)&&(i!=NORTH)&&(i!=SOUTH)&&(target<=WEST))		// Ok to west
				return WEST;
			else
				return NOP;
		}

		tmpl(int)::f_infsm(int target, int i)
		{			
			if((m_c[NORTH])&&(i!=NORTH)&&(target<=NORTH))					// Ok to north
				return FSM_REQ_NORTH;
			else if((m_c[SOUTH])&&(i!=SOUTH)&&(target<=SOUTH))				// Ok to south
				return FSM_REQ_SOUTH;
			else if((m_c[EAST])&&(i!=EAST)&&(i!=NORTH)&&(i!=SOUTH)&&(target<=EAST))		// Ok to east
				return FSM_REQ_EAST;
			else if((m_c[WEST])&&(i!=WEST)&&(i!=NORTH)&&(i!=SOUTH)&&(target<=WEST))		// Ok to west
				return FSM_REQ_WEST;
			else
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
			clusterCoordinates<x_adressing_size, y_adressing_size> * aIO_table)
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

			m_c[0] = true;
			m_c[1] = n;
			m_c[2] = s;
			m_c[3] = e;
			m_c[4] = w;

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

				// internal variables used in each output port module
				bool	        	out_fifo_write[2][5]; 	// for each channel & each output : write data in fifo 
				bool	        	out_fifo_read[2][5]; 	// for each channel & each output : consume data in fifo 
				sc_uint<data_size>	output_data[2][5];    	// for each channel & each output : data written into fifo

				// signals supporting the communications between input port modules & output port modules
				bool            	output_get[2][5][5];  // output j consume data from input i in channel k 

				int	        	input_req[2][5];      // for each channel & each input : requested output port (the NOP value means no request)
				sc_uint<data_size>	final_data[5];        // for each input : data value 
				bool            	final_write[2][5];    // for each input : data valid for channel 0 & 1

				//////////////////////////////////////
				// Input FSM Requests
				// Setting input_req and in_fifo_write
				//////////////////////////////////////
				for(int i=0; i<5; i++) if(m_c[i]){
					for(int k=0; k<2; k++){
						if((r_infsm[k][i].read()==FSM_REQ_NORTH)||(r_infsm[k][i].read()==FSM_REQ_SOUTH)||(r_infsm[k][i].read()==FSM_REQ_EAST)||(r_infsm[k][i].read()==FSM_REQ_WEST))
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
					bool	tdm;			// channel 1 has priority when tdm is true
					tdm = r_tdm[i].read();  
					r_tdm[i] = (tdm && !put[1][i])||(!tdm && put[0][i]);
					final_write[0][i] = put[0][i]&&(!tdm||!put[1][i]);
					final_write[1][i] = put[1][i]&&( tdm||!put[0][i]);
					if(final_write[1][i]) 
						final_data[i] = data[1][i];
					else                                
						final_data[i] = data[0][i];

					for(int k=0; k<2; k++){
						in_fifo_write[k][i] = p_in[k][i].write.read();
						input_req[k][i] = NOP; // Default value
						switch(r_infsm[k][i].read()){
							case FSM_REQ:
								if(put[k][i]){
									if(is_broadcast(data[k][i])){
										if(m_broadcast[k]){
											input_req[k][i] = f_input_req(LOCAL, i);
											r_buf[k][i] = data[k][i];
										}else my_print input_print "broadcast forbidden" << std::endl;

									}else{
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "FSM_IDLE " << std::endl;
										#endif
										input_req[k][i]	= route(data[k][i], m_io[k]);
									}
								}
								break;

							case FSM_REQ_NORTH:
								input_req[k][i] = f_input_req(NORTH, i);					
								break;

							case FSM_REQ_SOUTH:
								input_req[k][i] = f_input_req(SOUTH, i);					
								break;

							case FSM_REQ_EAST:
								input_req[k][i] = f_input_req(EAST, i);					
								break;

							case FSM_REQ_WEST:
								input_req[k][i] = f_input_req(WEST, i);					
								break;

						} // end switch infsm
						#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
							if(input_req[k][i]!=NOP) my_print input_print "Demande la sortie : " << input_req[k][i] << std::endl;
						#endif
					} // end network loop
				} // end input loop

				int	output_index[2][5];
				for(int k=0; k<2; k++) 
					for(int j=0; j<5; j++) if(m_c[j]) 
						output_index[k][j] = r_output_index[k][j]; 

				// Output allocations
				for(int j=0; j<5; j++) if(m_c[j]){
					for(int k=0; k<2; k++){
						int index = output_index[k][j];
						if(index==NOP){
							for(int n = index+1; n < index+6; n++){ 
								int i = n % 5;
								if(m_c[i]&&(!r_input_alloc[k][i])&&(input_req[k][i]==j)){
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print output_print "Allocation de la sortie à " << i << " qui demande " << input_req[k][i] << std::endl;
									#endif
									output_index[k][j] = i;
									r_input_alloc[k][i] = true;
									break;
								}
							}
						}
					}
				}

				// Setting get signals
				for(int k=0; k<2; k++) for(int i=0; i<5; i++) if(m_c[i]) for(int j=0; j<5; j++) if(m_c[j]){
					output_get[k][i][j] = (output_index[k][j] == i) && out_fifo[k][j].wok();
					#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
						if(!out_fifo[k][j].wok())
							my_print output_print "Fifo de sortie pleine." << std::endl;
					#endif
				}

				for(int k=0; k<2; k++) for(int j=0; j<5; j++) if(m_c[j]) 
					r_output_index[k][j] = output_index[k][j]; 

				// Output transfers and deallocations
				for(int j=0; j<5; j++) if(m_c[j]){
					for(int k=0; k<2; k++){

						// output mux implementation
						out_fifo_write[k][j] = false;
						for(int i=0; i<5; i++) if(m_c[i]){
							if(output_get[k][i][j] && final_write[k][i]){
								output_data[k][j] = final_data[i];
								out_fifo_write[k][j] = true;
								#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
									my_print output_print "Transfert de la donnée entrant en " << i << std::endl;
								#endif
							}
						}
						out_fifo_read[k][j] = p_out[k][j].read;

						// deallocation FSMs
						int index = r_output_index[k][j].read();
						if((index!=NOP)&&is_eop(output_data[k][j]) && out_fifo_write[k][j] && out_fifo[k][j].wok())
						{
							#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
								my_print output_print "Libération de " << index << std::endl;
							#endif
							r_input_alloc[k][index] = false;
							r_output_index[k][j] = NOP;
						}							
					}
				}

				// Setting input infsm and in_fifo_read
				for(int i=0; i<5; i++) if(m_c[i]){
					for(int k=0; k<2; k++) {

						bool get = (m_c[0]&&output_get[k][i][0])||(m_c[1]&&output_get[k][i][1])||(m_c[2]&&output_get[k][i][2])||(m_c[3]&&output_get[k][i][3])||(m_c[4]&&output_get[k][i][4]);
						bool sending = get&&final_write[k][i];

						// Default values
						in_fifo_read[k][i] = false;

						// Mise à jour des automates
						if(sending){
							int n_infsm = -1;
							switch(r_infsm[k][i].read()){
								case FSM_REQ:
									in_fifo_read[k][i] = true;				
									if(is_broadcast(data[k][i])){
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "FSM_IDLE : nouveau broadcast" << std::endl;
										#endif
										if((m_c[LOCAL])&&(i!=LOCAL))
											r_infsm[k][i] = FSM_DT_LOCAL;
										else if((m_c[NORTH])&&(i!=NORTH))
											r_infsm[k][i] = FSM_DT_NORTH;
										else if((m_c[SOUTH])&&(i!=SOUTH))
											r_infsm[k][i] = FSM_DT_SOUTH;
										else if((m_c[EAST])&&(i!=EAST)&&(i!=NORTH)&&(i!=SOUTH))	
											r_infsm[k][i] = FSM_DT_EAST;
										else if((m_c[WEST])&&(i!=WEST)&&(i!=NORTH)&&(i!=SOUTH))
											r_infsm[k][i] = FSM_DT_WEST;
									}else{
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "Passe de REQ à DTN" << std::endl;
										#endif
										r_infsm[k][i] = FSM_DTN;
									}
							                break;

								case FSM_DTN:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_DTN" << std::endl;
									#endif
									in_fifo_read[k][i] = true;
									if(is_eop(data[k][i])){
										#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
											my_print input_print "Passe de DTN à REQ" << std::endl;
										#endif
										r_infsm[k][i] = FSM_REQ;
									}
									break;

								case FSM_DT_LOCAL:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_DT_LOCAL" << std::endl;
									#endif
									n_infsm = f_infsm(NORTH, i);
									break;

								case FSM_REQ_NORTH:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_REQ_NORTH" << std::endl;
									#endif
									r_infsm[k][i] = FSM_DT_NORTH;
									break;

								case FSM_DT_NORTH:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_DT_NORTH" << std::endl;
									#endif
									n_infsm = f_infsm(SOUTH, i);
							                break;

								case FSM_REQ_SOUTH:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_REQ_SOUTH" << std::endl;
									#endif
									r_infsm[k][i] = FSM_DT_SOUTH;
									break;

					            	   	case FSM_DT_SOUTH:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_DT_SOUTH" << std::endl;
									#endif
									n_infsm = f_infsm(EAST, i);
									break;

								case FSM_REQ_EAST:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_REQ_EAST" << std::endl;
									#endif
									r_infsm[k][i] = FSM_DT_EAST;
									break;

								case FSM_DT_EAST:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_DT_EAST" << std::endl;
									#endif
									n_infsm = f_infsm(WEST, i);
               								break;

								case FSM_REQ_WEST:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_REQ_WEST" << std::endl;
									#endif
									r_infsm[k][i] = FSM_DT_WEST;
									break;

								case FSM_DT_WEST:
									#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
										my_print input_print "FSM_DT_WEST" << std::endl;
									#endif
									n_infsm = f_infsm(WEST+1, i);
       	        							break;

							}// end switch fsm

							if(n_infsm>=0){
								#if VIRTUAL_DSPIN_ROUTER_DEBUG == 1
									my_print input_print "Passe dans l'etat " << n_infsm << std::endl;
								#endif
								r_infsm[k][i] = n_infsm;
								in_fifo_read[k][i] = n_infsm == FSM_REQ;
							}

						}
						 #if VIRTUAL_DSPIN_ROUTER_DEBUG == 1 
							else if(put[k][i]) my_print input_print "get = " << get << " ; final_write = " << final_write[k][i] << std::endl; 
						#endif
					} // end loop channels
				} // end loop inputs

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
