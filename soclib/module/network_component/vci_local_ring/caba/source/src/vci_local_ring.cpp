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
 * Authors  : Franck WAJSBÜRT, Abdelmalek SI MERABET, Alain GREINER
 * Date     : July 2010
 * Copyright: UPMC - LIP6
 */

#include <iostream> 
#include <stdarg.h>
#include "alloc_elems.h"
#include "../include/vci_local_ring.h"

namespace soclib { namespace caba {

using namespace soclib::common;
using namespace soclib::caba;

#define tmpl(x) template<typename vci_param, int cmd_width, int rsp_width> \
        x VciLocalRing<vci_param, cmd_width, rsp_width>

////////////////////////////////////////////////////////
tmpl(/**/)::VciLocalRing( sc_module_name 	insname,
                         const MappingTable 	&mt,
                         const IntTab 		&ringid,
                         const int 		&wrapper_fifo_depth,
                         const int 		&gateway_fifo_depth,
                         int 			nb_attached_initiator,
                         int 			nb_attached_target)                                             
    : BaseModule(insname), 
      m_nai(nb_attached_initiator),  
      m_nat(nb_attached_target)
    {
        m_initiator_wrapper = new VciRingInitiatorWrapper<vci_param>*[m_nai];
	m_target_wrapper    = new VciRingTargetWrapper<vci_param>*[m_nat];        
	p_to_initiator      = alloc_elems<VciTarget<vci_param> >("p_to_initiator", m_nai);
	p_to_target         = alloc_elems<VciInitiator<vci_param> >("p_to_target", m_nat);
	m_ring_signals      = alloc_elems<RingSignals>("m_ring_signals", m_nai + m_nat + 2);

	std::ostringstream o;

	// initiator wrappers generation  
	for( int x = 0; x < m_nai ; x++ )
	{
            o << name() << "_i_wrapper_" << x;
            m_initiator_wrapper[x] = new VciRingInitiatorWrapper<vci_param>(o.str().c_str(), 
                                                                            	false, 
                                                                            	wrapper_fifo_depth,   
                                                                            	mt, 
                                                                            	ringid, 
                                                                            	x);
        }
        // target wrappers generation 
        for( int x = 0; x < m_nat ; x++ ) 
        {
            o << name() << "_t_wrapper_" << x;
            m_target_wrapper[x] = new VciRingTargetWrapper<vci_param>(o.str().c_str(), 
                                                                      		false,
                                                                      		wrapper_fifo_depth,   
                                                                      		mt, 
                                                                      		ringid, 
                                                                      		x);
        }
        // initiator half gateway generation
	o << name() << "_i_gateway_";
        m_init_gateway = new RingDspinHalfGatewayInitiator<cmd_width, rsp_width>(o.str().c_str(), 
										true,
                                                                    		gateway_fifo_depth, 
                                                                    		mt, 
                                                                    		ringid, 
                                                                    		false);
        // target half gateway generation
	o << name() << "_t_gateway_";
        m_tgt_gateway = new RingDspinHalfGatewayTarget<cmd_width, rsp_width>(o.str().c_str(), 
										true,
                                                                    		gateway_fifo_depth, 
                                                                    		mt, 
                                                                    		ringid, 
                                                                    		false);

        // NETLIST
        int nbsig = 0;
        // connecting initiator wrappers
	for( int x = 0; x < m_nai ; x++ )
        {
             m_initiator_wrapper[x]->p_clk		(p_clk);
             m_initiator_wrapper[x]->p_resetn		(p_resetn);
             m_initiator_wrapper[x]->p_ring_in		(m_ring_signals[nbsig]);
             m_initiator_wrapper[x]->p_ring_out		(m_ring_signals[++nbsig]);
             m_initiator_wrapper[x]->p_vci		(p_to_initiator[x]);                    
        }       
        // connecting target wrappers              
        for( int x = 0 ; x < m_nat ; x++ )
        {
            m_target_wrapper[x]->p_clk			(p_clk);
            m_target_wrapper[x]->p_resetn		(p_resetn);
            m_target_wrapper[x]->p_vci			(p_to_target[x]) ; 
            m_target_wrapper[x]->p_ring_in		(m_ring_signals[nbsig]);
            m_target_wrapper[x]->p_ring_out		(m_ring_signals[++nbsig]);                 
        }
        // connecting initiator half gateway 
	m_init_gateway->p_clk				(p_clk);
	m_init_gateway->p_resetn			(p_resetn);       
	m_init_gateway->p_ring_in			(m_ring_signals[nbsig]);
	m_init_gateway->p_ring_out			(m_ring_signals[++nbsig]);    
	m_init_gateway->p_gate_cmd_in			(p_gate_cmd_in);    
	m_init_gateway->p_gate_rsp_out			(p_gate_rsp_out);    

        // connecting target half gateway 
	m_tgt_gateway->p_clk				(p_clk);
	m_tgt_gateway->p_resetn				(p_resetn);       
	m_tgt_gateway->p_ring_in			(m_ring_signals[nbsig]);
	m_tgt_gateway->p_ring_out			(m_ring_signals[0]);    
	m_tgt_gateway->p_gate_cmd_out			(p_gate_cmd_out);    
	m_tgt_gateway->p_gate_rsp_in 			(p_gate_rsp_in);    

    }
   
    ///////////////////////////
    tmpl(/**/)::~VciLocalRing()
    {
	delete m_init_gateway;
	delete m_tgt_gateway;
	
	for(int x = 0; x < m_nai; x++)
		delete m_initiator_wrapper[x];
	
	for(int x = 0; x < m_nat; x++)
		delete m_target_wrapper[x];
	        
	delete [] m_initiator_wrapper;
	delete [] m_target_wrapper;
	
	dealloc_elems(p_to_initiator,m_nai);
	dealloc_elems(p_to_target, m_nat);
	dealloc_elems(m_ring_signals, m_nai + m_nat + 2);
    }

}} // end namespace
