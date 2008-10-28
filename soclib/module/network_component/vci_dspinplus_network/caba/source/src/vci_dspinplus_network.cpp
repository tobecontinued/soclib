/* -*- c++ -*-
  * File : vci_dspinplus_network.cpp
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
  */

#include <sstream>
#include <cassert>
#include "alloc_elems.h"
#include "../include/vci_dspinplus_network.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename vci_param, int dspin_fifo_size, int dspin_yx_size> x VciDspinPlusNetwork<vci_param, dspin_fifo_size, dspin_yx_size>

    ////////////////////////////////
    //      constructor
    //
    ////////////////////////////////

    tmpl(/**/)::VciDspinPlusNetwork(sc_module_name insname,
								const soclib::common::MappingTable &mt,
								size_t width_network,
								size_t height_network) : soclib::caba::BaseModule(insname)
    {
	assert( width_network <= 15 && height_network <= 15 );

	//
	// VCI_Interfaces
	//
	p_to_initiator = new soclib::caba::VciTarget<vci_param>*[height_network];
	p_to_target = new soclib::caba::VciInitiator<vci_param>*[height_network];

	//
	// DSPIN_Signals
	//
	s_req_NS = new soclib::caba::DspinSignals<37>*[height_network + 2];
	s_req_EW = new soclib::caba::DspinSignals<37>*[height_network + 2];
	s_req_SN = new soclib::caba::DspinSignals<37>*[height_network + 2];
	s_req_WE = new soclib::caba::DspinSignals<37>*[height_network + 2];
	
	s_req_RW = new soclib::caba::DspinSignals<37>*[height_network];
	s_req_WR = new soclib::caba::DspinSignals<37>*[height_network];

	s_rsp_NS = new soclib::caba::DspinSignals<33>*[height_network + 2];
	s_rsp_EW = new soclib::caba::DspinSignals<33>*[height_network + 2];
	s_rsp_SN = new soclib::caba::DspinSignals<33>*[height_network + 2];
	s_rsp_WE = new soclib::caba::DspinSignals<33>*[height_network + 2];

	s_rsp_RW = new soclib::caba::DspinSignals<33>*[height_network];
	s_rsp_WR = new soclib::caba::DspinSignals<33>*[height_network];

	//
	// Dspin_wrapper
	//
	t_initiator_wrapper = new soclib::caba::VciDspinPlusInitiatorWrapper<vci_param, dspin_fifo_size, dspin_yx_size>**[height_network];
	t_target_wrapper    = new soclib::caba::VciDspinPlusTargetWrapper<vci_param, dspin_fifo_size, dspin_yx_size>**[height_network];

	//
	// Dspin_Router
	//
	t_req_router = new soclib::caba::DspinPlusRouter<37, dspin_fifo_size, dspin_yx_size>**[height_network];
	t_rsp_router = new soclib::caba::DspinPlusRouter<33, dspin_fifo_size, dspin_yx_size>**[height_network];



	for( size_t y = 0; y < height_network + 2 ; y++ ){

	    if( y < height_network ){
		p_to_initiator[y] = soclib::common::alloc_elems<soclib::caba::VciTarget<vci_param> >( "p_to_initiator", width_network);
		p_to_target[y] = soclib::common::alloc_elems<soclib::caba::VciInitiator<vci_param> >("p_to_target", width_network);
		t_initiator_wrapper[y] = new soclib::caba::VciDspinPlusInitiatorWrapper<vci_param, dspin_fifo_size, dspin_yx_size>*[width_network];
		t_target_wrapper[y]    = new soclib::caba::VciDspinPlusTargetWrapper<vci_param, dspin_fifo_size, dspin_yx_size>*[width_network];

		t_req_router[y] = new soclib::caba::DspinPlusRouter<37, dspin_fifo_size, dspin_yx_size>*[width_network];
		t_rsp_router[y] = new soclib::caba::DspinPlusRouter<33, dspin_fifo_size, dspin_yx_size>*[width_network];

		s_req_RW[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<37> >("s_req_RW", width_network);
		s_req_WR[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<37> >("s_req_WR", width_network);
		s_rsp_RW[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<33> >("s_rsp_RW", width_network);
		s_rsp_WR[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<33> >("s_rsp_WR", width_network);

		for( size_t x = 0; x < width_network ; x++ ){
		    t_initiator_wrapper[y][x] = new soclib::caba::VciDspinPlusInitiatorWrapper<vci_param, dspin_fifo_size, dspin_yx_size>("t_initiator_wrapper", mt);
		    t_target_wrapper[y][x] = new soclib::caba::VciDspinPlusTargetWrapper<vci_param, dspin_fifo_size, dspin_yx_size>("t_target_wrapper", mt);

		    t_req_router[y][x] = new soclib::caba::DspinPlusRouter<37, dspin_fifo_size, dspin_yx_size>("t_req_router", ((y<<4) & 0xF0) | (x & 0x0F) );
		    t_rsp_router[y][x] = new soclib::caba::DspinPlusRouter<33, dspin_fifo_size, dspin_yx_size>("t_rsp_router", ((y<<4) & 0xF0) | (x & 0x0F) );
		}
	    }	    

	    s_req_NS[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<37> >("s_req_NS", width_network + 2);
	    s_req_SN[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<37> >("s_req_SN", width_network + 2);	    
	    s_req_EW[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<37> >("s_req_EW", width_network + 2);
	    s_req_WE[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<37> >("s_req_WE", width_network + 2);	    

	    s_rsp_NS[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<33> >("s_rsp_NS", width_network + 2);
	    s_rsp_SN[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<33> >("s_rsp_SN", width_network + 2);
	    s_rsp_EW[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<33> >("s_rsp_EW", width_network + 2);
	    s_rsp_WE[y] = soclib::common::alloc_elems<soclib::caba::DspinSignals<33> >("s_rsp_WE", width_network + 2);
	}

	//
	// NETLIST
	//
	for( size_t y = 0 ; y < height_network ; y++ ){
	    for( size_t x = 0 ; x < width_network ; x++ ){
		//
		// CLK RESETN
		//
		t_initiator_wrapper[y][x]->p_clk(p_clk);
		t_initiator_wrapper[y][x]->p_resetn(p_resetn);

		t_target_wrapper[y][x]->p_clk(p_clk);
		t_target_wrapper[y][x]->p_resetn(p_resetn);

		t_rsp_router[y][x]->p_clk(p_clk);
		t_rsp_router[y][x]->p_resetn(p_resetn);

		t_req_router[y][x]->p_clk(p_clk);
		t_req_router[y][x]->p_resetn(p_resetn);

		//
		// VCI <=> Wrapper
		//
		t_initiator_wrapper[y][x]->p_vci(p_to_initiator[y][x]);
		t_target_wrapper[y][x]->p_vci(p_to_target[y][x]);

		//
		// DSPIN <=> Wrapper
		//		

		t_initiator_wrapper[y][x]->p_dspin_out(s_req_WR[y][x]);
		t_req_router[y][x]->p_in[LOCAL](s_req_WR[y][x]);

		t_req_router[y][x]->p_out[LOCAL](s_req_RW[y][x]);
		t_target_wrapper[y][x]->p_dspin_in(s_req_RW[y][x]);

		t_target_wrapper[y][x]->p_dspin_out(s_rsp_WR[y][x]);
		t_rsp_router[y][x]->p_in[LOCAL](s_rsp_WR[y][x]);

		t_rsp_router[y][x]->p_out[LOCAL](s_rsp_RW[y][x]);
		t_initiator_wrapper[y][x]->p_dspin_in(s_rsp_RW[y][x]);


		//
		// DSPIN <=> DSPIN
		//

		//
		// NORTH <=> SOUTH
		//		
		//
		t_req_router[y][x]->p_in[NORTH](s_req_SN[y+2][x+1]);
		t_req_router[y][x]->p_in[SOUTH](s_req_NS[y][x+1]);
		t_req_router[y][x]->p_in[EAST](s_req_WE[y+1][x+2]);
		t_req_router[y][x]->p_in[WEST](s_req_EW[y+1][x]);

		t_req_router[y][x]->p_out[NORTH](s_req_NS[y+1][x+1]);
		t_req_router[y][x]->p_out[SOUTH](s_req_SN[y+1][x+1]);
		t_req_router[y][x]->p_out[EAST](s_req_EW[y+1][x+1]);
		t_req_router[y][x]->p_out[WEST](s_req_WE[y+1][x+1]);

		t_rsp_router[y][x]->p_in[NORTH] (s_rsp_SN[y+2][x+1]);
		t_rsp_router[y][x]->p_in[SOUTH] (s_rsp_NS[y][x+1]);
		t_rsp_router[y][x]->p_in[EAST]  (s_rsp_WE[y+1][x+2]);
		t_rsp_router[y][x]->p_in[WEST]  (s_rsp_EW[y+1][x]);

		t_rsp_router[y][x]->p_out[NORTH](s_rsp_NS[y+1][x+1]);
		t_rsp_router[y][x]->p_out[SOUTH](s_rsp_SN[y+1][x+1]);
		t_rsp_router[y][x]->p_out[EAST] (s_rsp_EW[y+1][x+1]);
		t_rsp_router[y][x]->p_out[WEST] (s_rsp_WE[y+1][x+1]);

	    }
	}

	Y = height_network;
	X = width_network;
    }

    tmpl(/**/)::~VciDspinPlusNetwork()
    {

	for( size_t y = 0; y < Y + 2 ; y++ ){
	    if( y < Y ){

		soclib::common::dealloc_elems( p_to_initiator[y], X);
		soclib::common::dealloc_elems( p_to_target[y], X);

		soclib::common::dealloc_elems( s_req_RW[y], X);
		soclib::common::dealloc_elems( s_req_WR[y], X);

		soclib::common::dealloc_elems( s_rsp_RW[y], X);
		soclib::common::dealloc_elems( s_rsp_WR[y], X);

		for( size_t x = 0; x < X ; x ++ ){
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
	    soclib::common::dealloc_elems( s_req_NS[y], X + 2);
	    soclib::common::dealloc_elems( s_req_SN[y], X + 2);
	    soclib::common::dealloc_elems( s_req_EW[y], X + 2);
	    soclib::common::dealloc_elems( s_req_WE[y], X + 2);

	    soclib::common::dealloc_elems( s_rsp_NS[y], X + 2);
	    soclib::common::dealloc_elems( s_rsp_SN[y], X + 2);
	    soclib::common::dealloc_elems( s_rsp_EW[y], X + 2);
	    soclib::common::dealloc_elems( s_rsp_WE[y], X + 2);
	}

	delete [] p_to_initiator;
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

