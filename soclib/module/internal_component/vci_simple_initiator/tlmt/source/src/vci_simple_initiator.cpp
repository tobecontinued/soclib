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
                cmd.cmd = vci_param::CMD_READ;
                //addresses[0] = 0xBFC00000;
                //cmd.address = addresses;
                cmd.address = 0xBFC00000;
                cmd.be = 0xF;
                cmd.contig = true;
                cmd.buf = localbuf;
                cmd.length = 1;
                cmd.srcid = 0;
                cmd.trdid = 0;
                cmd.pktid = 0;

		tlmt_core::tlmt_return ret;
		ret = p_vci.send(&cmd, c0.time());
		std::cout << name() << "ret.time=" << ret.time() << std::endl;
		sc_core::wait(e0);

		std::cout << std::hex << localbuf[0] << std::endl;
		std::cout << std::dec;

		localbuf[0]++;

		std::cout << std::hex << localbuf[0] << std::endl;
		std::cout << std::dec;

                cmd.cmd = vci_param::CMD_WRITE;
                //addresses[0] = 0xBFC00000;
                //cmd.address = addresses;
                cmd.address = 0xBFC00000;
                cmd.be = 0xF;
                cmd.contig = true;
                cmd.buf = localbuf;
                cmd.length = 1;
                cmd.srcid = 0;
                cmd.trdid = 0;
                cmd.pktid = 0;

                ret = p_vci.send(&cmd, c0.time());
                std::cout << name() << "ret.time=" << ret.time() << std::endl;
                sc_core::wait(e0);

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
