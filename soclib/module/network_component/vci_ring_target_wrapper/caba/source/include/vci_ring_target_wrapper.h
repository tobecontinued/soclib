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
 * Author   : Franck WAJSBÜRT, Yang GAO 
 * Date     : 28/09/2007
 * Copyright: UPMC - LIP6
 */

#ifndef VCI_RING_TARGET_WRAPPER_H_
#define VCI_RING_TARGET_WRAPPER_H_

#include <systemc.h>
#include "caba_base_module.h"
#include "vci_ring_ports.h"
#include "vci_initiator.h"

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
	soclib::caba::RingINPort<vci_param> 		p_ri;
	soclib::caba::RingOUTPort<vci_param> 		p_ro;
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

	sc_signal<bool>		r_cible_ack_ok;   
	sc_signal<bool>		r_cible_reserve;
	sc_signal<typename vci_param::srcid_t>	r_reserve_srcid;

	sc_signal<bool>         r_ring_neg_mux;   //1 local, 0 ring
	sc_signal<bool>         r_ring_data_mux;  //1 local, 0 ring

	sc_signal<sc_uint<3> >	r_ring_data_cmd_p;
	sc_signal<bool>         r_cible_ext;
	sc_signal<bool>		r_trans_end;

	sc_signal<sc_uint<4> >	r_counter_req;
	sc_signal<sc_uint<4> >	r_counter_res;

	// methods
	void transition();
	void genMoore();
	void genMealy();
	void ANI_Output();
	void Decoder();
	void Mux();
	void Req_Counter();
	void Res_Counter();

};

}} // end namespace
		
#endif // VCI_RING_TARGET_WRAPPER_H_
