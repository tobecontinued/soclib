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

#include <iostream>
#include <stdarg.h>
#include <systemc.h>
#include "caba/interconnect/vci_ring_target_wrapper.h"
#include "common/register.h"

namespace soclib { namespace caba {

#define Ring soclib::caba::Ring
#define tmpl(x) template<typename vci_param> x VciRingTargetWrapper<vci_param>

////////////////////////////////
//	constructor
////////////////////////////////

tmpl(/**/)::VciRingTargetWrapper(
    sc_module_name insname,
    int nseg,
    int base_adr,
    ...)
  : soclib::caba::BaseModule(insname),
    s_nseg(nseg)
{

SC_METHOD (transition);
dont_initialize();
sensitive << p_clk.pos();

SC_METHOD(Decoder);
dont_initialize();
sensitive << p_ri.ring_neg_cmd
          << p_ri.ring_neg_msblsb
          << p_vci.cmdack
          << p_ri.ring_data_cmd
          << p_ri.ring_data_adresse
          << r_cible_reserve;

SC_METHOD(Mux);
dont_initialize();
sensitive << p_ri.ring_data_cmd
	  << r_fsm_state 
	  << r_cible_ack_ok
	  << r_cible_ext
	  << p_vci.rspval
	  << p_vci.reop;

SC_METHOD (genMoore);
dont_initialize();
sensitive << p_clk.neg();

SC_METHOD (genMealy);
dont_initialize();
sensitive << p_clk.neg()
          << p_ri.ring_data_cmd
          << p_ri.ring_data_adresse
          << p_ri.ring_data_be
          << p_ri.ring_data
          << p_ri.ring_data_eop
          << p_ri.ring_data_srcid
          << p_ri.ring_data_pktid
          << p_vci.rspval
          << r_cible_ext;

SC_METHOD(ANI_Output);
dont_initialize();
sensitive << r_ring_data_cmd_p
          << p_ri.ring_neg_cmd
          << r_ring_neg_mux
          << p_ri.ring_neg_srcid
          << p_ri.ring_neg_msblsb
          << r_ring_data_mux
          << p_vci.reop
          << p_vci.rsrcid
          << p_vci.rpktid
          << p_vci.rdata
          << p_vci.rerror
          << p_ri.ring_data_eop
          << p_ri.ring_data_be
          << p_ri.ring_data_srcid
          << p_ri.ring_data_pktid
          << p_ri.ring_data_adresse
          << p_ri.ring_data
          << p_ri.ring_data_error;

CIBLE_ID = (int *)malloc(s_nseg*(sizeof(int)));

va_list listeBaseAdr;		
int tmpBaseAdr;			

va_start (listeBaseAdr, base_adr);

tmpBaseAdr = base_adr ;

for (int i=0; i<s_nseg; i++)
{
    CIBLE_ID[i] = tmpBaseAdr;
    tmpBaseAdr = va_arg (listeBaseAdr,int);
} // fin de creation de tous les address bases des segments 

va_end (listeBaseAdr);

} //  end constructor

/////////////////////////////////////////////
// 	Decoder       
/////////////////////////////////////////////
tmpl(void)::Decoder()
{
	r_cible_ack_ok = false;
	if(((p_ri.ring_neg_cmd.read()>= NEG_REQ_READ)&&(p_ri.ring_neg_cmd.read()<= NEG_REQ_STORE_COND))
  	&&(r_cible_reserve == false)
  	&&(p_vci.cmdack.read() == true)){
        
        	for(int i=0;i<s_nseg;i++)
        	{
            		if ((int)p_ri.ring_neg_msblsb.read() == CIBLE_ID[i])
            		{
				r_cible_ack_ok = true;
				break;
	    		}
		}  
	}

	bool id_match = false;
	for(int i=0;i<s_nseg;i++)
	{
		if ((int)((p_ri.ring_data_adresse.read()&0xFF000000)>>24) == CIBLE_ID[i])
		{
			id_match = true;
			break;
		}
	}  

	if(((p_ri.ring_data_cmd.read()>= DATA_REQ_READ)&&(p_ri.ring_data_cmd.read()<= DATA_REQ_STORE_COND))
	&&(id_match == true)
	&&(p_vci.cmdack.read() == true)){
		r_cible_ext = true;  
	}else{
		r_cible_ext = false;  	
	}

} 

/////////////////////////////////////////////
// 	neg and data anneau mux       
/////////////////////////////////////////////
tmpl(void)::Mux()
{
	r_ring_data_cmd_p = p_ri.ring_data_cmd.read();	
	r_ring_data_mux = RING;
	r_trans_end = false;

	switch(r_fsm_state) {
	    case TAR_IDLE :
		if(r_cible_ack_ok == true){
			r_ring_neg_mux = LOCAL;
		}else{
			r_ring_neg_mux = RING;
		}
	    	break;

	    case TAR_RESERVE_FIRST :
		r_ring_neg_mux = RING;
		if(r_cible_ext == true){
	    		r_ring_data_cmd_p = DATA_EMPTY;
	    	}
	    	break;	
	
	    case TAR_RESERVE :
		r_ring_neg_mux = RING;
		if((r_cible_ext == true)
		&&(p_vci.rspval.read() == true)){
	    		r_ring_data_cmd_p = DATA_RES;
	    		r_ring_data_mux = LOCAL;
	    	}	
	    	if((p_vci.rspval.read() == true)
		  &&(p_vci.reop.read() == true)
		  &&(p_ri.ring_data_cmd.read() == DATA_EMPTY)){
	    		r_ring_data_mux = LOCAL;
	    		r_ring_data_cmd_p = DATA_RES;
			r_trans_end = true;
	    	}		
	    	break;
	} // end switch 

}

////////////////////////////////
//	transition 
////////////////////////////////
tmpl(void)::transition()       
{
	if(p_resetn == false) { 
		r_fsm_state = TAR_IDLE;
		return;
	} // end reset

	switch(r_fsm_state) {
		case TAR_IDLE :
			if(r_cible_ack_ok == true) {
				r_fsm_state = TAR_RESERVE_FIRST;
			}else{
				r_fsm_state = TAR_IDLE;
			}
			break;

		case TAR_RESERVE_FIRST :	//the first word of request
			if(r_cible_ext == true) {
				r_fsm_state = TAR_RESERVE;
			}else{
				r_fsm_state = TAR_RESERVE_FIRST;
			}
			break;	
	
		case TAR_RESERVE :
			if(r_trans_end == true){
				r_fsm_state = TAR_IDLE; 
			}else {
				r_fsm_state = TAR_RESERVE;
			}
			break;
		} // end switch TAR_FSM
}  // end Transition()

/////////////////////////////////////////////
// 	GenMoore()     
/////////////////////////////////////////////
tmpl(void)::genMoore()
{
	switch(r_fsm_state) {
		case TAR_IDLE :
			r_cible_reserve = false;
			break;
		case TAR_RESERVE_FIRST :
			r_cible_reserve = true;
			break;
		case TAR_RESERVE :
			r_cible_reserve = true;
			break;
	} // end switch

} // end GenMoore

/////////////////////////////////////////////
// 	GenMealy()       
/////////////////////////////////////////////
tmpl(void)::genMealy()
{
	p_vci.cmdval = false;

	switch(r_fsm_state) {
	    case TAR_IDLE :
	    	break;

	    case TAR_RESERVE_FIRST :
		if(r_cible_ext == true){
	    		p_vci.cmdval = true;	
	    		p_vci.address = p_ri.ring_data_adresse.read();	
	    		p_vci.be = p_ri.ring_data_be.read();     	
	    		p_vci.cmd = p_ri.ring_data_cmd.read();   	
	    		p_vci.wdata = p_ri.ring_data.read();  	
	    		p_vci.eop = p_ri.ring_data_eop.read();    	
	    		p_vci.srcid = p_ri.ring_data_srcid.read();  
			p_vci.pktid = p_ri.ring_data_pktid.read();
	    	}
	    	break;	
	
	    case TAR_RESERVE :
		if((r_cible_ext == true)
		&&(p_vci.rspval.read() == true)){
	    		p_vci.cmdval = true;	
	    		p_vci.address = p_ri.ring_data_adresse.read();	
	    		p_vci.be = p_ri.ring_data_be.read();     	
	    		p_vci.cmd = p_ri.ring_data_cmd.read();   	
	    		p_vci.wdata = p_ri.ring_data.read();  	
	    		p_vci.eop = p_ri.ring_data_eop.read();    	
	    		p_vci.srcid = p_ri.ring_data_srcid.read(); 
 			p_vci.pktid = p_ri.ring_data_pktid.read();
	    	}	
	    	break;
	} // end switch 
} // end GenMealy

/////////////////////////////////////////////
// 	ANI_Output()       
/////////////////////////////////////////////
tmpl(void)::ANI_Output()
{
	
	/* ANNEAU NEG OUTPUT MUX */
	if(r_ring_neg_mux == LOCAL){
		p_ro.ring_neg_cmd  = NEG_ACK;
		p_ro.ring_neg_srcid = p_ri.ring_neg_srcid.read();
		p_ro.ring_neg_msblsb = p_ri.ring_neg_msblsb.read();
	
	}else {			//RING
		p_ro.ring_neg_cmd  = p_ri.ring_neg_cmd.read(); 
		p_ro.ring_neg_srcid = p_ri.ring_neg_srcid.read();
		p_ro.ring_neg_msblsb = p_ri.ring_neg_msblsb.read();	
	}		

	/* ANNEAU DATA OUTPUT MUX */		
	if(r_ring_data_mux == LOCAL){
		p_ro.ring_data_cmd = r_ring_data_cmd_p;
		p_ro.ring_data_eop = p_vci.reop.read();   
		p_ro.ring_data_be = 0;    
		p_ro.ring_data_srcid = p_vci.rsrcid.read();  
		p_ro.ring_data_pktid = p_vci.rpktid.read(); 
		p_ro.ring_data_adresse = 0x00000000;
		p_ro.ring_data = p_vci.rdata.read();        
		p_ro.ring_data_error = p_vci.rerror.read(); 
		p_vci.rspack = true;
 

	}else {			//RING
		p_ro.ring_data_cmd = r_ring_data_cmd_p;
		p_ro.ring_data_eop = p_ri.ring_data_eop.read();   
		p_ro.ring_data_be = p_ri.ring_data_be.read();    
		p_ro.ring_data_srcid = p_ri.ring_data_srcid.read();  
		p_ro.ring_data_pktid = p_ri.ring_data_pktid.read(); 
		p_ro.ring_data_adresse = p_ri.ring_data_adresse.read();
		p_ro.ring_data = p_ri.ring_data.read();        
		p_ro.ring_data_error = p_ri.ring_data_error.read();  
		p_vci.rspack = false;
	}	
} // end ANI_Output

}} // end namespace
