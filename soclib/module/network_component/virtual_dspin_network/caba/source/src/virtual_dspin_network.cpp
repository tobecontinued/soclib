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

#include "virtual_dspin_network.h"
#include <cstdlib>
#include <cassert>
#include <sstream>
#include "alloc_elems.h"
#include <new>

namespace soclib 
{ 
	namespace caba 
	{
		#define tmpl(x) template<int io_mask_size, int io_number_size, int x_adressing_size, int y_adressing_size, int cmd_data_size, int cmd_io_mask_offset,  int cmd_io_number_offset, int cmd_x_adressing_offset, int cmd_y_adressing_offset,int cmd_eop_offset, int cmd_broadcast_offset, int rsp_data_size, int rsp_io_mask_offset, int rsp_io_number_offset, int rsp_x_adressing_offset, int rsp_y_adressing_offset, int rsp_eop_offset, int in_fifo_size, int out_fifo_size> x VirtualDspinNetwork<io_mask_size, io_number_size, x_adressing_size, y_adressing_size, cmd_data_size, cmd_io_mask_offset,  cmd_io_number_offset, cmd_x_adressing_offset, cmd_y_adressing_offset,cmd_eop_offset, cmd_broadcast_offset, rsp_data_size, rsp_io_mask_offset, rsp_io_number_offset, rsp_x_adressing_offset, rsp_y_adressing_offset, rsp_eop_offset, in_fifo_size, out_fifo_size>
		
		////////////////////////////////
		//      constructor
		////////////////////////////////
		tmpl(/**/)::VirtualDspinNetwork(sc_module_name insname, int size_x, int size_y, bool broadcast0, bool broadcast1, bool io0, bool io1, clusterCoordinates<x_adressing_size, y_adressing_size> * aIO_table)	: soclib::caba::BaseModule(insname), p_clk("clk"), p_resetn("resetn")
		{

			assert((size_x > 1)&&(size_y > 1) && "Virtual dspin network needs at least 2 columns and 2 rows");
	
			m_size_x = size_x;
			m_size_y = size_y;

			std::ostringstream o;

			o.str("");
			o << insname << "_vWiresCmd" ;
			vWiresCmd = soclib::common::alloc_elems<soclib::caba::DspinSignals<cmd_data_size> >(o.str().c_str(),  4, size_x, size_y-1);
			o.str("");
			o << insname << "_hWiresCmd" ;
			hWiresCmd = soclib::common::alloc_elems<soclib::caba::DspinSignals<cmd_data_size> >(o.str().c_str(),  4, size_x-1, size_y);
			o.str("");
			o << insname << "_vWiresRsp" ;
			vWiresRsp = soclib::common::alloc_elems<soclib::caba::DspinSignals<rsp_data_size> >(o.str().c_str(),  4, size_x, size_y-1);
			o.str("");
			o << insname << "_hWiresRsp" ;
			hWiresRsp = soclib::common::alloc_elems<soclib::caba::DspinSignals<rsp_data_size> >(o.str().c_str(),  4, size_x-1, size_y);

			cmdVirtualDspinRouters = (soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
					cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size> **) 
				malloc(sizeof(soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size> *)*size_x);

			rspVirtualDspinRouters = (soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
					rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size> **) 
				malloc(sizeof(soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size,
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size> *)*size_x);

			for(int i=0; i<size_x; i++){
				cmdVirtualDspinRouters[i] = (soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size> *)
					malloc(sizeof(soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
							cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size> )*size_y);
			}			

			for(int i=0; i<size_x; i++){
				rspVirtualDspinRouters[i] = (soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size,
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size> *)
					malloc(sizeof(soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
							rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size> )*size_y);
			}

			// The four corners cmd
			o.str("");
			o << insname << "_cmdVirtualDspinRouter[" << 0 		<< "][" << 0 		<< "]";
			new(&cmdVirtualDspinRouters[0][0]) 
				soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
					o.str().c_str(), 0		, 0		, true, false, true, false, broadcast0, broadcast1, io0, io1, aIO_table);
			o.str("");
			o << insname << "_cmdVirtualDspinRouter[" << 0 		<< "][" << (size_y-1) 	<< "]";
			new(&cmdVirtualDspinRouters[0][size_y-1]) 
				soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
					o.str().c_str(), 0		, (size_y-1)	, false, true, true, false, broadcast0, broadcast1, io0, io1, aIO_table);
			o.str("");
			o << insname << "_cmdVirtualDspinRouter[" << size_x-1 	<< "][" << 0 		<< "]";
			new(&cmdVirtualDspinRouters[size_x-1][0]) 
				soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
					o.str().c_str(), (size_x-1)	, 0		, true, false, false, true, broadcast0, broadcast1, io0, io1, aIO_table);
			o.str("");
			o << insname << "_cmdVirtualDspinRouter[" << size_x-1 	<< "][" << size_y-1 	<< "]";
			new(&cmdVirtualDspinRouters[size_x-1][size_y-1]) 
				soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
					o.str().c_str(), (size_x-1)	, (size_y-1)	, false, true, false, true, broadcast0, broadcast1, io0, io1, aIO_table);

			// The four corners rsp
			o.str("");
			o << insname << "_rspVirtualDspinRouter[" << 0 		<< "][" << 0 		<< "]";
			new(&rspVirtualDspinRouters[0][0]) 
				soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
					o.str().c_str(), 0		, 0		, true, false, true, false, false, false, io0, io1, aIO_table);
			o.str("");
			o << insname << "_rspVirtualDspinRouter[" << 0 		<< "][" << (size_y-1) 	<< "]";
			new(&rspVirtualDspinRouters[0][size_y-1]) 
				soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size,
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
					o.str().c_str(), 0		, (size_y-1)	, false, true, true, false, false, false, io0, io1, aIO_table);
			o.str("");
			o << insname << "_rspVirtualDspinRouter[" << size_x-1 	<< "][" << 0 		<< "]";
			new(&rspVirtualDspinRouters[size_x-1][0]) 
				soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
					o.str().c_str(), (size_x-1)	, 0		, true, false, false, true, false, false, io0, io1, aIO_table);
			o.str("");
			o << insname << "_rspVirtualDspinRouter[" << size_x-1 	<< "][" << size_y-1 	<< "]";
			new(&rspVirtualDspinRouters[size_x-1][size_y-1]) 
				soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
					o.str().c_str(), (size_x-1)	, (size_y-1)	, false, true, false, true, false, false, io0, io1, aIO_table);

			// South rown cmd
			for(int i=1; i<size_x-1; i++){
				o.str("");
				o << insname << "_cmdVirtualDspinRouter[" << i 		<< "][" << 0 		<< "]";
				new(&cmdVirtualDspinRouters[i][0]) 
					soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
							cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
						o.str().c_str(), i	, 0		, true, false, true, true, broadcast0, broadcast1, io0, io1, aIO_table);
			}

			// South rown rsp
			for(int i=1; i<size_x-1; i++){
				o.str("");
				o << insname << "_rspVirtualDspinRouter[" << i 		<< "][" << 0 		<< "]";
				new(&rspVirtualDspinRouters[i][0]) 
					soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
							rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
						o.str().c_str(), i	, 0		, true, false, true, true, false, false, io0, io1, aIO_table);
			}

			// North row cmd
			for(int i=1; i<size_x-1; i++){
				o.str("");
				o << insname << "_cmdVirtualDspinRouter[" << i 		<< "][" << size_y-1 	<< "]";
				new(&cmdVirtualDspinRouters[i][size_y-1]) 
					soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
							cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
						o.str().c_str(), i	, size_y-1	, false, true, true, true, broadcast0, broadcast1, io0, io1, aIO_table);
			}

			// North row rsp
			for(int i=1; i<size_x-1; i++){
				o.str("");
				o << insname << "_rspVirtualDspinRouter[" << i 		<< "][" << size_y-1 	<< "]";
				new(&rspVirtualDspinRouters[i][size_y-1]) 
					soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
							rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
						o.str().c_str(), i	, size_y-1	, false, true, true, true, false, false, io0, io1, aIO_table);
			}
			
			// West column cmd
			for(int j=1; j<size_y-1; j++){
				o.str("");
				o << insname <<  "_cmdVirtualDspinRouter[" << 0 	<< "][" << j 		<< "]";
				new(&cmdVirtualDspinRouters[0][j]) 
					soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
							cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
						o.str().c_str(), 0	, j		, true, true, true, false, broadcast0, broadcast1, io0, io1, aIO_table);
			}

			// West column rsp
			for(int j=1; j<size_y-1; j++){
				o.str("");
				o << insname << "_rspVirtualDspinRouter[" << 0 		<< "][" << j 		<< "]";
				new(&rspVirtualDspinRouters[0][j]) 
					soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
							rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
						o.str().c_str(), 0	, j		, true, true, true, false, false, false, io0, io1, aIO_table);
			}

			// East column cmd
			for(int j=1; j<size_y-1; j++){
				o.str("");
				o << insname << "_cmdVirtualDspinRouter[" << size_x-1 	<< "][" << j 		<< "]";
				new(&cmdVirtualDspinRouters[size_x-1][j]) 
					soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
							cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
						o.str().c_str(), size_x-1, j		, true, true, false, true, broadcast0, broadcast1, io0, io1, aIO_table);
			}

			// East column rsp
			for(int j=1; j<size_y-1; j++){
				o.str("");
				o << insname << "_rspVirtualDspinRouter[" << size_x-1 	<< "][" << j 		<< "]";
				new(&rspVirtualDspinRouters[size_x-1][j]) 
					soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
							rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
						o.str().c_str(), size_x-1, j		, true, true, false, true, false, false, io0, io1, aIO_table);
			}


			// The middle cells cmd
			for(int i=1; i<size_x-1; i++)
				for(int j=1; j<size_y-1; j++){
					o.str("");
					o << insname << "_cmdVirtualDspinRouter[" << i << "][" << j << "]";
					new(&cmdVirtualDspinRouters[i][j]) 
						soclib::caba::VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size,
								cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size>(
							o.str().c_str(), i, j, true, true, true, true, broadcast0, broadcast1, io0, io1, aIO_table);
			}

			// The middle cells rsp
			for(int i=1; i<size_x-1; i++)
				for(int j=1; j<size_y-1; j++){
					o.str("");
					o << insname << "_rspVirtualDspinRouters[" << i << "][" << j << "]";
					new(&rspVirtualDspinRouters[i][j]) 
						soclib::caba::VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
								rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size>(
							o.str().c_str(), i, j, true, true, true, true, false, false, io0, io1, aIO_table);
			}


			for(int i=0; i<size_x; i++){	// Connecting regular signals
				for(int j=0; j<size_y; j++){
					cmdVirtualDspinRouters[i][j].p_clk(p_clk);
					cmdVirtualDspinRouters[i][j].p_resetn(p_resetn);
					rspVirtualDspinRouters[i][j].p_clk(p_clk);
					rspVirtualDspinRouters[i][j].p_resetn(p_resetn);
				}
			}				


			for(int l=0; l<2; l++){		// Vertical connexions cmd
				for(int i=0; i<size_x; i++){
					for(int j=0; j<size_y-1; j++){
						cmdVirtualDspinRouters[i][j  ].p_out[l][1](vWiresCmd[l  ][i][j]);
						cmdVirtualDspinRouters[i][j+1].p_in[l][2](vWiresCmd[l  ][i][j]);
						cmdVirtualDspinRouters[i][j+1].p_out[l][2](vWiresCmd[l+2][i][j]);
						cmdVirtualDspinRouters[i][j  ].p_in[l][1](vWiresCmd[l+2][i][j]);
					}
				}
			}

			for(int l=0; l<2; l++){		// Horizontal connexions cmd
				for(int i=0; i<size_x-1; i++){
					for(int j=0; j<size_y; j++){
						cmdVirtualDspinRouters[i  ][j].p_out[l][3](hWiresCmd[l][i][j]);
						cmdVirtualDspinRouters[i+1][j].p_in[l][4](hWiresCmd[l][i][j]);
						cmdVirtualDspinRouters[i+1][j].p_out[l][4](hWiresCmd[l+2][i][j]);
						cmdVirtualDspinRouters[i  ][j].p_in[l][3](hWiresCmd[l+2][i][j]);
					}
				}
			}

			for(int l=0; l<2; l++){		// Vertical connexions rsp
				for(int i=0; i<size_x; i++){
					for(int j=0; j<size_y-1; j++){
						rspVirtualDspinRouters[i][j  ].p_out[l][1](vWiresRsp[l  ][i][j]);
						rspVirtualDspinRouters[i][j+1].p_in[l][2](vWiresRsp[l  ][i][j]);
						rspVirtualDspinRouters[i][j+1].p_out[l][2](vWiresRsp[l+2][i][j]);
						rspVirtualDspinRouters[i][j  ].p_in[l][1](vWiresRsp[l+2][i][j]);
					}
				}
			}

			for(int l=0; l<2; l++){		// Horizontal connexions rsp
				for(int i=0; i<size_x-1; i++){
					for(int j=0; j<size_y; j++){
						rspVirtualDspinRouters[i  ][j].p_out[l][3](hWiresRsp[l][i][j]);
						rspVirtualDspinRouters[i+1][j].p_in[l][4](hWiresRsp[l][i][j]);
						rspVirtualDspinRouters[i+1][j].p_out[l][4](hWiresRsp[l+2][i][j]);
						rspVirtualDspinRouters[i  ][j].p_in[l][3](hWiresRsp[l+2][i][j]);
					}
				}
			}

			// Connecting ports to Virtual Dspin Routers
			o.str("");
			o << insname << "_ports";
			ports = soclib::common::alloc_elems<soclib::caba::VirtualDspinNetworkPort<cmd_data_size, rsp_data_size> >(o.str().c_str(),  2, size_x, size_y);
			for(int i=0; i<size_x; i++){
				for(int j=0; j<size_y; j++){
					for(int k=0; k<2; k++){
						cmdVirtualDspinRouters[i][j].p_in[k][0].data(ports[k][i][j].in_cmd_data);
						cmdVirtualDspinRouters[i][j].p_in[k][0].read(ports[k][i][j].in_cmd_read);
						cmdVirtualDspinRouters[i][j].p_in[k][0].write(ports[k][i][j].in_cmd_write);

						rspVirtualDspinRouters[i][j].p_out[k][0].data(ports[k][i][j].out_rsp_data);
						rspVirtualDspinRouters[i][j].p_out[k][0].read(ports[k][i][j].out_rsp_read);
						rspVirtualDspinRouters[i][j].p_out[k][0].write(ports[k][i][j].out_rsp_write);

						cmdVirtualDspinRouters[i][j].p_out[k][0].data(ports[k][i][j].out_cmd_data);
						cmdVirtualDspinRouters[i][j].p_out[k][0].read(ports[k][i][j].out_cmd_read);
						cmdVirtualDspinRouters[i][j].p_out[k][0].write(ports[k][i][j].out_cmd_write);

						rspVirtualDspinRouters[i][j].p_in[k][0].data(ports[k][i][j].in_rsp_data);
						rspVirtualDspinRouters[i][j].p_in[k][0].read(ports[k][i][j].in_rsp_read);
						rspVirtualDspinRouters[i][j].p_in[k][0].write(ports[k][i][j].in_rsp_write);
					}
				}
			}
		} //  end constructor

		tmpl(/**/)::~VirtualDspinNetwork()
		{
			soclib::common::dealloc_elems(ports,		2, 	m_size_x, 	m_size_y	);
			soclib::common::dealloc_elems(vWiresCmd,	4, 	m_size_x, 	m_size_y-1	);
			soclib::common::dealloc_elems(hWiresCmd,	4, 	m_size_x-1, 	m_size_y	);
			soclib::common::dealloc_elems(vWiresRsp,	4, 	m_size_x, 	m_size_y-1	);
			soclib::common::dealloc_elems(hWiresRsp,	4, 	m_size_x-1, 	m_size_y	);
			soclib::common::dealloc_elems(cmdVirtualDspinRouters, 	m_size_x, 	m_size_y	);
			soclib::common::dealloc_elems(rspVirtualDspinRouters, 	m_size_x, 	m_size_y	);
		} // end destructor
	} // end namespace caba
} // end namespace soclib
