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

    template<typename vci_param, int dspin_fifo_size, int dspin_yx_size>
	class VciDspinInitiatorWrapper
	: public soclib::caba::BaseModule
	{

	    // FSM of request
	    enum fsm_state_req{
		REQ_DSPIN_HEADER,
		REQ_VCI_ADDRESS_HEADER,
		REQ_VCI_CMD_READ_HEADER,
		REQ_VCI_CMD_WRITE_HEADER,
		REQ_VCI_DATA_PAYLOAD,
	    };

	    // FSM of response
	    enum fsm_state_rsp{
		RSP_DSPIN_HEADER,
		RSP_VCI_HEADER,		
		RSP_VCI_DATA_PAYLOAD,
	    };

	    protected:
	    SC_HAS_PROCESS(VciDspinInitiatorWrapper);

	    public:
	    // ports
	    sc_in<bool>                             	p_clk;
	    sc_in<bool>                             	p_resetn;

	    // fifo req ant rsp
	    DspinOutput<38>				p_dspin_out;
	    DspinInput<34>				p_dspin_in;

	    // ports vci
	    soclib::caba::VciTarget<vci_param>      	p_vci;

	    // constructor / destructor
	    VciDspinInitiatorWrapper(sc_module_name insname, const soclib::common::MappingTable &mt);

	    private:
	    // internal registers
	    sc_signal<int>        r_fsm_state_req;
	    sc_signal<int>        r_fsm_state_rsp;
	    sc_signal<int>        r_srcid;
	    sc_signal<int>        r_pktid;
	    sc_signal<int>        r_trdid;
	    sc_signal<int>        r_error;

	    // deux fifos req and rsp
	    soclib::caba::GenericFifo<sc_uint<38> >  fifo_req;
	    soclib::caba::GenericFifo<sc_uint<34> >  fifo_rsp;

	    // methods systemc
	    void transition();
	    void genMoore();

	    // routing table
	    soclib::common::AddressDecodingTable<uint32_t, int>  m_routing_table;
	    int                                                  srcid_mask;

	    // checker
	    static_assert(vci_param::N == 32 || vci_param::N == 36); // checking VCI address size
	    static_assert(vci_param::B == 4);   // checking VCI data size

        static_assert(vci_param::K <= 8); // checking plen size
        static_assert(vci_param::S <= 14); // checking srcid size
        static_assert(vci_param::P <= 4); // checking pktid size
        static_assert(vci_param::T <= 4); // checking trdid size
        static_assert(vci_param::E <= 4); // checking rerror size

	    static_assert(dspin_fifo_size <= 256 && dspin_fifo_size >= 1); // checking FIFO size
	    static_assert(dspin_yx_size <= 6 && dspin_yx_size >= 1);  // checking DSPIN index size
	};

}} // end namespace
               
#endif // VCI_DSPIN_INITIATOR_WRAPPER_H_

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
