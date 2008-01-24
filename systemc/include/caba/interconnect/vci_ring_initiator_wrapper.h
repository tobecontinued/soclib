/*
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
 *
 * Author   : Yang GAO 
 * Date     : 28/09/2007
 * Copyright: UPMC - LIP6
 */

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
	
	sc_signal<bool>         r_neg_ack_ok;       //negotiating is succeed 
	sc_signal<bool>         r_data_eop;         //the lasted mot of a paquet 
	
	sc_signal<bool>         r_ring_neg_mux;	  //1 local, 0 ring
	sc_signal<bool>         r_ring_data_mux;    //1 local, 0 ring
	
	sc_signal<sc_uint<2> >	r_ring_data_cmd_p;

	// methods
	void transition();
	void genMealy();
	void ANI_Output();
	void Decoder();
	void Mux();

};

}} // end namespace
		
#endif // VCI_RING_INITIATOR_WRAPPER_H_
