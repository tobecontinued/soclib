/* -*- c++ -*-
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
 * Authors  : Franck WAJSBÜRT, Abdelmalek SI MERABET 
 * Date     : December 2008
 * Copyright: UPMC - LIP6
 */

#include <iostream> 
#include <stdarg.h>
#include "alloc_elems.h"
#include "../include/vci_local_ring_network.h"

namespace soclib { namespace caba {

using soclib::common::alloc_elems;
using soclib::common::dealloc_elems;

#define tmpl(x) template<typename vci_param> x VciLocalRingNetwork<vci_param>

////////////////////////////////
//      constructor
//
////////////////////////////////
tmpl(/**/)::VciLocalRingNetwork( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,
                         const int &wrapper_fifo_depth,
                         const int &half_gateway_fifo_depth,
                         int nb_attached_initiator,
                         int nb_attached_target)                                             
                         : soclib::caba::BaseModule(insname), 
                           m_ns(nb_attached_initiator+nb_attached_target+1), 
                           m_nai(nb_attached_initiator),  
                           m_nat(nb_attached_target)
 {
        
	t_initiator_wrapper = new soclib::caba::VciRingInitiatorWrapper<vci_param>*[m_nai];
	t_target_wrapper    = new soclib::caba::VciRingTargetWrapper<vci_param>*[m_nat];        
	p_to_initiator =  soclib::common::alloc_elems<soclib::caba::VciTarget<vci_param> >("p_to_initiator", m_nai);
	p_to_target = soclib::common::alloc_elems<soclib::caba::VciInitiator<vci_param> >("p_to_target", m_nat);
	t_ring_signal = soclib::common::alloc_elems<soclib::caba::RingSignals>("t_ring_signal", m_ns);
	
	// generation wrappers initiateurs  
	for( int x = 0; x < m_nai ; x++ )
	{
		bool alloc_init = (x==0);             
		std::ostringstream o;
		o << name() << "_i_wrapper_" << x;
		t_initiator_wrapper[x] = new soclib::caba::VciRingInitiatorWrapper<vci_param>(o.str().c_str(), 
                                                                                          alloc_init, 
                                                                                          wrapper_fifo_depth,   
                                                                                          mt, 
                                                                                          ringid, 
                                                                                          x);
	}
    
        // generation wrappers targets
	for( int x = 0; x < m_nat ; x++ ) 
	{
		bool alloc_target = (x==0); 
		std::ostringstream o;
		o << name() << "_t_wrapper_" << x;
		t_target_wrapper[x] = new soclib::caba::VciRingTargetWrapper<vci_param>(o.str().c_str(), 
                                                                                    alloc_target,                                                                                                          wrapper_fifo_depth,   
                                                                                    mt, 
                                                                                    ringid, 
                                                                                    x);
	}

         
        // generation gateway
        t_ring_gateway = new soclib::caba::RingGateway("t_ring_gateway", mt, ringid, false, false, false, half_gateway_fifo_depth); 

        // NETLIST
 	int nbsig = 0;
                                          
	// connexion initiateurs
	for( int x = 0; x < m_nai ; x++ )
        {
 		t_initiator_wrapper[x]->p_clk(p_clk);
 		t_initiator_wrapper[x]->p_resetn(p_resetn);
 		t_initiator_wrapper[x]->p_ring_in(t_ring_signal[nbsig]);
 		t_initiator_wrapper[x]->p_ring_out(t_ring_signal[++nbsig]);
 		t_initiator_wrapper[x]->p_vci(p_to_initiator[x]);                    
         }       
                       
       // connexion targets              
       for( int x = 0 ; x < m_nat ; x++ )
       {
 		t_target_wrapper[x]->p_clk(p_clk);
 		t_target_wrapper[x]->p_resetn(p_resetn);
	 	t_target_wrapper[x]->p_vci(p_to_target[x]) ; 
 		t_target_wrapper[x]->p_ring_in(t_ring_signal[nbsig]);
 		t_target_wrapper[x]->p_ring_out(t_ring_signal[++nbsig]);                 
       }

      // connecting gateway with last target and first init
	t_ring_gateway->p_clk(p_clk);
	t_ring_gateway->p_resetn(p_resetn);       
	t_ring_gateway->p_ring_in(t_ring_signal[nbsig]);
	t_ring_gateway->p_ring_out(t_ring_signal[0]);    
	t_ring_gateway->p_out_rsp_fifo(p_out_rsp_fifo);    
	t_ring_gateway->p_in_rsp_fifo(p_in_rsp_fifo); 
	t_ring_gateway->p_out_cmd_fifo(p_out_cmd_fifo);    
	t_ring_gateway->p_in_cmd_fifo(p_in_cmd_fifo);
 }
   
//---------------- destructor
tmpl(/**/)::~VciLocalRingNetwork()
    {

	delete t_ring_gateway;
	
	for(int x = 0; x < m_nai; x++)
		delete t_initiator_wrapper[x];
	
	for(int x = 0; x < m_nat; x++)
		delete t_target_wrapper[x];
	        
	delete [] t_initiator_wrapper;
	delete [] t_target_wrapper;
	
	dealloc_elems(p_to_initiator,m_nai);
	dealloc_elems(p_to_target, m_nat);
	dealloc_elems(t_ring_signal, m_ns);       
    }
}} // end namespace
