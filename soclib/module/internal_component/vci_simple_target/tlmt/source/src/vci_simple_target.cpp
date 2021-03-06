/* -*- mode: c++; coding: utf-8 -*-
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
 * Maintainers: fpecheux, nipo
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 */

#include "../include/vci_simple_target.h"

namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciSimpleTarget<vci_param>

tmpl(void)::callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
		     const tlmt_core::tlmt_time &time,
		     void *private_data)
{
	soclib::tlmt::vci_rsp_packet<vci_param> rsp;

	std::cout << "SimpleTarget " << name() << " callback" << std::endl;
	p_vci.send(&rsp, time+tlmt_core::tlmt_time(42));

	std::cout << " Demande de temps: " << p_vci.peer_time() << std::endl;
	std::cout << " Demande d'activite: " << p_vci.peer_active() << std::endl;
}

tmpl(/**/)::VciSimpleTarget(sc_core::sc_module_name name)
		   : soclib::tlmt::BaseModule(name),
		   p_vci("vci", new tlmt_core::tlmt_callback<VciSimpleTarget,soclib::tlmt::vci_cmd_packet<vci_param> *>(
				   this, &VciSimpleTarget<vci_param>::callback))
{
}

}}
