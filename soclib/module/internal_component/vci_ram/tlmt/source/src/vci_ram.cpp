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

#include "vci_param.h"
#include "../include/vci_ram.h"

namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciRam<vci_param>

tmpl(tlmt_core::tlmt_return&)::callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
										const tlmt_core::tlmt_time &time,
										void *private_data)
{

//std::cout << name() << " callback" << std::endl;

	// First, find the right segment using the first address of the packet
	
	std::list<soclib::common::Segment>::iterator seg;	
	bool reached = false;
	size_t seg_index;
	for (seg_index=0,seg = m_segments.begin(); 
		 seg != m_segments.end() && !reached; ++seg_index, ++seg ) {
		soclib::common::Segment &s = *seg;
		//if (!s.contains(pkt->address[0]))
		if (!s.contains(pkt->address))
			continue;
		reached=true;
		switch(pkt->cmd)
		{
		case vci_param::CMD_READ:
			return callback_read(seg_index,s,pkt,time,private_data);
		case vci_param::CMD_WRITE:
			return callback_write(seg_index,s,pkt,time,private_data);
		}
		return m_return;
	}
	if (!reached) {
		std::cout << "Address does not match any segment" << std::endl ;
	}

	return m_return;
}

tmpl(tlmt_core::tlmt_return&)::callback_read(size_t seg_index,
											 soclib::common::Segment &s,
											 soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
											 const tlmt_core::tlmt_time &time,
											 void *private_data)
{
	// std::cout << "callback_read" << std::endl;
	size_t i,packet_size;

	packet_size=pkt->length;
	if (pkt->contig) {
		for (i=0;i<packet_size;i++) {
			//pkt->buf[i]= m_contents[seg_index][((pkt->address[0]+i*4)-s.baseAddress()) / 4];
			pkt->buf[i]= m_contents[seg_index][((pkt->address+i*4)-s.baseAddress()) / 4];
		}
	} else {
		for (i=0;i<packet_size;i++) {
			//pkt->buf[i]= m_contents[seg_index][(pkt->address[i]-s.baseAddress()) / 4];
			pkt->buf[i]= m_contents[seg_index][(pkt->address-s.baseAddress()) / 4];
		}
	}
	// std::cout << "time callback_read=" << time << std::endl;

	//rsp.cmd=pkt->cmd;
	rsp.length=pkt->length;
	rsp.srcid=pkt->srcid;
	rsp.pktid=pkt->pktid;
	rsp.trdid=pkt->trdid;
	rsp.error=0;

	p_vci.send(&rsp, time+tlmt_core::tlmt_time(pkt->length+5));
	m_return.set_time(time+tlmt_core::tlmt_time(pkt->length+5));
	return m_return;
}

tmpl(tlmt_core::tlmt_return&)::callback_write(size_t seg_index,
											  soclib::common::Segment &s,
											  soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
											  const tlmt_core::tlmt_time &time,
											  void *private_data)
{
	// std::cout << "callback_write" << std::endl;
	size_t i,packet_size;

	packet_size=pkt->length;
	for (i=0;i<packet_size;i++)
	{
		//uint32_t index = (pkt->address[i]-s.baseAddress()) / 4;
		uint32_t index = (pkt->address-s.baseAddress()) / 4;
		ram_t *tab = m_contents[seg_index];
		unsigned int cur = tab[index];
		uint32_t mask = 0;
		unsigned int be=pkt->be;

		if ( be & 1 )
			mask |= 0x000000ff;
		if ( be & 2 )
			mask |= 0x0000ff00;
		if ( be & 4 )
			mask |= 0x00ff0000;
		if ( be & 8 )
			mask |= 0xff000000;

		tab[index] = (cur & ~mask) | (pkt->buf[i] & mask);
	}
	//rsp.cmd=pkt->cmd;
	rsp.length=pkt->length;
	rsp.srcid=pkt->srcid;
	rsp.pktid=pkt->pktid;
	rsp.trdid=pkt->trdid;

	p_vci.send(&rsp, time+tlmt_core::tlmt_time(pkt->length+5));
	m_return.set_time(time+tlmt_core::tlmt_time(pkt->length+5));

	return m_return;
}

tmpl(/**/)::VciRam(
	sc_core::sc_module_name name,
	const soclib::common::IntTab &index,
	const soclib::common::MappingTable &mt,
	common::ElfLoader &loader)
		   : soclib::tlmt::BaseModule(name),
		   m_index(index),
		   m_mt(mt),
		   m_loader(new common::ElfLoader(loader)),
		   p_vci("vci", new tlmt_core::tlmt_callback<VciRam,soclib::tlmt::vci_cmd_packet<vci_param> *>(
				   this, &VciRam<vci_param>::callback))
{
	m_segments = m_mt.getSegmentList(m_index);
	std::list<soclib::common::Segment>::iterator seg;

	m_contents = new ram_t*[m_segments.size()];
	size_t word_size = sizeof(typename vci_param::data_t);
	size_t i=0;
	for (i=0, seg = m_segments.begin(); seg != m_segments.end(); ++i, ++seg ) {
		soclib::common::Segment &s = *seg;	
		m_contents[i] = new ram_t[(s.size()+word_size-1)/word_size];
	}

	if ( m_loader ) {
		for (i=0, seg = m_segments.begin(); seg != m_segments.end(); ++i, ++seg ) {
			soclib::common::Segment &s = *seg;	
			m_loader->load(&m_contents[i][0], s.baseAddress(), s.size());
			for ( size_t addr = 0; addr < s.size()/word_size; ++addr )
				m_contents[i][addr] = le_to_machine(m_contents[i][addr]);
		}
	}
}


}}
