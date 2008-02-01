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
 *     François Pêcheux <fancois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 */

#include "../include/vci_simple_initiator.h"

namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciSimpleInitiator<vci_param>

tmpl(tlmt_core::tlmt_return&)::callback(soclib::tlmt::vci_rsp_packet<vci_param> *pkt,
										const tlmt_core::tlmt_time &time,
										void *private_data)
{
	std::cout << name() << " callback" << std::endl;
	e0.notify(sc_core::SC_ZERO_TIME);
	c0.set_time(time);
	std::cout << " c0.time=" << c0.time() << std::endl;
	std::cout << " Fini !" << std::endl;
	return m_return;
}

tmpl(void)::behavior()
{
	soclib::tlmt::vci_cmd_packet<vci_param> cmd;
	uint32_t addresses[32];
	uint32_t localbuf[32];

	for(;;) {
		cmd.length = 4;
		addresses[0] = 0xBFC00000;
		addresses[1] = 0xBFC00004;
		addresses[2] = 0xBFC00008;
		addresses[3] = 0xBFC0000c;
		cmd.address = addresses;
		cmd.buf = localbuf;
		cmd.cmd = vci_param::CMD_READ;
		cmd.srcid = 0;

		tlmt_core::tlmt_return ret;
		ret = p_vci.send(&cmd, c0.time());
		std::cout << name() << "ret.time=" << ret.time() << std::endl;
		sc_core::wait(e0);

		std::cout << std::hex << localbuf[0] << std::endl;
		std::cout << std::hex << localbuf[1] << std::endl;
		std::cout << std::hex << localbuf[2] << std::endl;
		std::cout << std::hex << localbuf[3] << std::endl;
		std::cout << std::dec;
	}
}

tmpl(/**/)::VciSimpleInitiator( sc_core::sc_module_name name )
		   : soclib::tlmt::BaseModule(name),
		   p_vci("vci", new tlmt_core::tlmt_callback<VciSimpleInitiator,soclib::tlmt::vci_rsp_packet<vci_param> *>(
					 this, &VciSimpleInitiator<vci_param>::callback), &c0)
{
	SC_THREAD(behavior);
}

}}
