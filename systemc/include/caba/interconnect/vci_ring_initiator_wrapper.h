/////////////////////////////////////////////////////////////////////
// File     : vci_ring_initiator_wrapper.h
// Author   : Yang GAO 
// Date     : 28/09/2007
// Copyright: UPMC - LIP6
// This program is released under the GNU Public License 
//
// This component is a initiator wrapper for generic VCI anneau-ani. 
/////////////////////////////////////////////////////////////////////

#ifndef VCI_RING_INITIATOR_WRAPPER_H_
#define VCI_RING_INITIATOR_WRAPPER_H_

#include <systemc.h>
#include "caba/util/base_module.h"
#include "caba/interface/ring_ports.h"
#include "caba/interface/vci_target.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciRingInitiatorWrapper 
	: public soclib::caba::BaseModule
{

protected:
	SC_HAS_PROCESS(VciRingInitiatorWrapper);

public:
	// ports
	sc_in<bool> 					p_clk;
	sc_in<bool>   					p_resetn;
	soclib::caba::RingNegINPort<vci_param> 		p_rni;
	soclib::caba::RingDataINPort<vci_param> 	p_rdi;
	soclib::caba::RingNegOUTPort<vci_param> 	p_rno;
	soclib::caba::RingDataOUTPort<vci_param>	p_rdo;
	soclib::caba::VciTarget<vci_param>		p_vci;
	
	// constructor / destructor
	VciRingInitiatorWrapper(sc_module_name	insname);

private:
        enum fsm_state_e {
		NEG_IDLE,  	//idle
		NEG_ACK_WAIT,  	//wait for the result of negotiating
		NEG_DATA,   	//transaction of data
	};
	// internal registers
	sc_signal<int>		r_fsm_state;	  //automate
	
	sc_signal<bool>         neg_ack_ok;       //negotiating is succeed 
	sc_signal<bool>         data_eop;         //the lasted mot of a paquet 
	
	sc_signal<bool>         ring_neg_mux;	  //1 local, 0 ring
	sc_signal<bool>         ring_data_mux;    //1 local, 0 ring
	
	sc_signal<sc_uint<2> >	ring_data_cmd_p;

	// methods
	void transition();
	void genMealy();
	void ANI_Output();
	void Decoder();

};

}} // end namespace
		
#endif // VCI_RING_INITIATOR_WRAPPER_H_
