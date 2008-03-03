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
	p_from_initiator = (soclib::caba::VciTarget<vci_param>**)malloc(sizeof(soclib::caba::VciTarget<vci_param>) * height_network);
	p_to_target = (soclib::caba::VciInitiator<vci_param>**)malloc(sizeof(soclib::caba::VciInitiator<vci_param>) * height_network);


	//
	// VCI_signals
	//
	s_initiator_wrapper = (soclib::caba::VciSignals<vci_param>**)malloc(sizeof(soclib::caba::VciSignals<vci_param>) * height_network); 
	s_target_wrapper = (soclib::caba::VciSignals<vci_param>**)malloc(sizeof(soclib::caba::VciSignals<vci_param>) * height_network);

	//
	// DSPIN_Signals
	//
	s_req_NS = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (height_network - 1));
	s_req_EW = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);
	s_req_SN = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (height_network - 1));
	s_req_WE = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);
	
	s_req_RW = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);
	s_req_WR = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);

	s_rsp_NS = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (height_network - 1));
	s_rsp_EW = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);
	s_rsp_SN = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (height_network - 1));
	s_rsp_WE = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);

	s_rsp_RW = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);
	s_rsp_WR = (soclib::caba::DspinSignals<dspin_data_size>**)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * height_network);

	//
	// Dspin_wrapper
	//
	t_initiator_wrapper = (soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>**)
	    malloc(sizeof(soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>) * height_network);
	t_target_wrapper    = (soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>**)
	    malloc(sizeof(soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>) * height_network);

	//
	// Dspin_Router
	//
	t_req_router = (soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size> **)
	    malloc(sizeof(soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>) * height_network);
	t_rsp_router = (soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size> **)
	    malloc(sizeof(soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>) * height_network);

	for( int y = 0; y < height_network ; y++ ){
	    p_from_initiator[y] = (soclib::caba::VciTarget<vci_param>*)malloc(sizeof(soclib::caba::VciTarget<vci_param>) * width_network);
	    p_to_target[y] = (soclib::caba::VciInitiator<vci_param>*)malloc(sizeof(soclib::caba::VciInitiator<vci_param>) * width_network);

	    s_to_initiator_wrapper = (soclib::caba::VciSignals<vci_param>**)malloc(sizeof(soclib::caba::VciSignals<vci_param>) * width_network); 
	    s_to_target_wrapper = (soclib::caba::VciSignals<vci_param>**)malloc(sizeof(soclib::caba::VciSignals<vci_param>) * width_network);

	    s_req_NS[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);
	    s_req_SN[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);
	    
	    s_req_EW[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (width_network - 1));
	    s_req_WE[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (width_network- 1));
	    
	    s_req_RW[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);
	    s_req_WR[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);

	    s_rsp_NS[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);
	    s_rsp_SN[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);
	    s_rsp_EW[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (width_network - 1));
	    s_rsp_WE[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * (width_network- 1));	    
	    s_rsp_RW[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);
	    s_rsp_WR[y] = (soclib::caba::DspinSignals<dspin_data_size>*)malloc(sizeof(soclib::caba::DspinSignals<dspin_data_size>) * width_network);


	    t_initiator_wrapper[y] = (soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>*)
		malloc(sizeof(soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>) * width_network);
	    t_target_wrapper[y]    = (soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>*)
		malloc(sizeof(soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>) * width_network);
	    
	    t_req_router[y] = (soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size> *)
		malloc(sizeof(soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>) * width_network);
	    t_rsp_router[y] = (soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size> *)
		malloc(sizeof(soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>) * width_network);

	    for( int x = 0; x < width_network ; x++ ){
		p_from_initiator[y][x] = soclib::caba::VciTarget<vci_param>("port_vci_initiator");
		p_to_target[y][x] = soclib::caba::VciInitiator<vci_param>("port_vci_targer");
	
		s_to_initiator_wrapper = soclib::caba::VciSignals<vci_param>("vci_signal_to_dspin_initiator_wrapper");
		s_to_target_wrapper = soclib::caba::VciSignals<vci_param>("vci_signal_to_dspin_target_wrapper");

		s_req_RW[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_router_wrapper_req");
		s_req_WR[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_wrapper_router_req");
		s_rsp_RW[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_router_wrapper_rsp");
		s_rsp_WR[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_wrapper_router_rsp");

		if( y < height_network - 1 && x < width_network - 1){
		    s_req_NS[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_N_S_req");
		    s_req_SN[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_S_N_req");
		    s_req_EW[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_E_W_req");
		    s_req_WE[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_W_E_req");

		    s_rsp_NS[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_N_S_rsp");
		    s_rsp_SN[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_S_N_rsp");
		    s_rsp_EW[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_E_W_rsp");
		    s_rsp_WE[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_W_E_rsp");
		} else if( y < height_network -1 ){
		    s_req_NS[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_N_S_req");
		    s_req_SN[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_S_N_req");
		    s_rsp_NS[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_N_S_rsp");
		    s_rsp_SN[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_S_N_rsp");
		} else if( x < width_network - 1 ){
		    s_req_EW[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_E_W_req");
		    s_req_WE[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_W_E_req");
		    s_rsp_EW[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_E_W_rsp");
		    s_rsp_WE[y][x] = soclib::caba::DspinSignals<dspin_data_size>("dspin_signal_routers_port_W_E_rsp");
		}

		t_initiator_wrapper[y][x] = soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>("i_wrapper", mt);
		t_target_wrapper[y][x] = soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>("t_wapper");
		
		t_req_router[y][x] = soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>("req_router", ((y<<4) & 0xF0) | (x & 0x0F) );
		t_rsp_router[y][x] = soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>("rsp_router", ((y<<4) & 0xF0) | (x & 0x0F) );
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
		t_initiator_wrapper[y][x].p_clk(p_clk);
		t_initiator_wrapper[y][x].p_resetn(p_resetn);

		t_target_wrapper[y][x].p_clk(p_clk);
		t_target_wrapper[y][x].p_resetn(p_resetn);

		t_req_router[y][x].p_clk(p_clk);
		t_req_router[y][x].p_resetn(p_resetn);

		t_rsp_router[y][x].p_clk(p_clk);
		t_req_router[y][x].p_resetn(p_resetn);
		
		//
		// VCI <=> Wrapper
		//
		p_from_initiator[y][x](s_to_initiator_wrapper[y][x]);
		p_to_target[y][x](s_to_target_wrapper[y][x]);

		t_initiator_wrapper[y][x].p_vci(s_to_initiator_wrapper[y][x]);
		t_target_wrapper[y][x].p_vci(s_to_target_wrapper[y][x]);

		//
		// DSPIN <=> Wrapper
		//
		t_initiator_wrapper[y][x].p_dspin_out(s_req_WR[y][x]);
		t_initiator_wrapper[y][x].p_dspin_in(s_rsp_RW[y][x]);

		t_req_router[y][x].p_out[LOCAL](s_req_RW[y][x]);
		t_req_router[y][x].p_in[LOCAL](s_req_WR[y][x]);

		t_target_wrapper[y][x].p_dspin_out(s_rsp_WR[y][x]);
		t_target_wrapper[y][x].p_dspin_in(s_req_RW[y][x]);

		t_rsp_router[y][x].p_out[LOCAL](s_rsp_RW[y][x]);
		t_rsp_router[y][x].p_in[LOCAL](s_rsp_WR[y][x]);

		//
		// DSPIN <=> DSPIN
		//

		//
		// NORTH <=> SOUTH
		//
		if( y < height_network - 1 ){
		    t_req_router[y][x].p_out[NORTH](s_req_NS[y][x]);
		    t_req_router[y][x].p_in[NORTH](s_req_SN[y][x]);
		    t_req_router[y+1][x].p_out[SOUTH](s_req_SN[y][x]);
		    t_req_router[y+1][x].p_in[SOUTH](s_req_NS[y][x]);

		    t_rsp_router[y][x].p_out[NORTH](s_rsp_NS[y][x]);
		    t_rsp_router[y][x].p_in[NORTH](s_rsp_SN[y][x]);
		    t_rsp_router[y+1][x].p_out[SOUTH](s_rsp_SN[y][x]);
		    t_rsp_router[y+1][x].p_in[SOUTH](s_rsp_NS[y][x]);
		}

		//
		// EAST <=> WEST
		//
		if( x < width_network - 1 ){
		    t_req_router[y][x].p_out[EAST](s_req_EW[y][x]);
		    t_req_router[y][x].p_in[EAST](s_req_WE[y][x]);
		    t_req_router[y][x+1].p_out[WEST](s_req_WE[y][x]);
		    t_req_router[y][x+1].p_in[WEST](s_req_EW[y][x]);

		    t_rsp_router[y][x].p_out[EAST](s_rsp_EW[y][x]);
		    t_rsp_router[y][x].p_in[EAST](s_rsp_WE[y][x]);
		    t_rsp_router[y][x+1].p_out[WEST](s_rsp_WE[y][x]);
		    t_rsp_router[y][x+1].p_in[WEST](s_rsp_EW[y][x]);
		}
	    }
	}

	Y = height_network;

	portRegister("clk", p_clk);
	portRegister("resetn", p_resetn);
    }

    tmpl(/**/)::~VciDspinNetwork()
    {
	for( int y = 0; y < Y ; y++ ){
		free( p_from_initiator[y] );
		free( p p_to_target[y] );

		free( p s_req_EW[y] );
		free( p s_req_WE[y] );
		free( p s_req_RW[y] );
		free( p s_req_WR[y] );

		free( p s_rsp_EW[y] );
		free( p s_rsp_WE[y] );
		free( p s_rsp_RW[y] );
		free( p s_rsp_WR[y] );

		if( y < Y - 1){
		    free( s_req_NS[y] );
		    free( s_req_SN[y] );
		    free( s_rsp_NS[y] );
		    free( s_rsp_SN[y] );
		}

		free( t_initiator_wrapper[y] );
		free( t_target_wrapper[y] );
		free( t_req_router[y] );
		free( t_rsp_router[y] );
	}

	free( p_from_initiator);
	free( p_to_target);

	free( s_req_NS);
	free( s_req_SN);
	free( s_req_EW);
	free( s_req_WE);
	free( s_req_RW);
	free( s_req_WR);

	free( s_rsp_NS);
	free( s_rsp_SN);
	free( s_rsp_EW);
	free( s_rsp_WE);
	free( s_rsp_RW);
	free( s_rsp_WR);

	free( t_initiator_wrapper);
	free( t_target_wrapper);
	free( t_req_router);
	free( t_rsp_router);
    }
}}

