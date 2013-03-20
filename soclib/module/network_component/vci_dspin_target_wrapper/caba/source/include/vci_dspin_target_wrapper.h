/* -*- c++ -*-
  *
  * File : vci_dspin_target_wrapper.h
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner
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

#ifndef VCI_DSPIN_TARGET_WRAPPER_H_
#define VCI_DSPIN_TARGET_WRAPPER_H_

#include <systemc>
#include "caba_base_module.h"
#include "vci_initiator.h"
#include "dspin_interface.h"

namespace soclib { namespace caba {

template<typename vci_param , size_t dspin_cmd_width, size_t dspin_rsp_width>
class VciDspinTargetWrapper
: public soclib::caba::BaseModule
{
    // CMD FSM
    enum cmd_fsm_state
    {
		CMD_IDLE,
        CMD_RW,
        CMD_READ,
        CMD_WDATA,
    }

    // RSP FSM
	enum rsp_fsm_state
    {
		RSP_IDLE,
        RSP_SINGLE,
        RSP_MULTI,
    }

    protected:
    SC_HAS_PROCESS(VciDspinTargetWrapper);

    public:
    // ports
    sc_in<bool>                             	p_clk;
    sc_in<bool>                             	p_resetn;
    soclib::caba::DspinInput<dspin_cmd_width>	p_dspin_cmd;
    soclib::caba::DspinOutput<dspin_rsp_width>	p_dspin_rsp;
	soclib::caba::VciInitiator<vci_param>      	p_vci;

	// constructor 
	VciDspinTargetWrapper( sc_module_name name );

    private:
    // internal registers
    sc_signal<int>                              r_cmd_fsm;
    sc_signal<int>                              r_rsp_fsm;
    sc_uint<dspin_cmd_width>                    r_cmd_buf0;
    sc_uint<dspin_cmd_width>                    r_cmd_buf1;

	// methods systemc
	void transition();
	void genMealy_vci_cmd();
	void genMealy_vci_rsp();
	void genMealy_dspin_cmd();
	void genMealy_dspin_rsp();
};

}} // end namespace
               
#endif // VCI_DSPIN_TARGET_WRAPPER_H_

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
