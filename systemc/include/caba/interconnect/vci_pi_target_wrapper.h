// -*- c++ -*-
// File: vci_pi_target_wrapper.h
// Author: Alain Greiner 
// Date: 15/04/2007  
// Copyright : UPMC - LIP6
// This program is released under the GNU General Public License. 
/////////////////////////////////////////////////////////////////////
// This hardware component is a VCI/PIBUS protocol converter.
// It behaves as a target on the PIBUS, and as an initiator  
// on the VCI side. It can be used by a VCI target to interface 
// a PIBUS based system. 
// It contains a single FSM that controls the three 
// interfaces: VCI command, VCI response, and PIBUS.        
// Therefore, the throuhput cannot be larger than 2 cycles     
// per 32 bits word, even in case of burst.
// The supported PIBUS response codes are PI_ACK_RDY, PI_ACK_WAT
// and PI_ACK_ERR.  
/////////////////////////////////////////////////////////////////////

#ifndef VCI_PI_TARGET_WRAPPER_H_
#define VCI_PI_TARGET_WRAPPER_H_

#include <systemc.h>
#include "caba/interface/pibus_target_ports.h"
#include "caba/util/base_module.h"
#include "caba/interface/vci_initiator.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciPiTargetWrapper 
	: public soclib::caba::BaseModule
{

protected:
	SC_HAS_PROCESS(VciPiTargetWrapper);

public:
	// ports
	sc_in<bool> 				p_clk;
	sc_in<bool>   				p_resetn;
	sc_in<bool>				p_sel;
	soclib::caba::PibusTarget 	p_pi;
	soclib::caba::VciInitiator<vci_param>	p_vci;
	
	// constructor / destructor
	VciPiTargetWrapper(sc_module_name	insname);

	enum fsm_state_e {
	FSM_IDLE,
	FSM_CMD_READ,
	FSM_RSP_READ,
	FSM_CMD_WRITE,
	FSM_RSP_WRITE,
	};

private:
	// internal registers
	sc_signal<int>				r_fsm_state;
	sc_signal<int>				r_adr;
	sc_signal<int>				r_opc;
	sc_signal<bool>				r_lock;

	// methods
	void transition();
	void genMealy();

	static_assert(vci_param::N == 32); // checking VCI address size
	static_assert(vci_param::B == 4); // checking VCI data size
};

}}
		
#endif // VCI_PI_TARGET_WRAPPER_H_
