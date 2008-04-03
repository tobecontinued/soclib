/* -*- c++ -*-
  * File : vci_dspin_initiator_warpper.h
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

#ifndef VCI_DSPIN_INITIATOR_WRAPPER_H_
#define VCI_DSPIN_INITIATOR_WRAPPER_H_

#include <systemc>
#include "caba_base_module.h"
#include "vci_target.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "dspin_interface.h"

namespace soclib { namespace caba {

    using namespace sc_core;

    template<typename vci_param, int dspin_data_size, int dspin_fifo_size>
	class VciDspinInitiatorWrapper
	: public soclib::caba::BaseModule
	{

	    // FSM of request
	    enum fsm_state_req{
		REQ_HEADER,
		REQ_ADDRESS_READ,
		REQ_ADDRESS_WRITE,
		REQ_DATA_WRITE,
	    };

	    // FSM of response
	    enum fsm_state_rsp{
		RSP_HEADER,
		RSP_DATA,
	    };

	    protected:
	    SC_HAS_PROCESS(VciDspinInitiatorWrapper);

	    public:
	    // ports
	    sc_in<bool>                             	p_clk;
	    sc_in<bool>                             	p_resetn;

	    // fifo req ant rsp
	    DspinOutput<dspin_data_size>		p_dspin_out;
	    DspinInput<dspin_data_size>			p_dspin_in;

	    // ports vci
	    soclib::caba::VciTarget<vci_param>      	p_vci;

	    // constructor / destructor
	    VciDspinInitiatorWrapper(sc_module_name    insname,
		    const soclib::common::MappingTable &mt);

	    private:
	    // internal registers
	    sc_signal<int>				r_fsm_state_req;
	    sc_signal<int>				r_fsm_state_rsp;
	    sc_signal<int>                  		r_srcid;
	    sc_signal<int>                  		r_pktid;
	    sc_signal<int>                  		r_trdid;

	    // deux fifos req and rsp
	    soclib::caba::GenericFifo<sc_uint<dspin_data_size> >  fifo_req;
	    soclib::caba::GenericFifo<sc_uint<dspin_data_size> >  fifo_rsp;

	    // methods systemc
	    void transition();
	    void genMoore();

	    // routing table
	    soclib::common::AddressDecodingTable<uint32_t, int> 	m_routing_table;
	    int						srcid_mask;

	    // methods
	    bool parity(int val);

	    // checker
	    static_assert(vci_param::N == 32); // checking VCI address size
	    static_assert(vci_param::B == 4);  // checking VCI data size
	    static_assert(vci_param::E == 1);  // checking VCI error size
	    static_assert(dspin_fifo_size <= 256 && dspin_fifo_size >= 1);
	};

}} // end namespace
               
#endif // VCI_DSPIN_INITIATOR_WRAPPER_H_
