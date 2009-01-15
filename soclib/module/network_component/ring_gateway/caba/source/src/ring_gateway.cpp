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
#include "../include/ring_gateway.h"

namespace soclib { namespace caba {

// constructor
RingGateway::RingGateway( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,
                         bool  alloc_init,
                         bool  alloc_target,
                         bool  local,
                         const int &half_gateway_fifo_depth)
                        : soclib::caba::BaseModule(insname) 
  {  
      
	hg_initiator= new soclib::caba::HalfGatewayInitiator("hg_initiator", alloc_init, half_gateway_fifo_depth, mt, ringid, local);
	hg_target=    new soclib::caba::HalfGatewayTarget("hg_target", alloc_target, half_gateway_fifo_depth, mt, ringid, local);
	
	hg_initiator->p_clk(p_clk);
	hg_initiator->p_resetn(p_resetn);
	hg_initiator->p_ring_in(p_ring_in);
	hg_initiator->p_ring_out(t_ring_signal);
	hg_initiator->p_rsp_fifo(p_out_rsp_fifo);  
	hg_initiator->p_cmd_fifo(p_in_cmd_fifo);
	         
	hg_target->p_clk(p_clk);
	hg_target->p_resetn(p_resetn);
	hg_target->p_ring_in(t_ring_signal);
	hg_target->p_ring_out(p_ring_out);                 
	hg_target->p_rsp_fifo(p_in_rsp_fifo);
	hg_target->p_cmd_fifo(p_out_cmd_fifo);              
 }

// destructor
RingGateway::~RingGateway()
{

	delete hg_target;
	delete hg_initiator;
    
}

   
}} // end namespace
