/* -*- c++ -*-
 *
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
 * Maintainers: alinevieiramello@hotmail.com, alain
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr> 
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 *     Alain Greiner <alain.greiner@lip6.fr> 
 */

#ifndef __VCI_VGMN_H__ 
#define __VCI_VGMN_H__

#include <tlmdt>
#include "interconnect.h"

namespace soclib { namespace tlmdt {

////////////////////////
class VciVgmn   
////////////////////////
  : public Interconnect       
{
public:  

  VciVgmn( sc_core::sc_module_name            name,              // module name
	       const soclib::common::MappingTable &mt,               // mapping table
	       const size_t                       n_inits,           // number of initiators
	       const size_t                       n_targets,         // number of targets
	       const size_t                       min_latency,       // minimal latency
	       const size_t                       fifo_depth,        // not used in TLMDT
           const size_t                       default_tgtid = 0) // default target index
  
};

}}

#endif /* __VCI_VGMN__ */
