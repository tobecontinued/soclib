/* -*- c++ -*-
  * File : vci_dspin_initiator_warpper.cpp
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

#include "../include/vci_dspin_initiator_wrapper.h"
#include "register.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename vci_param, int dspin_data_size, int dspin_fifo_size> x VciDspinInitiatorWrapper<vci_param, dspin_data_size, dspin_fifo_size>

    ////////////////////////////////
    //      constructor
    ////////////////////////////////

    tmpl(/**/)::VciDspinInitiatorWrapper(sc_module_name insname,
					 const soclib::common::MappingTable &mt)
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
	SOCLIB_REG_RENAME(r_srcid);
	SOCLIB_REG_RENAME(r_pktid);
	SOCLIB_REG_RENAME(r_trdid);

	m_routing_table = mt.getRoutingTable(soclib::common::IntTab(), 0);
	srcid_mask = 0x7FFFFFFF >> ( 31 - vci_param::S );
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
    }

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
	    r_fsm_state_req = REQ_HEADER;
	    r_fsm_state_rsp = RSP_HEADER;
	    return;
	} // end reset

	// VCI request to DSPIN request
	// The VCI packet is analysed, translated,
	// and the DSPIN packet is stored in the fifo_req

	// req_fifo_read
	req_fifo_read = p_dspin_out.read.read();

	// r_fsm_state_req, req_fifo_write and req_fifo_data
	switch(r_fsm_state_req) {
	    case REQ_HEADER :
		if(p_vci.cmdval == true) {
		    req_fifo_write = true;
		    req_fifo_data = (sc_uint<36>) (m_routing_table[(int)(p_vci.address.read())])  |
			(sc_uint<36>) (p_vci.address.read() & 0xF0000000)  |
			(sc_uint<36>) ((p_vci.pktid.read() & 0x03)  << 18) |
			(sc_uint<36>) ((p_vci.trdid.read() & 0x0F)  << 20) |
			(sc_uint<36>) ((p_vci.cmd.read())           << 24) |
			(sc_uint<36>) ((p_vci.srcid.read()& 0x3ff)  << 8)  |
			(((sc_uint<36>) DSPIN_BOP)                  << 32) ;
		    if(p_vci.contig.read() == true) 
		    { req_fifo_data = req_fifo_data | (sc_uint<36>) DSPIN_CONTIG; } 
		    if(p_vci.cons.read() == true) 
		    { req_fifo_data = req_fifo_data | (sc_uint<36>) DSPIN_CONS; } 
		    if(parity(req_fifo_data) == true) 
		    { req_fifo_data = req_fifo_data | ((sc_uint<36>) DSPIN_PAR) << 32; } 
		    if(fifo_req.wok() == true) {
			if(p_vci.cmd.read() == vci_param::CMD_WRITE) {r_fsm_state_req = REQ_ADDRESS_WRITE;} 
			else                                	     {r_fsm_state_req = REQ_ADDRESS_READ;} 
		    }
		} else {
		    req_fifo_write = false;
		}
		break;
	    case REQ_ADDRESS_READ :
		req_fifo_write = p_vci.cmdval;
		if((p_vci.cmdval == true) && (fifo_req.wok() == true)) {
		    req_fifo_data = (sc_uint<36>) (p_vci.address.read() & 0x0FFFFFFF) |
			(((sc_uint<36>) p_vci.be.read()) << 28)           ;
		    if(parity(req_fifo_data) == true) 
		    { req_fifo_data = req_fifo_data | ((sc_uint<36>) DSPIN_PAR) << 32; } 
		    if(p_vci.eop == true) {
			req_fifo_data = req_fifo_data | (((sc_uint<36>) DSPIN_EOP) << 32);
			r_fsm_state_req = REQ_HEADER;
		    }
		}
		break;
	    case REQ_ADDRESS_WRITE :
		req_fifo_write = p_vci.cmdval;
		if((p_vci.cmdval == true) && (fifo_req.wok() == true)) {
		    req_fifo_data = (sc_uint<36>) (p_vci.address.read() & 0x0FFFFFFF) |
			(((sc_uint<36>) p_vci.be.read()) << 28)           ;
		    if(parity(req_fifo_data) == true) 
		    { req_fifo_data = req_fifo_data | ((sc_uint<36>) DSPIN_PAR) << 32; } 
		    r_fsm_state_req = REQ_DATA_WRITE;
		}
		break;
	    case REQ_DATA_WRITE :
		req_fifo_write = p_vci.cmdval;
		if((p_vci.cmdval == true) && (fifo_req.wok() == true)) {
		    req_fifo_data = (sc_uint<36>) p_vci.wdata.read();
		    if(parity(req_fifo_data) == true) 
		    { req_fifo_data = req_fifo_data | ((sc_uint<36>) DSPIN_PAR) << 32; } 
		    if(p_vci.eop == true) {
			req_fifo_data = req_fifo_data | (((sc_uint<36>) DSPIN_EOP) << 32);
			r_fsm_state_req = REQ_HEADER;
		    }else{ 
			r_fsm_state_req=REQ_ADDRESS_WRITE;
		    }
		}
		break;
	} // end switch r_fsm_state_req

	// fifo_req
	if((req_fifo_write == true) && (req_fifo_read == false)) { fifo_req.simple_put(req_fifo_data); } 
	if((req_fifo_write == true) && (req_fifo_read == true))  { fifo_req.put_and_get(req_fifo_data); } 
	if((req_fifo_write == false) && (req_fifo_read == true)) { fifo_req.simple_get(); }

	// DSPIN response to VCI response
	// The DSPIN packet is stored in the fifo_rsp
	// The FIFO output is analysed and translated to a VCI packet

	// rsp_fifo_write, rsp_fifo_data
	rsp_fifo_write = p_dspin_in.write.read();
	rsp_fifo_data  = p_dspin_in.data.read();

	// r_fsm_state_rsp, BUF_RPKTID, rsp_fifo_read
	switch(r_fsm_state_rsp) {
	    case RSP_HEADER :
		rsp_fifo_read = true;
		if(fifo_rsp.rok() == true) {
		    r_srcid = (uint32_t)((fifo_rsp.read() >>8     ) & srcid_mask);
		    r_pktid = (uint32_t)((fifo_rsp.read() >> 18) & 0x03);
		    r_trdid = (uint32_t)((fifo_rsp.read() >> 20) & 0x0f);
		    r_fsm_state_rsp = RSP_DATA;
		}
		break;
	    case RSP_DATA :
		rsp_fifo_read = p_vci.rspack;
		if((fifo_rsp.rok() == true) && (p_vci.rspack == true)) {
		    if(((fifo_rsp.read() >> 32) & DSPIN_EOP) == DSPIN_EOP) { r_fsm_state_rsp = RSP_HEADER; }
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
	    case REQ_HEADER :
		p_vci.cmdack = false;
		break;
	    case REQ_ADDRESS_READ :
		p_vci.cmdack = fifo_req.wok();
		break;
	    case REQ_ADDRESS_WRITE :
		p_vci.cmdack = false;
		break;
	    case REQ_DATA_WRITE :
		p_vci.cmdack = fifo_req.wok();
		break;
	} // end switch VCI_r_fsm_state_req

	// VCI RSP interface

	switch(r_fsm_state_rsp) {
	    case RSP_HEADER :
		p_vci.rspval = false;
		p_vci.rdata = (sc_uint<vci_param::N>) 0;
		p_vci.rpktid = (sc_uint<vci_param::P>) 0;
		p_vci.rtrdid = (sc_uint<vci_param::T>) 0;
		p_vci.rsrcid = (sc_uint<vci_param::S>) 0;
		p_vci.rerror = (sc_uint<vci_param::E>) 0;
		p_vci.reop   = false;
		break;
	    case RSP_DATA :
		p_vci.rspval = fifo_rsp.rok();
		p_vci.rdata = (sc_uint<vci_param::N>) (fifo_rsp.read() & 0xffffffff);
		p_vci.rpktid = (sc_uint<vci_param::P>)r_pktid;
		p_vci.rtrdid = (sc_uint<vci_param::T>)r_trdid;
		p_vci.rsrcid = (sc_uint<vci_param::S>)r_srcid;
		if(((fifo_rsp.read() >> 32) & DSPIN_ERR) == DSPIN_ERR){ p_vci.rerror = 1; }
		else                                                  { p_vci.rerror = 0; }
		if(((fifo_rsp.read() >> 32) & DSPIN_EOP) == DSPIN_EOP) {p_vci.reop = true;}
		else                                                   {p_vci.reop = false;}
		break;
	} // end switch VCI_r_fsm_state_rsp

	// DSPIN_OUT interface

	p_dspin_out.write = fifo_req.rok();
	p_dspin_out.data = fifo_req.read();

	// DSPIN_IN interface

	p_dspin_in.read = fifo_rsp.wok();

    }; // end genMoore

}} // end namespace

