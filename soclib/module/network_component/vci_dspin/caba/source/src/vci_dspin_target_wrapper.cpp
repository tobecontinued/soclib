/* -*- c++ -*-
  * File : vci_dspin_initiator_warpper.h
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner, Abbas Sheibanyrad, Ivan Miro, Zhen Zhang
  *
  * SOCLIB_LGPL_HEADER_BEGIN
  * SoCLib is free software; you can redistribute it and/or modify it
  * under the terms of the GNU Lesser General Public License as published
  * by the Free Software Foundation; version 2.1 of the License.
  * SoCLib is distributed in the hope that it will be useful, but
  * WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * You should have received a copy of the GNU Lesser General Public
  * License along with SoCLib; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  * 02110-1301 USA
  * SOCLIB_LGPL_HEADER_END
  */

#include "../include/vci_dspin_target_wrapper.h"
#include "register.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename vci_param, int dspin_data_size, int dspin_fifo_size, int dspin_srcid_msb_size> x VciDspinTargetWrapper<vci_param, dspin_data_size, dspin_fifo_size, dspin_srcid_msb_size>

    ////////////////////////////////
    //      constructor
    ////////////////////////////////

    tmpl(/**/)::VciDspinTargetWrapper(sc_module_name insname)
	       : soclib::caba::BaseModule(insname),
	       fifo_req("FIFO_REQ", dspin_fifo_size),
	       fifo_rsp("FIFO_RSP", dspin_fifo_size)
    {
	SC_METHOD (transition);
	dont_initialize();
	sensitive << p_clk.pos();
	SC_METHOD (genMoore);
	dont_initialize();
	sensitive  << p_clk.neg();

	SOCLIB_REG_RENAME(r_fsm_state_req);
	SOCLIB_REG_RENAME(r_fsm_state_rsp);
	SOCLIB_REG_RENAME(r_cmd);
	SOCLIB_REG_RENAME(r_be);
	SOCLIB_REG_RENAME(r_msbad);
	SOCLIB_REG_RENAME(r_lsbad);
	SOCLIB_REG_RENAME(r_cons);
	SOCLIB_REG_RENAME(r_contig);
	SOCLIB_REG_RENAME(r_srcid);
	SOCLIB_REG_RENAME(r_pktid);
	SOCLIB_REG_RENAME(r_trdid);

    } //  end constructor

    ////////////////////////////////
    //      functions
    ///////////////////////////////
    tmpl(bool)::parity(int val){
	int tmp = 0;
	for (int i = 0; i < 32; i++){
	    tmp = tmp + (val >> i);
	}
	return (tmp & 1 == 1);
    };

    ////////////////////////////////
    //      transition
    ////////////////////////////////
    tmpl(void)::transition()
    {
	sc_uint<36>	req_fifo_data;
	bool		req_fifo_write;
	bool		req_fifo_read;
	sc_uint<36>	rsp_fifo_data;
	bool		rsp_fifo_write;
	bool		rsp_fifo_read;

	if (p_resetn == false) {
	    fifo_req.init();
	    fifo_rsp.init();
	    r_fsm_state_req = REQ_IDLE;
	    r_fsm_state_rsp = RSP_HEADER;
	    return;
	} // end reset

	// DSPIN request to VCI request 
	// The DSPIN packet is written into the fifo_req
	// and the FIFO output is analysed and translated

	// req_fifo_write and req_fifo_data
	req_fifo_write = p_dspin_in.write.read();
	req_fifo_data  = p_dspin_in.data.read();

	// r_fsm_state_req, req_fifo_read, BUF_CMD, BUF_SRCID, BUF_PKTID, BUF_TRDID, BUF_MSBAD, BUF_LSBAD
	switch(r_fsm_state_req) {
	    case REQ_IDLE :
		req_fifo_read = true;
		if(fifo_req.rok() == true) {
		    r_cmd    = (sc_uint<2>)  ((fifo_req.read() >> 24) & 0x000000003); 
		    r_srcid  = (sc_uint<vci_param::S>)  ((fifo_req.read() >> 8)  & 0x0000003ff);
		    r_msbad  = (sc_uint<vci_param::N>)  ((fifo_req.read())       & 0x0f0000000);
		    r_pktid  = (sc_uint<vci_param::P>)  ((fifo_req.read() >> 18) & 0x000000003);
		    r_trdid  = (sc_uint<vci_param::T>)  ((fifo_req.read() >> 20) & 0x00000000f);
		    if((fifo_req.read() & (sc_uint<36>)DSPIN_CONS) == (sc_uint<36>)DSPIN_CONS) { r_cons = true; } 
		    else 								       { r_cons = false; }
		    if((fifo_req.read() & (sc_uint<36>)DSPIN_CONTIG) == (sc_uint<36>)DSPIN_CONTIG) { r_contig = true; } 
		    else 									   { r_contig = false; }
		    if((sc_uint<2>)(fifo_req.read() >> 24) == vci_param::CMD_WRITE) {
			r_fsm_state_req = REQ_ADDRESS_WRITE; 
		    } else {
			r_fsm_state_req = REQ_ADDRESS_READ; 
		    }
		}
		break;
	    case REQ_ADDRESS_READ :
		req_fifo_read = p_vci.cmdack.read();
		if((p_vci.cmdack.read() == true) && (fifo_req.rok() == true) && 
			(((fifo_req.read() >> 32) & DSPIN_EOP) == DSPIN_EOP)) 
		{ r_fsm_state_req = REQ_IDLE; }
		break;
	    case REQ_ADDRESS_WRITE :
		req_fifo_read = true;
		if(fifo_req.rok() == true) {
		    r_be     = (sc_uint<vci_param::B>)  (fifo_req.read() >> 28);
		    r_lsbad  = (sc_uint<vci_param::N>)  (fifo_req.read()) & 0x0fffffff;
		    r_fsm_state_req    = REQ_DATA_WRITE;
		}
		break;
	    case REQ_DATA_WRITE :
		req_fifo_read = p_vci.cmdack.read();
		if((p_vci.cmdack.read() == true) && (fifo_req.rok() == true)) {
		    if(((fifo_req.read() >> 32) & DSPIN_EOP) == DSPIN_EOP) {
			r_fsm_state_req = REQ_IDLE;
		    } else {
			r_fsm_state_req = REQ_ADDRESS_WRITE;
		    }
		}
		break;
	} // end switch r_fsm_state_req

	// fifo_req
	if((req_fifo_write == true) && (req_fifo_read == false)) { fifo_req.simple_put(req_fifo_data); } 
	if((req_fifo_write == true) && (req_fifo_read == true))  { fifo_req.put_and_get(req_fifo_data); } 
	if((req_fifo_write == false) && (req_fifo_read == true)) { fifo_req.simple_get(); }


	// VCI response to DSPIN response 
	// The VCI packet is analysed, translated, and
	// the SPIN packet is written into the fifo_rsp
	// 
	// rsp_fifo_read 
	rsp_fifo_read  = p_dspin_out.read.read();

	// r_fsm_state_rsp, rsp_fifo_write and rsp_fifo_data
	switch(r_fsm_state_rsp) {
	    case RSP_HEADER :
		rsp_fifo_write = p_vci.rspval.read();
		if((p_vci.rspval.read() == true) && (fifo_rsp.wok() == true)) { 
		    rsp_fifo_data = ((sc_uint<36>)p_vci.rsrcid.read() >>(vci_param::S - dspin_srcid_msb_size)) |// take only the MSB bits
			(((sc_uint<36>)p_vci.rsrcid.read())<<8) |
			(((sc_uint<36>)p_vci.rpktid.read() & 0x03) << 18) |
			(((sc_uint<36>)p_vci.rtrdid.read() & 0x0f) << 20) |
			(((sc_uint<36>)DSPIN_BOP)                <<32)  ; 
		    if(parity(rsp_fifo_data) == true) { 
			rsp_fifo_data = rsp_fifo_data | ((sc_uint<36>)DSPIN_PAR << 32); } 
		    r_fsm_state_rsp = RSP_DATA; 
		}
		break;
	    case RSP_DATA :
		rsp_fifo_write = p_vci.rspval.read();
		if((p_vci.rspval.read() == true) && (fifo_rsp.wok() == true)) { 
		    rsp_fifo_data = (sc_uint<36>)p_vci.rdata.read(); 
		    if(parity(rsp_fifo_data) == true) { 
			rsp_fifo_data = rsp_fifo_data | ((sc_uint<36>)DSPIN_PAR << 32); } 
		    if(p_vci.rerror.read() != 0) { 
			rsp_fifo_data = rsp_fifo_data | ((sc_uint<36>)DSPIN_ERR << 32); }
		    if(p_vci.reop.read() == true) { 
			rsp_fifo_data = rsp_fifo_data | ((sc_uint<36>)DSPIN_EOP << 32); 
			r_fsm_state_rsp = RSP_HEADER;
		    }
		}
		break;
	} // end switch r_fsm_state_rsp

	// fifo_rsp
	if((rsp_fifo_write == true) && (rsp_fifo_read == false)) { fifo_rsp.simple_put(rsp_fifo_data); } 
	if((rsp_fifo_write == true) && (rsp_fifo_read == true))  { fifo_rsp.put_and_get(rsp_fifo_data); } 
	if((rsp_fifo_write == false) && (rsp_fifo_read == true)) { fifo_rsp.simple_get(); }
    }; // end transition

    ////////////////////////////////
    //      genMealy
    ////////////////////////////////
    tmpl(void)::genMoore()
    {
	// VCI REQ interface

	switch(r_fsm_state_req) {
	    case REQ_IDLE :
		p_vci.cmdval = false;
		break;
	    case REQ_ADDRESS_READ :
		p_vci.cmdval = fifo_req.rok();
		p_vci.address = ((sc_uint<vci_param::N>)(fifo_req.read()) & 0x0fffffff) | (sc_uint<vci_param::N>)r_msbad;
		p_vci.be = (sc_uint<vci_param::B>)(fifo_req.read() >> 28);
		p_vci.cmd = r_cmd;
		p_vci.wdata = 0;
		p_vci.pktid = r_pktid;
		p_vci.srcid = r_srcid;
		p_vci.trdid = r_trdid;
		p_vci.plen = 0;
		p_vci.clen = 0;
		p_vci.cfixed = false;
		p_vci.cons = r_cons;
		p_vci.contig = r_contig;
		p_vci.wrap = false;
		if(((int)(fifo_req.read() >> 32) & DSPIN_EOP) == DSPIN_EOP) { p_vci.eop = true; }
		else                                                        { p_vci.eop = false; }
		break;

	    case REQ_ADDRESS_WRITE :
		p_vci.cmdval = false;
		break;

	    case REQ_DATA_WRITE :
		p_vci.cmdval = fifo_req.rok();
		p_vci.address = (sc_uint<vci_param::N>)r_lsbad | (sc_uint<vci_param::N>)r_msbad;
		p_vci.be = r_be;
		p_vci.cmd = r_cmd;
		p_vci.wdata = (sc_uint<8*vci_param::B>)(fifo_req.read());
		p_vci.pktid = r_pktid;
		p_vci.srcid = r_srcid;
		p_vci.trdid = r_trdid;
		p_vci.plen = 0;
		p_vci.clen = 0;
		p_vci.cfixed = false;
		p_vci.cons = r_cons;
		p_vci.contig = r_contig;
		p_vci.wrap = false;
		if(((int)(fifo_req.read() >> 32) & DSPIN_EOP) == DSPIN_EOP) { p_vci.eop = true; }
		else                                                        { p_vci.eop = false; }
		break;

	} // end switch r_fsm_state_req

	// VCI RSP interface

	if((r_fsm_state_rsp != RSP_HEADER) && (fifo_rsp.wok() == true)) {p_vci.rspack = fifo_rsp.wok();}
	else                                                            {p_vci.rspack = false;}

	// p_dspin_in interface

	p_dspin_in.read = fifo_req.wok();

	// p_dspin_out interface

	p_dspin_out.write = fifo_rsp.rok();
	p_dspin_out.data = fifo_rsp.read();

    }; // end genMoore

}} // end namespace

