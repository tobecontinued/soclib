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
 * Authors  : Abdelmalek SI MERABET 
 * Date     : March 2010 
 * Copyright: UPMC - LIP6
 */

#include <iostream> 
#include <string>
#include <stdarg.h>
#include "alloc_elems.h"
#include "../include/vci_local_ring_fast.h"

namespace soclib { namespace caba {

using soclib::common::alloc_elems;
using soclib::common::dealloc_elems;

#define tmpl(x) template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size > x VciLocalRingFast<vci_param, ring_cmd_data_size, ring_rsp_data_size >

////////////////////////////////
//      constructor           //
////////////////////////////////
tmpl(/**/)::VciLocalRingFast( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,
                         const int &wrapper_fifo_depth,
                         const int &half_gateway_fifo_depth,
                         int nb_attached_initiator,
                         int nb_attached_target)                                             
                         : soclib::caba::BaseModule(insname), 
                           m_ns(nb_attached_initiator+nb_attached_target+2), 
                           m_nai(nb_attached_initiator),  
                           m_nat(nb_attached_target)
 {
        

        
	p_to_initiator =  soclib::common::alloc_elems<soclib::caba::VciTarget<vci_param> >("p_to_initiator", m_nai);
	p_to_target = soclib::common::alloc_elems<soclib::caba::VciInitiator<vci_param> >("p_to_target", m_nat);

        m_ring_initiator = new ring_initiator_t*[m_nai];
	m_ring_target    = new ring_target_t*[m_nat]; 
        m_ring_signal    = new ring_signal_t[m_ns];


	std::ostringstream o;
	o << name() << "_hg_init";
        bool alloc_hg_init = (m_nai == 0);
        m_half_gateway_initiator = new half_gateway_initiator_t(o.str().c_str(), alloc_hg_init, half_gateway_fifo_depth, mt, ringid, false);

	std::ostringstream p;
	p << name() << "_hg_target";
	bool alloc_hg_target = (m_nat == 0);
        m_half_gateway_target    = new half_gateway_target_t(p.str().c_str(), alloc_hg_target, half_gateway_fifo_depth, mt, ringid, false);

        for(size_t i=0; i<m_nai; ++i) {
                bool alloc_init = (i==0);
		std::ostringstream o;
		o << name() << "_init_" << i;
                m_ring_initiator[i] = new ring_initiator_t(o.str().c_str(), alloc_init, wrapper_fifo_depth, mt, ringid, i);
        }

        for(size_t i=0; i<m_nat; ++i) {
                bool alloc_target = (i==0);
		std::ostringstream o;
		o << name() << "_target_" << i;
                m_ring_target[i]  = new ring_target_t(o.str().c_str(), alloc_target, wrapper_fifo_depth, mt, ringid, i);
        }

        SC_METHOD(transition);
        dont_initialize();
        sensitive << p_clk.pos();

        SC_METHOD(genMoore);
        dont_initialize();
        sensitive << p_clk.neg();
 }
//--------------------------//
tmpl(void)::transition()    //
//--------------------------//
{


        if ( ! p_resetn.read() ) {
                for(size_t i=0;i<m_nai;i++) 
                        m_ring_initiator[i]->reset();
                for(size_t t=0;t<m_nat;t++)
                        m_ring_target[t]->reset();

                m_half_gateway_initiator->reset();
                m_half_gateway_target->reset();
                return;
        }

// update ring signals three times
// this is intended in order to break the loop due to dependency existing between ring signals
// this rule is based on relaxation principle

//-*--------------- 1st iteration 
       for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i]);
        }

if(m_nai > 0) {
        for(size_t i=0;i<m_nat;i++){
                m_ring_target[i]->update_ring_signals(m_ring_signal[m_nai+i-1], m_ring_signal[m_nai+i] );
        }
 }
else {

        for(size_t i=0;i<m_nat;i++){
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_target[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i] );
        }
}

        m_half_gateway_initiator->update_ring_signals(m_ring_signal[m_nai+m_nat-1], m_ring_signal[m_nai+m_nat] );

        m_half_gateway_target->update_ring_signals(m_ring_signal[m_ns-2], m_ring_signal[m_ns-1] );

//-*--------------- 2nd iteration
       for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i]);
        }

if(m_nai > 0) {
        for(size_t i=0;i<m_nat;i++){
                m_ring_target[i]->update_ring_signals(m_ring_signal[m_nai+i-1], m_ring_signal[m_nai+i] );
        }
 }
else {

        for(size_t i=0;i<m_nat;i++){
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_target[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i] );
        }
}


        m_half_gateway_initiator->update_ring_signals(m_ring_signal[m_nai+m_nat-1], m_ring_signal[m_nai+m_nat] );

        m_half_gateway_target->update_ring_signals(m_ring_signal[m_ns-2], m_ring_signal[m_ns-1] );


//-*--------------- 3rd iteration 
       for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i]);
        }

if(m_nai > 0) {
        for(size_t i=0;i<m_nat;i++){
                m_ring_target[i]->update_ring_signals(m_ring_signal[m_nai+i-1], m_ring_signal[m_nai+i] );
        }
 }
else {

        for(size_t i=0;i<m_nat;i++){
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_target[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i] );
        }
}


        m_half_gateway_initiator->update_ring_signals(m_ring_signal[m_nai+m_nat-1], m_ring_signal[m_nai+m_nat] );

        m_half_gateway_target->update_ring_signals(m_ring_signal[m_ns-2], m_ring_signal[m_ns-1] );

//-----------------------------------------------//
// transition                                    //
//----------------------------------------------//

        for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->transition(p_to_initiator[i], m_ring_signal[h]);
        }

if(m_nai > 0) {
        for(size_t t=0;t<m_nat;t++) {
                m_ring_target[t]->transition(p_to_target[t], m_ring_signal[m_nai+t-1]);
        }
}
else {

        for(size_t t=0;t<m_nat;t++) {
		size_t h = 0;
                if(t == 0) h = m_ns-1;
                else h = t-1;

                m_ring_target[t]->transition(p_to_target[t], m_ring_signal[h]);
        }
}
//      m_half_gateway_initiator->transition(p_gate_initiator, m_ring_signal[m_nai+m_nat-1]);
        m_half_gateway_initiator->transition(p_gate_cmd_in, p_gate_rsp_out, m_ring_signal[m_nai+m_nat-1]);
//      m_half_gateway_target->transition(p_gate_target, m_ring_signal[m_nai+m_nat]);
        m_half_gateway_target->transition(p_gate_cmd_out, p_gate_rsp_in, m_ring_signal[m_nai+m_nat]);


/*-------- print
        for(size_t t=0;t<m_nat;t++) {
                print_signal(m_nai+t-1);
                print_vci_target(t);
        }

        for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                print_signal(h);
                print_vci_init(i);
        }
*/
}

tmpl(void)::genMoore()
{

        for(size_t i=0;i<m_nai;i++) 
                m_ring_initiator[i]->genMoore(p_to_initiator[i]);
        for(size_t t=0;t<m_nat;t++)
                m_ring_target[t]->genMoore(p_to_target[t]);    

        m_half_gateway_initiator->genMoore(p_gate_cmd_in, p_gate_rsp_out);
   
        m_half_gateway_target->genMoore(p_gate_cmd_out, p_gate_rsp_in);
}   
//---------------- destructor
tmpl(/**/)::~VciLocalRingFast()
    {

        delete m_half_gateway_initiator;
        delete m_half_gateway_target;

	for(size_t x = 0; x < m_nai; x++)
		delete m_ring_initiator[x];
	
	for(size_t x = 0; x < m_nat; x++)
		delete m_ring_target[x];

	delete [] m_ring_initiator;
	delete [] m_ring_target;
        delete [] m_ring_signal;
	
	dealloc_elems(p_to_initiator,m_nai);
	dealloc_elems(p_to_target, m_nat);;       
    }
}} // end namespace
