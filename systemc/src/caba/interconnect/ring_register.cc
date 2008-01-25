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
		p_ro.ring_neg_cmd = NEG_EMPTY;
		p_ro.ring_neg_srcid = 0;
		p_ro.ring_neg_msblsb = 0;
		
		p_ro.ring_data_cmd = DATA_EMPTY;          
		p_ro.ring_data_eop = false;          
		p_ro.ring_data_be  = 0;           
		p_ro.ring_data_srcid = 0;
		p_ro.ring_data_pktid = 0;        
		p_ro.ring_data_adresse = 0;      
		p_ro.ring_data = 0;              
		p_ro.ring_data_error = 0;
		
		return;
	} 

    //output of the neg anneau  
    p_ro.ring_neg_cmd = p_ri.ring_neg_cmd.read();
    p_ro.ring_neg_srcid = p_ri.ring_neg_srcid.read();
    p_ro.ring_neg_msblsb = p_ri.ring_neg_msblsb.read();

    //output of the data anneau
    p_ro.ring_data_cmd = p_ri.ring_data_cmd.read();          
    p_ro.ring_data_eop = p_ri.ring_data_eop.read();          
    p_ro.ring_data_be  = p_ri.ring_data_be.read();           
    p_ro.ring_data_srcid = p_ri.ring_data_srcid.read(); 
    p_ro.ring_data_pktid = p_ri.ring_data_pktid.read();       
    p_ro.ring_data_adresse = p_ri.ring_data_adresse.read();      
    p_ro.ring_data = p_ri.ring_data.read();              
    p_ro.ring_data_error = p_ri.ring_data_error.read();

}  // end Transition()

}} // end namespace
