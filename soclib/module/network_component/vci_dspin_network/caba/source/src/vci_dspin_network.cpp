/* -*- c++ -*-
  * File : vci_dspin_network.cpp
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner, Abbas Sheibanyrad, Ivan Miro, Zhen Zhang
  *
  * SOCLIB_LGPL_HEADER_BEGIN
  * SoCLib is free software; you can redistribute it and/or modify it
  * under the terms of the GNU Lesser General Public License as published
  * by the Free Software Foundation; version 2.1 of the License.
  * SoCLib is distributed in the hope that it will be useful, but
  * WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * You should have received a copy of the GNU Lesser General Public
  * License along with SoCLib; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  * 02110-1301 USA
  * SOCLIB_LGPL_HEADER_END
  */

#include <cassert>
#include "alloc_elems.h"
#include "../include/vci_dspin_network.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename vci_param, int dspin_data_size, int dspin_fifo_size, int dspin_srcid_msb_size> x VciDspinNetwork<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>

    ////////////////////////////////
    //      constructor
    //
    ////////////////////////////////

    tmpl(/**/)::VciDspinNetwork(sc_module_name insname,
	    const soclib::common::MappingTable &mt,
	    int width_network,
	    int height_network) : soclib::caba::BaseModule(insname)
    {
	//
	// 0 <= width_network <= 15
	// 0 <= height_network <= 15
	//
	assert( (width_network & 0xFFFFFF00) == 0x0 && (height_network & 0xFFFFFF00) == 0x0 );

	//
	// VCI_Interfaces
	//
	p_from_initiator = new soclib::caba::VciTarget<vci_param>*[height_network];
	p_to_target = new soclib::caba::VciInitiator<vci_param>*[height_network];


	//
	// VCI_signals
	//
	s_initiator_wrapper = new soclib::caba::VciSignals<vci_param>*[height_network]; 
	s_target_wrapper = new soclib::caba::VciSignals<vci_param>*[height_network];

	//
	// DSPIN_Signals
	//
	s_req_NS = new soclib::caba::DspinSignals<dspin_data_size>*[height_network - 1];
	s_req_EW = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];
	s_req_SN = new soclib::caba::DspinSignals<dspin_data_size>*[height_network - 1];
	s_req_WE = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];
	
	s_req_RW = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];
	s_req_WR = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];

	s_rsp_NS = new soclib::caba::DspinSignals<dspin_data_size>*[height_network - 1];
	s_rsp_EW = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];
	s_rsp_SN = new soclib::caba::DspinSignals<dspin_data_size>*[height_network - 1];
	s_rsp_WE = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];

	s_rsp_RW = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];
	s_rsp_WR = new soclib::caba::DspinSignals<dspin_data_size>*[height_network];

	//
	// Dspin_wrapper
	//
	t_initiator_wrapper = new soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>**[height_network];
	t_target_wrapper    = new soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>**[height_network];

	//
	// Dspin_Router
	//
	t_req_router = new soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>**[height_network];
	t_rsp_router = new soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>**[height_network];

	for( int y = 0; y < height_network ; y++ ){
	    p_from_initiator[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >( "p_from_initiator", width_network);
	    p_to_target[y] = alloc_elems<soclib::caba::VciInitiator<vci_param> >("p_to_target", width_network);

	    s_to_initiator_wrapper[y] = alloc_elems<soclib::caba::VciSignals<vci_param> >("s_to_initiator_wrapper", width_network); 
	    s_to_target_wrapper[y] = alloc_elems<soclib::caba::VciSignals<vci_param> >("s_to_target_wrapper", width_network);

	    s_req_NS[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_req_NS", width_network);
	    s_req_SN[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_req_SN", width_network);	    
	    s_req_EW[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_req_EW", width_network - 1);
	    s_req_WE[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_req_WE", width_network- 1);	    
	    s_req_RW[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_req_RW", width_network);
	    s_req_WR[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_req_WR", width_network);

	    s_rsp_NS[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_rsp_NS", width_network);
	    s_rsp_SN[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_rsp_SN", width_network);
	    s_rsp_EW[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_rsp_EW", width_network - 1);
	    s_rsp_WE[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_rsp_WE", width_network- 1);	    
	    s_rsp_RW[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_rsp_RW", width_network);
	    s_rsp_WR[y] = alloc_elems<soclib::caba::DspinSignals<dspin_data_size> >("s_rsp_WR", width_network);


	    t_initiator_wrapper[y] = new soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>*[width_network];
	    t_target_wrapper[y]    = new soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>*[width_network];
	    
	    t_req_router[y] = new soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>*[width_network];
	    t_rsp_router[y] = new soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>*[width_network];

	    for( int x = 0; x < width_network ; x++ ){
		t_initiator_wrapper[y][x] = new soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>("i_wrapper", mt);
		t_target_wrapper[y][x] = new soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>("t_wapper");
		
		t_req_router[y][x] = new soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>("req_router", ((y<<4) & 0xF0) | (x & 0x0F) );
		t_rsp_router[y][x] = new soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>("rsp_router", ((y<<4) & 0xF0) | (x & 0x0F) );
	    }
	}

	//
	// NETLIST
	//
	for( int y = 0 ; y < height_network ; y++ ){
	    for( int x = 0 ; x < width_network ; x++ ){
		//
		// CLK RESETN
		//
		t_initiator_wrapper[y][x]->p_clk(p_clk);
		t_initiator_wrapper[y][x]->p_resetn(p_resetn);

		t_target_wrapper[y][x]->p_clk(p_clk);
		t_target_wrapper[y][x]->p_resetn(p_resetn);

		t_req_router[y][x]->p_clk(p_clk);
		t_req_router[y][x]->p_resetn(p_resetn);

		t_rsp_router[y][x]->p_clk(p_clk);
		t_req_router[y][x]->p_resetn(p_resetn);
		
		//
		// VCI <=> Wrapper
		//
		p_from_initiator[y][x](s_to_initiator_wrapper[y][x]);
		p_to_target[y][x](s_to_target_wrapper[y][x]);

		t_initiator_wrapper[y][x]->p_vci(s_to_initiator_wrapper[y][x]);
		t_target_wrapper[y][x]->p_vci(s_to_target_wrapper[y][x]);

		//
		// DSPIN <=> Wrapper
		//
		t_initiator_wrapper[y][x]->p_dspin_out(s_req_WR[y][x]);
		t_initiator_wrapper[y][x]->p_dspin_in(s_rsp_RW[y][x]);

		t_req_router[y][x]->p_out[LOCAL](s_req_RW[y][x]);
		t_req_router[y][x]->p_in[LOCAL](s_req_WR[y][x]);

		t_target_wrapper[y][x]->p_dspin_out(s_rsp_WR[y][x]);
		t_target_wrapper[y][x]->p_dspin_in(s_req_RW[y][x]);

		t_rsp_router[y][x]->p_out[LOCAL](s_rsp_RW[y][x]);
		t_rsp_router[y][x]->p_in[LOCAL](s_rsp_WR[y][x]);

		//
		// DSPIN <=> DSPIN
		//

		//
		// NORTH <=> SOUTH
		//
		if( y < height_network - 1 ){
		    t_req_router[y][x]->p_out[NORTH](s_req_NS[y][x]);
		    t_req_router[y][x]->p_in[NORTH](s_req_SN[y][x]);
		    t_req_router[y+1][x]->p_out[SOUTH](s_req_SN[y][x]);
		    t_req_router[y+1][x]->p_in[SOUTH](s_req_NS[y][x]);

		    t_rsp_router[y][x]->p_out[NORTH](s_rsp_NS[y][x]);
		    t_rsp_router[y][x]->p_in[NORTH](s_rsp_SN[y][x]);
		    t_rsp_router[y+1][x]->p_out[SOUTH](s_rsp_SN[y][x]);
		    t_rsp_router[y+1][x]->p_in[SOUTH](s_rsp_NS[y][x]);
		}

		//
		// EAST <=> WEST
		//
		if( x < width_network - 1 ){
		    t_req_router[y][x]->p_out[EAST](s_req_EW[y][x]);
		    t_req_router[y][x]->p_in[EAST](s_req_WE[y][x]);
		    t_req_router[y][x+1]->p_out[WEST](s_req_WE[y][x]);
		    t_req_router[y][x+1]->p_in[WEST](s_req_EW[y][x]);

		    t_rsp_router[y][x]->p_out[EAST](s_rsp_EW[y][x]);
		    t_rsp_router[y][x]->p_in[EAST](s_rsp_WE[y][x]);
		    t_rsp_router[y][x+1]->p_out[WEST](s_rsp_WE[y][x]);
		    t_rsp_router[y][x+1]->p_in[WEST](s_rsp_EW[y][x]);
		}
	    }
	}

	Y = height_network;
	X = width_network;

	portRegister("clk", p_clk);
	portRegister("resetn", p_resetn);
    }

    tmpl(/**/)::~VciDspinNetwork()
    {

	for( int y = 0; y < Y ; y++ ){

	    dealloc_elems( p_from_initiator[y], X);
	    dealloc_elems( p_to_target[y], X);

	    dealloc_elems( s_req_NS[y], X);
	    dealloc_elems( s_req_SN[y], X);
	    dealloc_elems( s_req_EW[y], X - 1);
	    dealloc_elems( s_req_WE[y], X - 1);
	    dealloc_elems( s_req_RW[y], X);
	    dealloc_elems( s_req_WR[y], X);

	    dealloc_elems( s_rsp_NS[y], X);
	    dealloc_elems( s_rsp_SN[y], X);
	    dealloc_elems( s_rsp_EW[y], X - 1);
	    dealloc_elems( s_rsp_WE[y], X - 1);
	    dealloc_elems( s_rsp_RW[y], X);
	    dealloc_elems( s_rsp_WR[y], X);

	    for( int x = 0; x < X ; x ++ ){
		delete t_initiator_wrapper[y][x];
		delete t_target_wrapper[y][x];
		delete t_req_router[y][x];
		delete t_rsp_router[y][x];
	    }

	    delete [] t_initiator_wrapper[y];
	    delete [] t_target_wrapper[y];
	    delete [] t_req_router[y];
	    delete [] t_rsp_router[y];
	}

	delete [] p_from_initiator;
	delete [] p_to_target;

	delete [] s_req_NS;
	delete [] s_req_SN;
	delete [] s_req_EW;
	delete [] s_req_WE;
	delete [] s_req_RW;
	delete [] s_req_WR;

	delete [] s_rsp_NS;
	delete [] s_rsp_SN;
	delete [] s_rsp_EW;
	delete [] s_rsp_WE;
	delete [] s_rsp_RW;
	delete [] s_rsp_WR;

	delete [] t_initiator_wrapper;
	delete [] t_target_wrapper;
	delete [] t_req_router;
	delete [] t_rsp_router;
    }
}}

