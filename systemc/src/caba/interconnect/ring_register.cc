/////////////////////////////////////////////////////////////////////
//  File: vci_pi_initiator_wrapper.cc  
//  Author: Alain Greiner 
//  Date: 15/04/2007 
//  Copyright : UPMC - LIP6
//  This program is released under the GNU General Public License 
/////////////////////////////////////////////////////////////////////

#include "caba/interconnect/ring_register.h"
#include "common/register.h"

namespace soclib { namespace caba {

#define Ring soclib::caba::Ring

#define tmpl(x) template<typename vci_param> x RingRegister<vci_param>

////////////////////////////////
//	constructor
////////////////////////////////

tmpl(/**/)::RingRegister(sc_module_name insname)
  : soclib::caba::BaseModule(insname)
{

SC_METHOD (transition);
dont_initialize();
sensitive << p_clk.pos();

} //  end constructor

////////////////////////////////
//	transition 
////////////////////////////////
tmpl(void)::transition()       
{
	if(p_resetn == false) { 
		p_rno.ring_neg_cmd = NEG_EMPTY;
		p_rno.ring_neg_srcid = 0;
		p_rno.ring_neg_msblsb = 0;
		
		p_rdo.ring_data_cmd = DATA_EMPTY;          
		p_rdo.ring_data_eop = false;          
		p_rdo.ring_data_be  = 0;           
		p_rdo.ring_data_srcid = 0;
		p_rdo.ring_data_pktid = 0;        
		p_rdo.ring_data_adresse = 0;      
		p_rdo.ring_data = 0;              
		p_rdo.ring_data_error = 0;

		// add for new version
//	p_rdo.ring_data_plen  = 0; 
//	p_rdo.ring_data_trdid = 0;
//	p_rdo.ring_data_cons  = false;
//	p_rdo.ring_data_wrap  = false;
//	p_rdo.ring_data_contig= false;
//	p_rdo.ring_data_clen  = 0;
//	p_rdo.ring_data_cfixed= false;

		
	return;
	} 

    //output of the neg anneau  
    p_rno.ring_neg_cmd = p_rni.ring_neg_cmd.read();
    p_rno.ring_neg_srcid = p_rni.ring_neg_srcid.read();
    p_rno.ring_neg_msblsb = p_rni.ring_neg_msblsb.read();

    //output of the data anneau
    p_rdo.ring_data_cmd = p_rdi.ring_data_cmd.read();          
    p_rdo.ring_data_eop = p_rdi.ring_data_eop.read();          
    p_rdo.ring_data_be  = p_rdi.ring_data_be.read();           
    p_rdo.ring_data_srcid = p_rdi.ring_data_srcid.read(); 
    p_rdo.ring_data_pktid = p_rdi.ring_data_pktid.read();       
    p_rdo.ring_data_adresse = p_rdi.ring_data_adresse.read();      
    p_rdo.ring_data = p_rdi.ring_data.read();              
    p_rdo.ring_data_error = p_rdi.ring_data_error.read();

    // add for new version
//   p_rdo.ring_data_plen  = p_rdi.ring_data_plen.read();                 
//   p_rdo.ring_data_trdid = p_rdi.ring_data_trdid.read();                 
//   p_rdo.ring_data_cons  = p_rdi.ring_data_cons.read();                 
//   p_rdo.ring_data_wrap  = p_rdi.ring_data_wrap.read();                
//   p_rdo.ring_data_contig= p_rdi.ring_data_contig.read();
//   p_rdo.ring_data_clen  = p_rdi.ring_data_clen.read();
//   p_rdo.ring_data_cfixed= p_rdi.ring_data_cfixed.read();

};  // end Transition()

}} // end namespace
