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

#include <stdarg.h>
#include "../include/router_ring_gateway.h"

namespace soclib { namespace caba {

// constructor
RouterRingGateway::RouterRingGateway( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,
                         bool  alloc_init,
                         bool  alloc_target,
                         const int &bridge_fifo_depth)
                   : soclib::caba::BaseModule(insname) 
  {  
      
	b_initiator= new soclib::caba::BridgeInitiator("b_initiator", alloc_init, bridge_fifo_depth, mt, ringid);
	b_target= new soclib::caba::BridgeTarget("b_target", alloc_target, bridge_fifo_depth, mt, ringid);
	        
	b_initiator->p_clk(p_clk);
	b_initiator->p_resetn(p_resetn);
	b_initiator->p_ring_in(p_ring_in);
	b_initiator->p_ring_out(t_ring_signal);
	b_initiator->p_rsp_fifo(p_out_rsp_fifo);  
	b_initiator->p_cmd_fifo(p_in_cmd_fifo);
	         
	b_target->p_clk(p_clk);
	b_target->p_resetn(p_resetn);
	b_target->p_ring_in(t_ring_signal);
	b_target->p_ring_out(p_ring_out);                 
	b_target->p_rsp_fifo(p_in_rsp_fifo);
	b_target->p_cmd_fifo(p_out_cmd_fifo);              
 }

// destructor
RouterRingGateway::~RouterRingGateway()
{

	delete b_target;
	delete b_initiator;        
}

   
}} // end namespace
