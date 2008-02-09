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
#ifndef SOCLIB_TLMT_VCI_RAM_H
#define SOCLIB_TLMT_VCI_RAM_H

#include <tlmt>
#include "vci_ports.h"
#include "tlmt_base_module.h"
#include "mapping_table.h"
#include "soclib_endian.h"
#include "elf_loader.h"

namespace soclib { namespace tlmt {

template<typename vci_param>
class VciRam 
    : public soclib::tlmt::BaseModule
{
private:
	soclib::common::IntTab m_index;
	soclib::common::MappingTable m_mt;
	soclib::common::ElfLoader *m_loader;

	std::list<soclib::common::Segment> m_segments;
	typedef typename vci_param::data_t ram_t;
	ram_t **m_contents;
	tlmt_core::tlmt_return m_return;
	vci_rsp_packet<vci_param> rsp;

protected:
    SC_HAS_PROCESS(VciRam);
public:
    soclib::tlmt::VciTargetPort<vci_param> p_vci;

    VciRam(
		sc_core::sc_module_name name,
		const soclib::common::IntTab &index,
		const soclib::common::MappingTable &mt,
		common::ElfLoader &loader);

    tlmt_core::tlmt_return &callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
									 const tlmt_core::tlmt_time &time,
									 void *private_data);

    tlmt_core::tlmt_return &callback_read(size_t segIndex,soclib::common::Segment &s,
										  soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
										  const tlmt_core::tlmt_time &time,
										  void *private_data);

    tlmt_core::tlmt_return &callback_write(size_t segIndex,soclib::common::Segment &s,
										   soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
										   const tlmt_core::tlmt_time &time,
										   void *private_data);

};

}}

#endif
