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

#include "../include/vci_ring_initiator_wrapper.h"
#include "register.h"

namespace soclib { namespace caba {

#define Ring soclib::caba::Ring

#define tmpl(x) template<typename vci_param> x VciRingInitiatorWrapper<vci_param>

////////////////////////////////
//	constructor
////////////////////////////////

tmpl(/**/)::VciRingInitiatorWrapper(sc_module_name insname)
  : soclib::caba::BaseModule(insname)
{

SC_METHOD (transition);
dont_initialize();
sensitive << p_clk.pos();

SC_METHOD(Decoder);
dont_initialize();
sensitive << p_ri.ring_neg_cmd
          << p_ri.ring_neg_srcid
	  << p_vci.srcid
          << p_ri.ring_data_cmd
          << p_ri.ring_data_srcid
          << p_ri.ring_data_eop
          << p_vci.rspack
	  << p_ri.ring_data_num
	  << r_counter_res;

SC_METHOD(Req_Counter);
dont_initialize();
sensitive << p_clk.pos();

SC_METHOD(Res_Counter);
dont_initialize();
sensitive << p_clk.pos();

SC_METHOD(Mux);
dont_initialize();
sensitive << r_fsm_state
          << p_ri.ring_neg_cmd
	  << p_vci.cmdval
          << r_ring_data_cmd_p
	  << p_vci.cmdval;

SC_METHOD (genMealy);
dont_initialize();
sensitive << r_ring_data_cmd_p
          << p_ri.ring_neg_cmd
          << p_vci.cmdval
          << p_vci.cmd
          << r_neg_ack_ok
          << p_clk.neg();

SC_METHOD(ANI_Output);
dont_initialize();
sensitive << p_ri.ring_data_cmd
          << p_ri.ring_data_srcid
          << p_vci.srcid
          << p_vci.rspack
          << p_ri.ring_data
          << p_ri.ring_data_num
          << p_ri.ring_data_eop
          << p_ri.ring_data_error
          << p_ri.ring_data_pktid
          << r_ring_neg_mux
          << r_ring_data_mux
          << p_vci.address
	  << p_ri.ring_neg_srcid
          << p_ri.ring_neg_msblsb
          << p_vci.eop
          << p_vci.be
          << p_vci.wdata
          << p_ri.ring_data_be
          << p_ri.ring_data_adresse
          << p_vci.pktid;

} //  end constructor

/////////////////////////////////////////////
//	counter
/////////////////////////////////////////////
tmpl(void)::Req_Counter()
{
	sc_uint<4> tmp_counter = r_counter_req;
	if (r_fsm_state != NEG_DATA) {
		r_counter_req = 0;
	}

	if (r_ring_data_mux == LOCAL)
	{
		tmp_counter++;
		r_counter_req = tmp_counter;
	}
}

tmpl(void)::Res_Counter()
{
	sc_uint<4> tmp_counter = r_counter_res;

	if (r_fsm_state != NEG_DATA)
		r_counter_res = 0;

	else if((p_ri.ring_data_cmd.read() == DATA_RES)&&(p_ri.ring_data_srcid.read() == p_vci.srcid.read())
   	 &&(p_vci.rspack.read() == true)&&(p_ri.ring_data_num.read() == r_counter_res)) {
		tmp_counter++;
		r_counter_res = tmp_counter;	
	}

}

/////////////////////////////////////////////
// 	Decoder       
/////////////////////////////////////////////
tmpl(void)::Decoder()
{
	if((p_ri.ring_neg_cmd.read() == NEG_ACK) && (p_ri.ring_neg_srcid.read() == p_vci.srcid.read())) 
	{ 
		r_neg_ack_ok = true; 
	} else{
		r_neg_ack_ok = false; 
	}

	if((p_ri.ring_data_cmd.read() == DATA_RES) && (p_ri.ring_data_srcid.read() == p_vci.srcid.read())
     	&&(p_ri.ring_data_eop.read() == true) && (p_vci.rspack.read() == true)
	&&(p_ri.ring_data_num.read() == r_counter_res)) 
	{ 
		r_data_eop = true; 
	} else{
		r_data_eop = false; 
	}

}

/////////////////////////////////////////////
// 	neg and data anneau mux       
/////////////////////////////////////////////
tmpl(void)::Mux()
{
	r_ring_neg_mux = RING;
	r_ring_data_mux = RING;

	if ((r_fsm_state == NEG_IDLE)&&(p_ri.ring_neg_cmd.read() == NEG_EMPTY)&&(p_vci.cmdval.read() == true)) {
        	r_ring_neg_mux = LOCAL;
	}

	if ((r_fsm_state == NEG_DATA) && (r_ring_data_cmd_p.read() == DATA_EMPTY)&&(p_vci.cmdval.read() == true)) {
		r_ring_data_mux = LOCAL;
	}

}

////////////////////////////////
//	transition 
////////////////////////////////
tmpl(void)::transition()       
{
	if(p_resetn == false) { 
		r_fsm_state = NEG_IDLE;
		return;
	} 

	switch(r_fsm_state) {
	case NEG_IDLE :
        	if((p_ri.ring_neg_cmd.read() == NEG_EMPTY)&&(p_vci.cmdval.read() == true)) {
        		r_fsm_state = NEG_ACK_WAIT;
        	}else {
        		r_fsm_state = NEG_IDLE;
        	} 
	break;
	case NEG_ACK_WAIT :
		if(r_neg_ack_ok == true) { 
			r_fsm_state = NEG_DATA; 
        	}else {
        		r_fsm_state = NEG_ACK_WAIT;
        	} 
	break;
	case NEG_DATA :
        	if(r_data_eop == true) {
        		r_fsm_state = NEG_IDLE;
        	}
        	else {
        		r_fsm_state = NEG_DATA;
        	} 
	break;   
	} // end switch INIT_FSM
}  // end Transition()

/////////////////////////////////////////////
// 	GenMealy()       
/////////////////////////////////////////////
tmpl(void)::genMealy()
{
	p_ro.ring_data_cmd = r_ring_data_cmd_p;
	p_vci.cmdack = false;

	switch(r_fsm_state) {
	case NEG_IDLE :
        	if((p_ri.ring_neg_cmd.read() == NEG_EMPTY)&&(p_vci.cmdval.read() == true)) {
			switch(p_vci.cmd.read()) {
	 	       	case vci_param::CMD_READ:
				p_ro.ring_neg_cmd = NEG_REQ_READ;	//read request 
	 	           	break;
	 	       	case vci_param::CMD_WRITE:
				p_ro.ring_neg_cmd = NEG_REQ_WRITE;  	//write request
	 	           	break;
	 	       	case vci_param::CMD_LOCKED_READ:
	 	           	p_ro.ring_neg_cmd = NEG_REQ_LOCKED_READ;//locked read request
	 	           	break;
	 	       	case vci_param::CMD_STORE_COND:
	 	           	p_ro.ring_neg_cmd = NEG_REQ_STORE_COND;	//store conditional request
	 	           	break;
	 	       	default:
	 	           	assert(0);
	 	       	}
        	}else{
			p_ro.ring_neg_cmd = p_ri.ring_neg_cmd.read();
		}
	break;
	case NEG_ACK_WAIT :
		if(r_neg_ack_ok == true){
			p_ro.ring_neg_cmd = NEG_EMPTY; 
		} else { 
			p_ro.ring_neg_cmd = p_ri.ring_neg_cmd.read();
		}
	break;
	case NEG_DATA :
		p_ro.ring_neg_cmd = p_ri.ring_neg_cmd.read();
		if(r_ring_data_cmd_p.read() == DATA_EMPTY){
			p_vci.cmdack = true;
			if(p_vci.cmdval.read() == true){
				switch(p_vci.cmd.read()) {
	 			case vci_param::CMD_READ:
					p_ro.ring_data_cmd = DATA_REQ_READ;	  //read request 
	 			   	break;
	 			case vci_param::CMD_WRITE:
					p_ro.ring_data_cmd = DATA_REQ_WRITE;  	  //write request
	 			   	break;
	 			case vci_param::CMD_LOCKED_READ:
	 			   	p_ro.ring_data_cmd = DATA_REQ_LOCKED_READ;//locked read request
	 			   	break;
	 			case vci_param::CMD_STORE_COND:
	 			   	p_ro.ring_data_cmd = DATA_REQ_STORE_COND; //store conditional request
	 			   	break;
	 			default:
	 			   	assert(0);
	 			}
			}
		}
	break;	
	} // end switch 

} // end GenMealy

/////////////////////////////////////////////
// 	ANI_Output()       
/////////////////////////////////////////////
tmpl(void)::ANI_Output()
{

	if((p_ri.ring_data_cmd.read() == DATA_RES)&&(p_ri.ring_data_srcid.read() == p_vci.srcid.read())
   		&&(p_vci.rspack.read() == true)&&(p_ri.ring_data_num.read() == r_counter_res)) {
		r_ring_data_cmd_p = DATA_EMPTY;

	} else {
		r_ring_data_cmd_p = p_ri.ring_data_cmd.read();
	}

	/* VCI OUTPUT */
	if((p_ri.ring_data_cmd.read() == DATA_RES)&&(p_ri.ring_data_srcid.read() == p_vci.srcid.read())&&(p_ri.ring_data_num.read() == r_counter_res)) {
		p_vci.rspval = true;
		p_vci.rdata =  p_ri.ring_data.read();
		p_vci.reop = p_ri.ring_data_eop.read();
		p_vci.rerror = p_ri.ring_data_error.read();
		p_vci.rsrcid = p_ri.ring_data_srcid.read();
		p_vci.rpktid = p_ri.ring_data_pktid.read();
	
	}else {
		p_vci.rspval = false;
		p_vci.rdata = 0x00000000;
		p_vci.reop = false;
		p_vci.rerror = 0;
		p_vci.rsrcid = 0x00;
		p_vci.rpktid = 0;

	}

	/* ANNEAU NEG OUTPUT MUX */
	if(r_ring_neg_mux == LOCAL){
		p_ro.ring_neg_srcid = p_vci.srcid.read();
		p_ro.ring_neg_msblsb = (p_vci.address.read() & 0xFF000000) >> 24;
	}else {			//RING
		p_ro.ring_neg_srcid = p_ri.ring_neg_srcid.read();
		p_ro.ring_neg_msblsb = p_ri.ring_neg_msblsb.read();	
	}		

	/* ANNEAU DATA OUTPUT MUX */		
	if(r_ring_data_mux == LOCAL){
		p_ro.ring_data_eop = p_vci.eop.read();   
		p_ro.ring_data_be = p_vci.be.read();    
		p_ro.ring_data_srcid = p_vci.srcid.read(); 
		p_ro.ring_data_pktid = p_vci.pktid.read(); 
		p_ro.ring_data_adresse = p_vci.address.read();
		p_ro.ring_data = p_vci.wdata.read();        
		p_ro.ring_data_error = false;//VCI.RERROR.read(); 
		p_ro.ring_data_num = r_counter_req; 
	}else {			//RING
		p_ro.ring_data_eop = p_ri.ring_data_eop.read();   
		p_ro.ring_data_be = p_ri.ring_data_be.read();    
		p_ro.ring_data_srcid = p_ri.ring_data_srcid.read();  
		p_ro.ring_data_pktid = p_ri.ring_data_pktid.read(); 
		p_ro.ring_data_adresse = p_ri.ring_data_adresse.read();
		p_ro.ring_data = p_ri.ring_data.read();        
		p_ro.ring_data_num = p_ri.ring_data_num.read();
		p_ro.ring_data_error = p_ri.ring_data_error.read(); 

	}

} // end ANI_Output

}} // end namespace
