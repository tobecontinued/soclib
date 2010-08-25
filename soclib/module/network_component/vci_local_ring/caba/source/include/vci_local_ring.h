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
#ifndef VCI_LOCAL_RING_H_
#define VCI_LOCAL_RING_H_

#include <systemc>
#include "caba_base_module.h"
#include "vci_ring_target_wrapper.h"
#include "vci_ring_initiator_wrapper.h"
#include "ring_dspin_half_gateway_initiator.h"
#include "ring_dspin_half_gateway_target.h"
#include "mapping_table.h"
#include "dspin_interface.h"
#include "vci_initiator.h"
#include "vci_target.h"

namespace soclib { namespace caba {

template<typename vci_param, int cmd_width, int rsp_width>
class VciLocalRing : public soclib::caba::BaseModule
{
public:
    // ports
    sc_core::sc_in<bool>             					p_clk;
    sc_core::sc_in<bool>             					p_resetn;
    soclib::caba::VciInitiator<vci_param>* 				p_to_target;
    soclib::caba::VciTarget<vci_param>*					p_to_initiator;
    soclib::caba::DspinOutput<cmd_width>				p_gate_cmd_out;
    soclib::caba::DspinInput<rsp_width>					p_gate_rsp_in;
    soclib::caba::DspinInput<cmd_width>      				p_gate_cmd_in;
    soclib::caba::DspinOutput<rsp_width>     				p_gate_rsp_out;

    // instanciated components & signals
    soclib::caba::VciRingInitiatorWrapper<vci_param>** 			m_initiator_wrapper;
    soclib::caba::VciRingTargetWrapper<vci_param>** 			m_target_wrapper;
    soclib::caba::RingDspinHalfGatewayInitiator<cmd_width,rsp_width>* 	m_init_gateway; 
    soclib::caba::RingDspinHalfGatewayTarget<cmd_width,rsp_width>* 	m_tgt_gateway; 
    soclib::caba::RingSignals * 					m_ring_signals;  
               
protected:
    SC_HAS_PROCESS(VciLocalRing);

private:                
    int 						m_nai; // number of attached initiators 
    int 						m_nat; // number of attached targets

public:
    VciLocalRing( sc_module_name 			insname,
                  const soclib::common::MappingTable 	&mt,
                  const soclib::common::IntTab 		&ringid,   
                  const int 				&wrapper_fifo_depth,
                  const int 				&gateway_fifo_depth,
                  int 					nb_attached_initiator,
                  int 					nb_attached_target);
                                                                   
    ~VciLocalRing();
};

}} // end namespace

#endif //VCI_LOCAL_RING_H_
