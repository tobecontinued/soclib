/////////////////////////////////////////////////////////////////////
// File     : vci_ring_initiator_wrapper.h
// Author   : Yang GAO 
// Date     : 28/09/2007
// Copyright: UPMC - LIP6
// This program is released under the GNU Public License 
//
// This component is a initiator wrapper for generic VCI anneau-ani. 
/////////////////////////////////////////////////////////////////////

#include "caba/interconnect/vci_ring_initiator_wrapper.h"
#include "common/register.h"

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
sensitive << p_rni.ring_neg_cmd
          << p_rni.ring_neg_srcid
	  << p_vci.srcid
          << p_rdi.ring_data_cmd
          << p_rdi.ring_data_srcid
          << p_rdi.ring_data_eop
          << p_vci.rspack;

SC_METHOD (genMealy);
dont_initialize();
sensitive << ring_data_cmd_p
          << p_rni.ring_neg_cmd
          << p_vci.cmdval
          << p_vci.cmd
          << neg_ack_ok
          << p_clk.neg();

SC_METHOD(ANI_Output);
dont_initialize();
sensitive << p_rdi.ring_data_cmd
          << p_rdi.ring_data_srcid
          << p_vci.srcid
          << p_vci.rspack
          << p_rdi.ring_data
          << p_rdi.ring_data_eop
          << p_rdi.ring_data_error
          << p_rdi.ring_data_pktid
          << ring_neg_mux
          << ring_data_mux
          << p_vci.address
	  << p_rni.ring_neg_srcid
          << p_rni.ring_neg_msblsb
          << p_vci.eop
          << p_vci.be
          << p_vci.wdata
          << p_rdi.ring_data_be
          << p_rdi.ring_data_adresse
          << p_vci.pktid;

} //  end constructor

/////////////////////////////////////////////
// 	Decoder       
/////////////////////////////////////////////
tmpl(void)::Decoder()
{
	if((p_rni.ring_neg_cmd.read() == NEG_ACK) && (p_rni.ring_neg_srcid.read() == p_vci.srcid.read())) 
	{ 
		neg_ack_ok = true; 
	} else{
		neg_ack_ok = false; 
	}

	if((p_rdi.ring_data_cmd.read() == DATA_RES) && (p_rdi.ring_data_srcid.read() == p_vci.srcid.read())
     	&&(p_rdi.ring_data_eop.read() == true) && (p_vci.rspack.read() == true)) 
	{ 
		data_eop = true; 
	} else{
		data_eop = false; 
	}

};

////////////////////////////////
//	transition 
////////////////////////////////
tmpl(void)::transition()       
{
	if(p_resetn == false) { 
		r_fsm_state = NEG_IDLE;
		neg_ack_ok = false;
		data_eop = false; 
		return;
	} 

	switch(r_fsm_state) {
	case NEG_IDLE :
        	if((p_rni.ring_neg_cmd.read() == NEG_EMPTY)&&(p_vci.cmdval.read() == true)) {
        		r_fsm_state = NEG_ACK_WAIT;
        	}else {
        		r_fsm_state = NEG_IDLE;
        	} 
	break;
	case NEG_ACK_WAIT :
		if(neg_ack_ok == true) { 
			r_fsm_state = NEG_DATA; 
        	}else {
        		r_fsm_state = NEG_ACK_WAIT;
        	} 
	break;
	case NEG_DATA :
        	if(data_eop == true) {
        		r_fsm_state = NEG_IDLE;
        	}
        	else {
        		r_fsm_state = NEG_DATA;
        	} 
	break;   
	} // end switch INIT_FSM
};  // end Transition()

/////////////////////////////////////////////
// 	GenMealy()       
/////////////////////////////////////////////
tmpl(void)::genMealy()
{
	ring_neg_mux = RING;
	ring_data_mux = RING;
	p_rdo.ring_data_cmd = ring_data_cmd_p;
	p_vci.cmdack = false;

	switch(r_fsm_state) {
	case NEG_IDLE :
        	if((p_rni.ring_neg_cmd.read() == NEG_EMPTY)&&(p_vci.cmdval.read() == true)) {
        		ring_neg_mux = LOCAL;

                        if(p_vci.cmd.read() == VciParams<4,1,32,1,1,1,8,1,1,1>::CMD_READ){
				p_rno.ring_neg_cmd = NEG_REQ_READ;   //read request 
			}else{
				p_rno.ring_neg_cmd = NEG_REQ_WRITE;  //write request
			}  
        	}else{
			p_rno.ring_neg_cmd = p_rni.ring_neg_cmd.read();
		}
	break;
	case NEG_ACK_WAIT :
		if(neg_ack_ok == true){p_rno.ring_neg_cmd = NEG_EMPTY; }

	break;
	case NEG_DATA :
		p_rno.ring_neg_cmd = p_rni.ring_neg_cmd.read();
		if(ring_data_cmd_p.read() == DATA_EMPTY){
			p_vci.cmdack = true;
			if(p_vci.cmdval.read() == true){
				ring_data_mux = LOCAL;
                        	if(p_vci.cmd.read() == VciParams<4,1,32,1,1,1,8,1,1,1>::CMD_READ){
					p_rdo.ring_data_cmd = DATA_REQ_READ;
				}else{
					p_rdo.ring_data_cmd = DATA_REQ_WRITE;
				}
			}
		}
	break;	
	} // end switch 

}; // end GenMealy

/////////////////////////////////////////////
// 	ANI_Output()       
/////////////////////////////////////////////
tmpl(void)::ANI_Output()
{
	if((p_rdi.ring_data_cmd.read() == DATA_RES)&&(p_rdi.ring_data_srcid.read() == p_vci.srcid.read())
   		&&(p_vci.rspack.read() == true)) {
		ring_data_cmd_p = DATA_EMPTY;
	}else {
		ring_data_cmd_p = p_rdi.ring_data_cmd.read();
	}

	/* VCI OUTPUT */
	if((p_rdi.ring_data_cmd.read() == DATA_RES)&&(p_rdi.ring_data_srcid.read() == p_vci.srcid.read())) {
		p_vci.rspval = true;
		p_vci.rdata =  p_rdi.ring_data.read();
		p_vci.reop = p_rdi.ring_data_eop.read();
		p_vci.rerror = p_rdi.ring_data_error.read();
		p_vci.rsrcid = p_rdi.ring_data_srcid.read();
		p_vci.rpktid = p_rdi.ring_data_pktid.read();
	
	}else {
		p_vci.rspval = false;
		p_vci.rdata = 0x00000000;
		p_vci.reop = false;
		p_vci.rerror = 0;
		p_vci.rsrcid = 0x00;
		p_vci.rpktid = 0;

	}

	/* ANNEAU NEG OUTPUT MUX */
	if(ring_neg_mux == LOCAL){
		p_rno.ring_neg_srcid = p_vci.srcid.read();
		p_rno.ring_neg_msblsb = (p_vci.address.read() & 0xFF000000) >> 24;
	}else {			//RING
		p_rno.ring_neg_srcid = p_rni.ring_neg_srcid.read();
		p_rno.ring_neg_msblsb = p_rni.ring_neg_msblsb.read();	
	}		

	/* ANNEAU DATA OUTPUT MUX */		
	if(ring_data_mux == LOCAL){
		p_rdo.ring_data_eop = p_vci.eop.read();   
		p_rdo.ring_data_be = p_vci.be.read();    
		p_rdo.ring_data_srcid = p_vci.srcid.read(); 
		p_rdo.ring_data_pktid = p_vci.pktid.read(); 
		p_rdo.ring_data_adresse = p_vci.address.read();
		p_rdo.ring_data = p_vci.wdata.read();        
		p_rdo.ring_data_error = false;//VCI.RERROR.read();  
	}else {			//RING
		p_rdo.ring_data_eop = p_rdi.ring_data_eop.read();   
		p_rdo.ring_data_be = p_rdi.ring_data_be.read();    
		p_rdo.ring_data_srcid = p_rdi.ring_data_srcid.read();  
		p_rdo.ring_data_pktid = p_rdi.ring_data_pktid.read(); 
		p_rdo.ring_data_adresse = p_rdi.ring_data_adresse.read();
		p_rdo.ring_data = p_rdi.ring_data.read();        
		p_rdo.ring_data_error = p_rdi.ring_data_error.read(); 

	}

}; // end ANI_Output

}} // end namespace
