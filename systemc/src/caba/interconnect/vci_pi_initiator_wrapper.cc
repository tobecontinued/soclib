/////////////////////////////////////////////////////////////////////
//  File: vci_pi_initiator_wrapper.cc  
//  Author: Alain Greiner 
//  Date: 15/04/2007 
//  Copyright : UPMC - LIP6
//  This program is released under the GNU General Public License 
/////////////////////////////////////////////////////////////////////

#include "caba/interconnect/vci_pi_initiator_wrapper.h"
#include "common/register.h"

namespace soclib { namespace caba {

#define Pibus soclib::caba::Pibus

#define tmpl(x) template<typename vci_param> x VciPiInitiatorWrapper<vci_param>

////////////////////////////////
//	constructor
////////////////////////////////

tmpl(/**/)::VciPiInitiatorWrapper(sc_module_name insname)
  : soclib::caba::BaseModule(insname)
{
SC_METHOD (transition);
dont_initialize();
sensitive << p_clk.pos();

SC_METHOD (genMealy);
dont_initialize();
sensitive  << p_clk.neg();
sensitive  << p_vci.address;
sensitive  << p_vci.eop;
sensitive  << p_pi.ack;
sensitive  << p_pi.d;

SOCLIB_REG_RENAME(r_fsm_state);
SOCLIB_REG_RENAME(r_wdata);
SOCLIB_REG_RENAME(r_srcid);
SOCLIB_REG_RENAME(r_pktid);
SOCLIB_REG_RENAME(r_trdid);
SOCLIB_REG_RENAME(r_opc);
SOCLIB_REG_RENAME(r_read);

} //  end constructor

////////////////////////////////
//	transition 
////////////////////////////////
tmpl(void)::transition()
{
if (p_resetn == false) {
	r_fsm_state = FSM_IDLE;
	return;
} // end reset

switch (r_fsm_state) {
	case FSM_IDLE:
        if (p_vci.cmdval) {
		r_fsm_state 	= FSM_REQ;
		r_srcid		= (int)p_vci.srcid.read();
		r_pktid		= (int)p_vci.pktid.read();
		r_trdid		= (int)p_vci.trdid.read();
		if 	(p_vci.cmd.read() == vci_param::CMD_READ)  r_read = true;
		else if (p_vci.cmd.read() == vci_param::CMD_WRITE) r_read = false;
		else 	{
      			printf("ERROR : The vci_pi_initiator_wrapper accepts only\n");
      			printf("vci_param::CMD_READ and vci_param::CMD_WRITE commands\n");
			exit(1);
        		} 
		if      (p_vci.be.read() == 0xF) 	r_opc = Pibus::OPC_WDU;
		else if (p_vci.be.read() == 0x3) 	r_opc = Pibus::OPC_HW0;
		else if (p_vci.be.read() == 0xC) 	r_opc = Pibus::OPC_HW1;
		else if (p_vci.be.read() == 0x1) 	r_opc = Pibus::OPC_BY0;
		else if (p_vci.be.read() == 0x2) 	r_opc = Pibus::OPC_BY1;
		else if (p_vci.be.read() == 0x4) 	r_opc = Pibus::OPC_BY2;
		else if (p_vci.be.read() == 0x8) 	r_opc = Pibus::OPC_BY3;
		else 	{
      			printf("ERROR : The vci_pi_initiator_wrapper accepts only VCI BE\n");
      			printf("corresponding to WDU, HW0, HW1, BY0, BY1, BY2, BY3 formats\n");
			exit(1);
        		} 
	} // end if cmdval
	break;

	case FSM_REQ:
        if (p_gnt) r_fsm_state = FSM_AD;
        break;

	case FSM_AD:
	r_wdata = (int)p_vci.wdata.read();
        if (p_vci.eop) r_fsm_state = FSM_DT;
	else           r_fsm_state = FSM_AD_DT;
        break;

	case FSM_AD_DT:
	if (p_vci.cmdval == false) {
      		printf("ERROR : The vci_pi_initiator_wrapper assumes that\n");
      		printf("there is no \"buble\" in a VCI command packet\n");
		exit(1);
        	} 
	if (p_vci.rspack == false) {
      		printf("ERROR : The vci_pi_initiator_wrapper assumes that\n");
      		printf("the VCI initiator always accept the response packet\n");
		exit(1);
        	} 
	r_wdata = (int)p_vci.wdata.read();
	if (p_vci.eop) {
		if      (p_pi.ack.read() == Pibus::ACK_RDY) r_fsm_state = FSM_DT;
		else if (p_pi.ack.read() == Pibus::ACK_WAT) r_fsm_state = FSM_AD_DT;
		else if (p_pi.ack.read() == Pibus::ACK_ERR) r_fsm_state = FSM_DT;
		else	{
      			printf("ERROR : The vci_pi_initiator_wrapper accepts only\n");
      			printf("Pibus::ACK_RDY, Pibus::ACK_WAT & Pibus::ACK_ERR responses\n");
			exit(1);
        		} 
	} else {
		if      (p_pi.ack.read() == Pibus::ACK_RDY) r_fsm_state = FSM_AD_DT;
		else if (p_pi.ack.read() == Pibus::ACK_WAT) r_fsm_state = FSM_AD_DT;
		else if (p_pi.ack.read() == Pibus::ACK_ERR) r_fsm_state = FSM_AD_DT;
		else	{
      			printf("ERROR : The vci_pi_initiator_wrapper accepts only\n");
      			printf("Pibus::ACK_RDY, Pibus::ACK_WAT & Pibus::ACK_ERR responses\n");
			exit(1);
        		} 
	}
        break;

	case FSM_DT:
	if (p_vci.rspack == false) {
      		printf("ERROR : The vci_pi_initiator_wrapper assumes that\n");
      		printf("the VCI initiator always accept the response packet\n");
		exit(1);
        	} 
	if      (p_pi.ack.read() == Pibus::ACK_RDY)  r_fsm_state = FSM_IDLE;
	else if (p_pi.ack.read() == Pibus::ACK_WAT)  r_fsm_state = FSM_DT;
	else if (p_pi.ack.read() == Pibus::ACK_ERR)  r_fsm_state = FSM_IDLE;
	else	{
      		printf("ERROR : The vci_pi_initiator_wrapper accepts only\n");
      		printf("Pibus::ACK_RDY, Pibus::ACK_WAT & Pibus::ACK_ERR responses\n");
		exit(1);
        	} 
        break;
        } // end switch fsm
}; // end transition

////////////////////////////////
//	genMealy
////////////////////////////////
tmpl(void)::genMealy()
{
switch (r_fsm_state) {
       
	case FSM_IDLE:
	p_req     	= false;
	p_vci.cmdack 	= false;
	p_vci.rspval 	= false;
        break;

	case FSM_REQ:
	p_req     	= true;
	p_vci.cmdack 	= false;
	p_vci.rspval 	= false;
        break;

	case FSM_AD:
	p_req     	= false;
	p_vci.cmdack 	= true;
	p_vci.rspval 	= false;
	p_pi.a 		= p_vci.address.read();
	p_pi.read	= r_read;
	p_pi.opc	= (sc_dt::sc_uint<4>)r_opc;
	p_pi.lock 	= !p_vci.eop;
        break;

	case FSM_AD_DT:
	p_req     	= false;
	p_vci.reop	= false;
	p_vci.rsrcid	= r_srcid.read();
	p_vci.rpktid	= r_pktid.read();
	p_vci.rtrdid	= r_trdid.read();
	if (p_pi.ack.read() == Pibus::ACK_RDY) {
		p_vci.cmdack 	= true;
		p_vci.rspval 	= true;
		p_vci.rerror	= 0;
	} else if (p_pi.ack.read() == Pibus::ACK_ERR) {
		p_vci.cmdack 	= true;
		p_vci.rspval 	= true;
		p_vci.rerror	= 1;
	} else {
		p_vci.cmdack	= false;
		p_vci.rspval	= false;
		p_vci.rerror	= 0;
	}
	if (r_read == false) 	p_pi.d  	= r_wdata.read();
	else			p_vci.rdata 	= p_pi.d.read();
	p_pi.a 		= p_vci.address.read();
	p_pi.read	= r_read;
	p_pi.opc	= (sc_dt::sc_uint<4>)r_opc;
	p_pi.lock 	= !p_vci.eop;
        break;

	case FSM_DT:
	p_req     	= false;
	p_vci.cmdack 	= false;
	p_vci.reop	= true;
	p_vci.rsrcid	= r_srcid.read();
	p_vci.rpktid	= r_pktid.read();
	p_vci.rtrdid	= r_trdid.read();
	if (p_pi.ack.read() == Pibus::ACK_RDY) {
		p_vci.rspval 	= true;
		p_vci.rerror	= 0;
	} else if (p_pi.ack.read() == Pibus::ACK_ERR) {
		p_vci.rspval 	= true;
		p_vci.rerror	= 1;
	} else {
		p_vci.rspval	= false;
		p_vci.rerror	= 0;
	}
	if (r_read == false) 	p_pi.d  	= r_wdata.read();
	else			p_vci.rdata 	= p_pi.d.read();
	break;
        } // end switch
}; // end genMealy

}} // end namespace
