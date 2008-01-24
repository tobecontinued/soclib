/////////////////////////////////////////////////////////////////////
// File     : vci_ring_target_wrapper.h
// Author   : Yang GAO 
// Date     : 28/09/2007
// Copyright: UPMC - LIP6
// This program is released under the GNU Public License 
//
// This component is a target wrapper for generic VCI anneau-ani. 
/////////////////////////////////////////////////////////////////////

#ifndef VCI_RING_TARGET_WRAPPER_H_
#define VCI_RING_TARGET_WRAPPER_H_

#include <systemc.h>
#include "caba/util/base_module.h"
#include "caba/interface/ring_ports.h"
#include "caba/interface/vci_initiator.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciRingTargetWrapper 
	: public soclib::caba::BaseModule
{

protected:
	SC_HAS_PROCESS(VciRingTargetWrapper);

public:
	// ports
	sc_in<bool> 					p_clk;
	sc_in<bool>   					p_resetn;
	soclib::caba::RingNegINPort<vci_param> 		p_rni;
	soclib::caba::RingDataINPort<vci_param> 	p_rdi;
	soclib::caba::RingNegOUTPort<vci_param> 	p_rno;
	soclib::caba::RingDataOUTPort<vci_param> 	p_rdo;
	soclib::caba::VciInitiator<vci_param>		p_vci;
	
	// constructor / destructor
	VciRingTargetWrapper(sc_module_name	insname,
				int nseg,
				int base_adr, ...);

private:
	enum fsm_state_e {
		TAR_IDLE,
		TAR_RESERVE_FIRST,
		TAR_RESERVE,
	};

	// internal registers
	int s_nseg;	
	int* CIBLE_ID;

	sc_signal<int>		r_fsm_state;	  //automate

	sc_signal<bool>		cible_ack_ok;   
	sc_signal<bool>		cible_reserve;

	sc_signal<bool>         ring_neg_mux;   //1 local, 0 ring
	sc_signal<bool>         ring_data_mux;  //1 local, 0 ring

	sc_signal<sc_uint<2> >	ring_data_cmd_p;
	sc_signal<bool>         cible_ext;
	sc_signal<bool>		trans_end;

	// methods
	void transition();
	void genMoore();
	void genMealy();
	void ANI_Output();
	void Decoder();

};

}} // end namespace
		
#endif // VCI_RING_TARGET_WRAPPER_H_
