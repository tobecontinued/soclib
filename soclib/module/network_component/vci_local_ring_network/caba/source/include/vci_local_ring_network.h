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
#ifndef VCI_LOCAL_RING_H_
#define VCI_LOCAL_RING_H_

#include <systemc>
#include "caba_base_module.h"
#include "vci_ring_target_wrapper.h"
#include "vci_ring_initiator_wrapper.h"
#include "ring_gateway.h"
#include "mapping_table.h"
#include "gate_ports.h"


namespace soclib { namespace caba {

    using namespace sc_core;

    template<typename vci_param>
        class VciLocalRingNetwork
        : public soclib::caba::BaseModule
        {
            public:
                sc_in<bool>             p_clk;
                sc_in<bool>             p_resetn;

                soclib::caba::VciInitiator<vci_param>* p_to_target;
                soclib::caba::VciTarget<vci_param>   * p_to_initiator;

                // Gate interface
		soclib::caba::GateInitiator             p_gate_initiator;
		soclib::caba::GateTarget                p_gate_target;

                //ring
                soclib::caba::VciRingInitiatorWrapper<vci_param>** t_initiator_wrapper;
                soclib::caba::VciRingTargetWrapper<vci_param>   ** t_target_wrapper;
                soclib::caba::RingGateway                        * t_ring_gateway; 
               
            protected:
                SC_HAS_PROCESS(VciLocalRingNetwork);

            private:                
                int m_ns;   // network number of signals
                int m_nai; // number of attached initiators 
                int m_nat; // number of attached targets
                soclib::caba::RingSignals * t_ring_signal;                                                

            public:
                VciLocalRingNetwork( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,   
                         const int &wrapper_fifo_depth,
                         const int &half_gateway_fifo_depth,
                         int nb_attached_initiator,
                         int nb_attached_target);
                                                                   
               ~VciLocalRingNetwork();
        };
}} // end namespace

#endif //VCI_LOCAL_RING_H_
