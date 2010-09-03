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
 * Author   : Abdelmalek SI MERABET 
 * Date     : March 2010
 * Copyright: UPMC - LIP6
 */

#include <iostream> 
#include <stdarg.h>
#include "alloc_elems.h"
#include "../include/vci_simple_ring_fast.h"

namespace soclib { namespace caba {

using soclib::common::alloc_elems;
using soclib::common::dealloc_elems;

#define tmpl(x) template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size > x VciSimpleRingFast<vci_param, ring_cmd_data_size, ring_rsp_data_size >

////////////////////////////////
//      constructor
//
////////////////////////////////
tmpl(/**/)::VciSimpleRingFast( 
        sc_module_name insname,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &ringid,
        const int &wrapper_fifo_depth,
        size_t nb_attached_initiator,
        size_t nb_attached_target)                                             
                : soclib::caba::BaseModule(insname),
                p_clk("clk"),
                p_resetn("resetn"), 
                m_ns(nb_attached_initiator+nb_attached_target), 
                m_nai(nb_attached_initiator),  
                m_nat(nb_attached_target)
{
	p_to_initiator = soclib::common::alloc_elems<soclib::caba::VciTarget<vci_param> >("p_to_initiator", m_nai);
	p_to_target    = soclib::common::alloc_elems<soclib::caba::VciInitiator<vci_param> >("p_to_target", m_nat);

	m_ring_initiator = new ring_initiator_t*[m_nai];
	m_ring_target    = new ring_target_t*[m_nat]; 
        m_ring_signal    = new ring_signal_t[m_ns];

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

tmpl(void)::transition()
{


        if ( ! p_resetn.read() ) {
                for(size_t i=0;i<m_nai;i++) 
                        m_ring_initiator[i]->reset();
                for(size_t t=0;t<m_nat;t++)
                        m_ring_target[t]->reset();
                return;
        }

// update ring signals Three iterations
// this is intended in order to break the loop due to dependency existing between ring signals
// this rule is based based on relaxation principle

//- 1st 
       for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i]);
        }

        for(size_t i=0;i<m_nat;i++){
                m_ring_target[i]->update_ring_signals(m_ring_signal[m_nai+i-1], m_ring_signal[m_nai+i] );
        }

//- 2nd
       for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i]);
        }

        for(size_t i=0;i<m_nat;i++){
                m_ring_target[i]->update_ring_signals(m_ring_signal[m_nai+i-1], m_ring_signal[m_nai+i] );
        }

//- 3rd 
       for(size_t i=0;i<m_nai;i++) {
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->update_ring_signals(m_ring_signal[h], m_ring_signal[i]);
        }

        for(size_t i=0;i<m_nat;i++){
                m_ring_target[i]->update_ring_signals(m_ring_signal[m_nai+i-1], m_ring_signal[m_nai+i] );
        }

// transition
        for(size_t i=0;i<m_nai;i++) {
//              std::cout << sc_time_stamp() << " -- " << name() << " *** vci_initiator : " << i << std::endl;
                size_t h = 0;
                if(i == 0) h = m_ns-1;
                else h = i-1;
                m_ring_initiator[i]->transition(p_to_initiator[i], m_ring_signal[h]);
        }
        for(size_t t=0;t<m_nat;t++) {
//              std::cout << sc_time_stamp() << " -- " << name() << " *** vci_target : " << t << std::endl;
                m_ring_target[t]->transition(p_to_target[t], m_ring_signal[m_nai+t-1]);
        }


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

}

tmpl(void)::print_vci_init(size_t i)
{
        std::cout << sc_time_stamp() << " -- " << name() << " *** vci_initiator : " << i << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_cmdval  : " <<             p_to_initiator[i].cmdval.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_cmdack  : " <<             p_to_initiator[i].cmdack.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_address : " << std::hex << p_to_initiator[i].address.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_plen    : " <<             p_to_initiator[i].plen.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_wdata   : " <<             p_to_initiator[i].wdata.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_cmd     : " <<             p_to_initiator[i].cmd.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rspval  : " <<             p_to_initiator[i].rspval.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rspack  : " <<             p_to_initiator[i].rspack.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rdata   : " << std::hex << p_to_initiator[i].rdata.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rsrcid  : " <<             p_to_initiator[i].rsrcid.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_reop    : " <<             p_to_initiator[i].reop.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rerror  : " <<             p_to_initiator[i].rerror.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << "*******************************" << std::endl;
}
tmpl(void)::print_vci_target(size_t i)
{
        std::cout << sc_time_stamp() << " -- " << name() << " *** vci_target : " << i << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_cmdval  : " <<             p_to_target[i].cmdval.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_cmdack  : " <<             p_to_target[i].cmdack.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_address : " << std::hex << p_to_target[i].address.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_plen    : " <<             p_to_target[i].plen.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_wdata   : " <<             p_to_target[i].wdata.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_cmd     : " <<             p_to_target[i].cmd.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rspval  : " <<             p_to_target[i].rspval.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rspack  : " <<             p_to_target[i].rspack.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rdata   : " << std::hex << p_to_target[i].rdata.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rsrcid  : " <<             p_to_target[i].rsrcid.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_reop    : " <<             p_to_target[i].reop.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " vci_rerror  : " <<             p_to_target[i].rerror.read() << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << "*******************************" << std::endl;
}
tmpl(void)::print_signal(size_t i)
{
        std::cout << sc_time_stamp() << " -- " << name() << " ++++ signals index : " << i << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " cmd_grant : " << m_ring_signal[i].cmd_grant << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " cmd_data  : " << std::hex << m_ring_signal[i].cmd_data << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " cmd_rok/w : " << m_ring_signal[i].cmd_w << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " cmd_r/wok : " << m_ring_signal[i].cmd_r << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " rsp_grant : " << m_ring_signal[i].rsp_grant << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " rsp_data  : " << m_ring_signal[i].rsp_data  << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " rsp_rok/w : " << m_ring_signal[i].rsp_w     << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " rsp_wok/r : " << m_ring_signal[i].rsp_r     << std::endl;
        std::cout << sc_time_stamp() << " -- " << name() << " ++++++++++++++++++++++++++++++++++++" << std::endl;
} 
//---------------- destructor
tmpl(/**/)::~VciSimpleRingFast()
{

	for(size_t x = 0; x < m_nai; x++)
		delete m_ring_initiator[x];
	
	for(size_t x = 0; x < m_nat; x++)
		delete m_ring_target[x];

//for(size_t x = 0; x < m_ns; x++)
//	delete m_ring_signal[x];

	delete [] m_ring_initiator;
	delete [] m_ring_target;
        delete [] m_ring_signal;
	
	dealloc_elems(p_to_initiator,m_nai);
	dealloc_elems(p_to_target, m_nat);
}
}} // end namespace
