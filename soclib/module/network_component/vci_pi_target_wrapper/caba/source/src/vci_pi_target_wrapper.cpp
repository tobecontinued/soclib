/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Alain Greiner <alain.greiner@lip6.fr>, 2006
 *
 * Maintainers: alain nipo
 */

#include "../include/vci_pi_target_wrapper.h"  
#include "register.h"

#define Pibus soclib::caba::Pibus

namespace  soclib { namespace caba {

#define tmpl(x) template<typename vci_param> x VciPiTargetWrapper<vci_param>

/////////////////////////////////////////////////////////////////////
//	constructor
/////////////////////////////////////////////////////////////////////
tmpl(/**/)::VciPiTargetWrapper(sc_module_name insname)
  : soclib::caba::BaseModule(insname)
{
SC_METHOD (transition);
dont_initialize();
sensitive << p_clk.pos();

SC_METHOD (genMealy);
dont_initialize();
sensitive << p_clk.neg();
sensitive << p_vci.rdata;
sensitive << p_vci.rspval;
sensitive << p_vci.rerror;
sensitive << p_pi.d;
	
SOCLIB_REG_RENAME(r_fsm_state);
SOCLIB_REG_RENAME(r_adr);
SOCLIB_REG_RENAME(r_lock);
SOCLIB_REG_RENAME(r_opc);
SOCLIB_REG_RENAME(r_lock);

} //  end constructor

/*****************************************************************************
	transition 
******************************************************************************/
tmpl(void)::transition()
{

if (p_resetn == false) {
	r_fsm_state = FSM_IDLE;
	return;
}

switch (r_fsm_state) {
	case FSM_IDLE:
	if (p_sel) {
		r_adr 	= p_pi.a.read();
		r_lock 	= p_pi.lock.read();
		r_opc	= p_pi.opc.read();
		if (p_pi.read == true)  r_fsm_state = FSM_CMD_READ;
		else			r_fsm_state = FSM_CMD_WRITE;
	}
	break;

	case FSM_CMD_READ:
	if (p_vci.cmdack) r_fsm_state = FSM_RSP_READ;
	break;

	case FSM_RSP_READ:
	if (p_vci.rspval) {
		r_adr 	= p_pi.a.read();
		r_lock 	= p_pi.lock.read();
		r_opc	= p_pi.opc.read();
		if (p_vci.reop) r_fsm_state = FSM_IDLE;
		else		r_fsm_state = FSM_CMD_READ;
	}
	break;

	case FSM_CMD_WRITE:
	if (p_vci.cmdack) r_fsm_state = FSM_RSP_WRITE;
	break;

	case FSM_RSP_WRITE:
	if (p_vci.rspval) {
		r_adr 	= p_pi.a.read();
		r_lock 	= p_pi.lock.read();
		r_opc	= p_pi.opc.read();
		if (p_vci.reop) r_fsm_state = FSM_IDLE;
		else		r_fsm_state = FSM_CMD_WRITE;
	}
	break;
} // end switch FSM

}

/************************************************************************
	genMealy
************************************************************************/
tmpl(void)::genMealy()
{
switch (r_fsm_state) {
	case FSM_IDLE:
	p_vci.cmdval = false;
	p_vci.rspack = false;
	break;

	case FSM_CMD_READ:
	p_vci.cmdval = true;
	p_vci.rspack = false;
	p_vci.address	= r_adr.read();
	p_vci.wdata	= 0;
	p_vci.plen	= 0;
	p_vci.be	= 0xF;
	p_vci.cmd	= vci_param::CMD_READ;
	if (r_lock == true)	p_vci.eop = false;
	else			p_vci.eop = true;
	p_pi.ack = Pibus::ACK_WAT;
	p_pi.d   = 0;
	break;
	
	case FSM_RSP_READ:
	p_vci.cmdval = false;
	p_vci.rspack = true;
	if (p_vci.rspval) {
		p_pi.d = p_vci.rdata.read();
		if (p_vci.rerror.read() != 0)	p_pi.ack = Pibus::ACK_ERR;
		else				p_pi.ack = Pibus::ACK_RDY;
	} else {
		p_pi.ack = Pibus::ACK_WAT;
		p_pi.d   = 0;
	}
	break;
	
	case FSM_CMD_WRITE:
	p_vci.cmdval = true;
	p_vci.rspack = false;
	p_vci.address	= r_adr.read();
	p_vci.wdata	= p_pi.d.read();
	p_vci.plen	= 0;
	if 	((r_opc.read() & 0x8) == 0)		p_vci.be = 0xF;
	else if (r_opc.read() == Pibus::OPC_HW0)	p_vci.be = 0x3;
	else if (r_opc.read() == Pibus::OPC_HW1)	p_vci.be = 0xC;
	else if (r_opc.read() == Pibus::OPC_BY0)	p_vci.be = 0x1;
	else if (r_opc.read() == Pibus::OPC_BY1)	p_vci.be = 0x2;
	else if (r_opc.read() == Pibus::OPC_BY2)	p_vci.be = 0x4;
	else if (r_opc.read() == Pibus::OPC_BY3)	p_vci.be = 0x8;
	else						p_vci.be = 0x0;
	p_vci.cmd	= vci_param::CMD_WRITE;
	if (r_lock == true)	p_vci.eop = false;
	else			p_vci.eop = true;
	p_pi.ack = Pibus::ACK_WAT;
	break;

	case FSM_RSP_WRITE:
	p_vci.cmdval = false;
	p_vci.rspack = true;
	if (p_vci.rspval) {
		if (p_vci.rerror.read() != 0)	p_pi.ack = Pibus::ACK_ERR;
		else				p_pi.ack = Pibus::ACK_RDY;
	} else {
		p_pi.ack = Pibus::ACK_WAT;
		p_pi.d   = 0;
	}
	break;
} // end switch FSM

}

}} // end namespace
