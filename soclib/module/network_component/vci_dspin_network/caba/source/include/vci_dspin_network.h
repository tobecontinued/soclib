/* -*- c++ -*-
  * File : vci_dspin_network.h
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

#ifndef VCI_DSPIN_NETWORK_H_
#define VCI_DSPIN_NETWORK_H_

#include <systemc>
#include "caba_base_module.h"
#include "vci_target.h"
#include "vci_initiator.h"
#include "mapping_table.h"

#include "dspin_router.h"
#include "vci_dspin_target_wrapper.h"
#include "vci_dspin_initiator_wrapper.h"
#include "dspin_interface.h"

namespace soclib { namespace caba {

    using namespace sc_core;

    template<typename vci_param, int dspin_data_size, int dspin_fifo_size, int dspin_srcid_msb_size>
	class VciDspinNetwork
	: public soclib::caba::BaseModule
	{
	    public:
		sc_in<bool>		p_clk;
		sc_in<bool>		p_resetn;

		soclib::caba::VciInitiator<vci_param>** p_to_target;
		soclib::caba::VciTarget<vci_param>** p_to_initiator;

	    protected:
	    SC_HAS_PROCESS(VciDspinNetwork);

	    private:
	    	int Y;
		int X;

	    	// signal vci
		soclib::caba::VciSignals<vci_param>** s_to_initiator_wrapper;
		soclib::caba::VciSignals<vci_param>** s_to_target_wrapper;

		
	    	// signal dspin
		//
		// signal between routers
		//
	    	soclib::caba::DspinSignals<dspin_data_size>** s_req_NS;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_req_EW;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_req_SN;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_req_WE;

		//
		// signal between router and wrapper
		//
	    	soclib::caba::DspinSignals<dspin_data_size>** s_req_RW;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_req_WR;
		
	    	soclib::caba::DspinSignals<dspin_data_size>** s_rsp_NS;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_rsp_EW;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_rsp_SN;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_rsp_WE;

	    	soclib::caba::DspinSignals<dspin_data_size>** s_rsp_RW;
	    	soclib::caba::DspinSignals<dspin_data_size>** s_rsp_WR;

	    	//dspin
		soclib::caba::VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>*** t_initiator_wrapper;
		soclib::caba::VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>*** t_targer_wrapper;
		soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>*** t_req_router;
		soclib::caba::DspinRouter<dspin_data_size, dspin_fifo_size>*** t_rsp_router;

		//checker
		static_assert(dspin_fifo_size <= 256 && dspin_fifo_size >= 1);

	    public:
		VciDspinNetwork( sc_module_name name,
			const soclib::common::MappingTable &mt,
			int width_network,   //X
			int height_network); //Y

		~VciDspinNetwork();
	}
}} // end 

#endif //VCI_DSPIN_NETWORK_H_
