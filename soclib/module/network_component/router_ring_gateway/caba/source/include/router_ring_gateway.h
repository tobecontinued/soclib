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

#ifndef ROUTER_RING_GATEWAY_H_
#define ROUTER_RING_GATEWAY_H_

#include <systemc>
#include "caba_base_module.h"
#include "bridge_target.h"
#include "bridge_initiator.h"
#include "ring_ports.h"
#include "mapping_table.h"
#include "fifo_ports.h"


namespace soclib { namespace caba {

    using namespace sc_core;

    class RouterRingGateway : public soclib::caba::BaseModule
        {
	public:
		sc_in<bool>                             p_clk;
		sc_in<bool>                             p_resetn;
		
		soclib::caba::RingIn                    p_ring_in;
		soclib::caba::RingOut                   p_ring_out;
		
		soclib::caba::FifoOutput<sc_uint<33> >  p_out_rsp_fifo;
		soclib::caba::FifoInput<sc_uint<37> >   p_in_cmd_fifo;
		
		soclib::caba::FifoInput<sc_uint<33> >   p_in_rsp_fifo;
		soclib::caba::FifoOutput<sc_uint<37> >  p_out_cmd_fifo;
		
		//ring
		soclib::caba::BridgeInitiator *b_initiator;
		soclib::caba::BridgeTarget    *b_target;

                
	protected:
		SC_HAS_PROCESS(RouterRingGateway);

	private:             
             
                                
		// ring signals
		soclib::caba::RingSignals t_ring_signal;
                                                

	public:
		RouterRingGateway(sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,   
                         bool  alloc_init,
                         bool  alloc_target,
                         const int &bridge_fifo_depth);  
                                          
		~RouterRingGateway();
        };
}} // end

#endif //ROUTER_RING_GATEWAY_H_
