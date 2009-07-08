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
 * Date     : january 2009
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
	std::ostringstream o;
	o << name() << "_hg_initiator";
	hg_initiator= new soclib::caba::HalfGatewayInitiator(o.str().c_str(), alloc_init, half_gateway_fifo_depth, mt, ringid, local);
	std::ostringstream p;
	p << name() << "_hg_target";
	hg_target=    new soclib::caba::HalfGatewayTarget(p.str().c_str(), alloc_target, half_gateway_fifo_depth, mt, ringid, local);
	
	hg_initiator->p_clk(p_clk);
	hg_initiator->p_resetn(p_resetn);
	hg_initiator->p_ring_in(p_ring_in);
	hg_initiator->p_ring_out(t_ring_signal);
	hg_initiator->p_gate_initiator(p_gate_initiator);  
		         
	hg_target->p_clk(p_clk);
	hg_target->p_resetn(p_resetn);
	hg_target->p_ring_in(t_ring_signal);
	hg_target->p_ring_out(p_ring_out);                 
	hg_target->p_gate_target(p_gate_target);        
 }

// destructor
RingGateway::~RingGateway()
{

	delete hg_target;
	delete hg_initiator;
    
}

   
}} // end namespace
