/////////////////////////////////////////////////////////////////////
// File: vci_pi_initiator_wrapper.h 
// Author: Alain Greiner 
// Date: 15/04/2007  
// Copyright : UPMC - LIP6
// This program is released under the GNU Public License 
//
// This hardware component is a VCI/PIBUS protocol converter.
// It behaves as an initiator on the PIBUS and as a target on 
// the VCI side. It can be used by a VCI initiator to interface
// a PIBUS based system. 
// The VCI address and data fields must have 32 bits. 
// The supported PIBUS response are PI_ACK_RDY, PI_ACK_WAT, PI_ACK_ERR.  
// The VCI initiator is supposed to be always "ready" :   
// When a command packet starts, the CMDVAL signal is true until 
// the end of packet(no buble), and the RSPACK signal is true 
// until  the end of the response packet.   
// The VCI packet can have any length, but only READ & WRITE  
// VCI commands are supported (NOP & READLOCK are not supported)
// Most output ports, including PI.A, PI.LOCK, VCI.RDATA,  
// VCI.RERROR are Mealy signals.  
/////////////////////////////////////////////////////////////////////

#ifndef VCI_PI_INITIATOR_WRAPPER_H_
#define VCI_PI_INITIATOR_WRAPPER_H_

#include <systemc>
#include "caba/util/base_module.h"
#include "caba/interface/pibus_initiator_ports.h"
#include "caba/interface/vci_target.h"

namespace soclib { namespace caba {

	using namespace sc_core;

template<typename vci_param>
class VciPiInitiatorWrapper 
	: public soclib::caba::BaseModule
{

enum fsm_state_e {
	 FSM_IDLE,
	 FSM_REQ,
	 FSM_AD,
	 FSM_AD_DT,
	 FSM_DT,
	 };

protected:
	SC_HAS_PROCESS(VciPiInitiatorWrapper);

public:
	// ports
	sc_in<bool> 				p_clk;
	sc_in<bool>   				p_resetn;
	sc_in<bool>   				p_gnt;
	sc_out<bool>   				p_req;
	soclib::caba::PibusInitiator p_pi;
	soclib::caba::VciTarget<vci_param>	p_vci;
	
	// constructor / destructor
	VciPiInitiatorWrapper(sc_module_name	insname);

private:
	// internal registers
	sc_signal<int>				r_fsm_state;
	sc_signal<int>				r_wdata;
	sc_signal<int>				r_srcid;
	sc_signal<int>				r_pktid;
	sc_signal<int>				r_trdid;
	sc_signal<int>				r_opc;
	sc_signal<bool>				r_read;

	// methods
	void transition();
	void genMealy();

        static_assert(vci_param::N == 32); // checking VCI address size
        static_assert(vci_param::B == 4); // checking VCI data size
};

}} // end namespace
		
#endif // VCI_PI_INITIATOR_WRAPPER_H_
