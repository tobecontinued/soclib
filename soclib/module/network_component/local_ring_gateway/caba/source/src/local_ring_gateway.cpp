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
#include "../include/local_ring_gateway.h"

namespace soclib { namespace caba {

// constructor
LocalRingGateway::LocalRingGateway( sc_module_name insname,
                         const soclib::common::MappingTable &mt,
                         const soclib::common::IntTab &ringid,
                         bool  alloc_init,
                         bool  alloc_target,
                         const int &bridge_fifo_depth)
                        : soclib::caba::BaseModule(insname) 
  {  
      
	local_binitiator= new soclib::caba::LocalBridgeInitiator("local_binitiator", alloc_init, bridge_fifo_depth, mt, ringid);
	local_btarget=    new soclib::caba::LocalBridgeTarget("local_btarget", alloc_target, bridge_fifo_depth, mt, ringid);
	
	local_binitiator->p_clk(p_clk);
	local_binitiator->p_resetn(p_resetn);
	local_binitiator->p_ring_in(p_ring_in);
	local_binitiator->p_ring_out(t_ring_signal);
	local_binitiator->p_rsp_fifo(p_out_rsp_fifo);  
	local_binitiator->p_cmd_fifo(p_in_cmd_fifo);
	         
	local_btarget->p_clk(p_clk);
	local_btarget->p_resetn(p_resetn);
	local_btarget->p_ring_in(t_ring_signal);
	local_btarget->p_ring_out(p_ring_out);                 
	local_btarget->p_rsp_fifo(p_in_rsp_fifo);
	local_btarget->p_cmd_fifo(p_out_cmd_fifo);              
 }

// destructor
LocalRingGateway::~LocalRingGateway()
{

	delete local_btarget;
	delete local_binitiator;
    
}

   
}} // end namespace
