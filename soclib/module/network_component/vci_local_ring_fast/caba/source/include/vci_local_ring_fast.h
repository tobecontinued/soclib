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
#ifndef VCI_LOCAL_RING_FAST_H_
#define VCI_LOCAL_RING_FAST_H_

#include <systemc>
#include "caba_base_module.h"
#include "mapping_table.h"
#include "generic_fifo.h"
#include "address_decoding_table.h"
#include "address_masking_table.h"
#include "ring_signals_2.h"
#include "vci_ring_initiator.h"
#include "vci_ring_target.h"
#include "ring_dspin_half_gateway_initiator_fast.h"
#include "ring_dspin_half_gateway_target_fast.h"
#include "dspin_interface.h"

namespace soclib { namespace caba {

    using namespace sc_core;

    template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size>
        class VciLocalRingFast
        : public soclib::caba::BaseModule
        {
            public:
                sc_in<bool>             p_clk;
                sc_in<bool>             p_resetn;

                soclib::caba::VciInitiator<vci_param>* p_to_target;
                soclib::caba::VciTarget<vci_param>   * p_to_initiator;

                // Gate interface
		//soclib::caba::GateInitiator2<ring_cmd_data_size, ring_rsp_data_size> p_gate_initiator;
		//soclib::caba::GateTarget2<ring_cmd_data_size, ring_rsp_data_size>    p_gate_target;

		soclib::caba::DspinOutput<ring_cmd_data_size>	p_gate_cmd_out;
		soclib::caba::DspinInput<ring_rsp_data_size>    p_gate_rsp_in;
		soclib::caba::DspinInput<ring_cmd_data_size>    p_gate_cmd_in;
		soclib::caba::DspinOutput<ring_rsp_data_size>   p_gate_rsp_out;
               
        protected:
                SC_HAS_PROCESS(VciLocalRingFast);

        private:                
                int m_ns;   // network number of signals
                int m_nai; // number of attached initiators 
                int m_nat; // number of attached targets
                                         
                typedef RingSignals2 ring_signal_t;
                typedef VciRingInitiator<vci_param, ring_cmd_data_size, ring_rsp_data_size> ring_initiator_t;
                typedef VciRingTarget<vci_param, ring_cmd_data_size, ring_rsp_data_size>    ring_target_t;
                typedef RingDspinHalfGatewayInitiatorFast<vci_param, ring_cmd_data_size, ring_rsp_data_size> half_gateway_initiator_t;
                typedef RingDspinHalfGatewayTargetFast<vci_param, ring_cmd_data_size, ring_rsp_data_size> half_gateway_target_t;
 
                void transition();
                void genMoore();  
     
                ring_signal_t    *m_ring_signal; 
                ring_initiator_t **m_ring_initiator;
                ring_target_t    **m_ring_target;
                
                half_gateway_initiator_t *m_half_gateway_initiator;
                half_gateway_target_t    *m_half_gateway_target;

        public:
                VciLocalRingFast( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,   
                         const int &wrapper_fifo_depth,
                         const int &half_gateway_fifo_depth,
                         int nb_attached_initiator,
                         int nb_attached_target);
                                                                   
               ~VciLocalRingFast();
        };
}} // end namespace

#endif //VCI_LOCAL_RING_FAST_H_
