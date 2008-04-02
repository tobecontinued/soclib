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

#include <limits>
#include "../include/vci_rsp_arb_cmd_rout.h"
#include "../include/vci_cmd_arb_rsp_rout.h"

namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciCmdArbRspRout<vci_param>

tmpl(tlmt_core::tlmt_return&)::callback(soclib::tlmt::vci_rsp_packet<vci_param> *pkt,
										const tlmt_core::tlmt_time &time,
										void *private_data)
{
	m_RspArbCmdRout[pkt->srcid]->p_vci.send(pkt,time+m_delay);
	return m_return;
}

tmpl(void)::behavior()
{
	while (1) {
		int decision=through_fifo();
		// std::cout << "decision=" << decision << std::endl;
		switch (decision) {
		case -1:	// no packet available
			sc_core::wait(e0);
			break;
		case -2:	// the only packet available is false 
			sc_core::wait(sc_core::SC_ZERO_TIME);
			break;
		default:	// decision contains the index of the destination target
		{
			tlmt_core::tlmt_time packet_time=m_fifos_time[decision];
			if (packet_time<c0.time())
				packet_time=c0.time();
			tlmt_core::tlmt_return ret;
			ret=p_vci.send(m_fifos_pkt[decision],packet_time);
			// std::cout << std::dec <<  name() << " ret.time=" << ret.time() << std::endl;
			c0.set_time(ret.time());
			m_fifos_pkt[decision]=NULL;
			sc_core::wait(e0); // should be optimized here
		}
		break;
		}
	}
}

tmpl(void)::setRspArbCmdRout(std::vector<typename soclib::tlmt::VciRspArbCmdRout<vci_param> *> &RspArbCmdRout)
{
	m_RspArbCmdRout=RspArbCmdRout;
}

tmpl(void)::put(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,uint32_t idx,
				const tlmt_core::tlmt_time &time)
{
	// std::cout << "put" << std::endl;
	m_fifos_pkt[idx]=pkt;
	m_fifos_time[idx]=time;
	e0.notify(sc_core::SC_ZERO_TIME);
}

tmpl(int)::through_fifo()
{
	tlmt_core::tlmt_time min_time=std::numeric_limits<uint32_t>::max();
	int min_index=-1;
	bool min_found=false;
	int idx=-1;
	int high_priority_index=-1;
	tlmt_core::tlmt_time packet_time;
	bool packet_condition=false;
	int available_packets=0;

	for (int j=m_nbinit-1;j>=0;j--) {
		// Do the round robin, starting with initiators with
		// lower priority

		uint32_t k=(m_selected_port+j);
		if (k>=m_nbinit)
			k-=m_nbinit;

		// Do not take inactive initiators into account
		// when looking for the fifo with smallest timestamp
		if (!m_RspArbCmdRout[k]->p_vci.peer_active())
			continue;

		// If the packet is a real one, get its time directly,
		// If not, get the original time of the initiator and add the interconnect
		// delay to compare comparable times
		if (m_fifos_pkt[k]!=NULL) {
			packet_time=m_fifos_time[k];
			packet_condition=true;
		}
		else {
			packet_time=m_RspArbCmdRout[k]->p_vci.peer_time()+m_delay;
			packet_condition=false;
		}

		// If the fifo k contains a packet with the smallest timestamp
		// (real of false), modify the min variables. min_index indicates
		// the initiator with the smallest timestamp. min_found indicates
		// that a minimum has been found
		if (packet_time < min_time) {
			min_time=packet_time;
			min_index=k;
			min_found=true;
		} else if (min_found && (packet_time==min_time)) {
			if ((packet_condition) || (!m_RspArbCmdRout[k]->p_vci.peer_sending())) {
				min_index=k;
			}
		}

		if (packet_condition) {
			if (packet_time<=c0.time()) {
				high_priority_index=k;
			}
			available_packets++;
		}
	}

	if (available_packets==0) {
		idx=-1;
	} else {
		if (high_priority_index != -1) {
			idx=high_priority_index;
		} else {
			if (m_fifos_pkt[min_index]!=NULL) {
				idx=min_index;
				m_selected_port=(min_index+1)%m_nbinit;
			}
			else {
				idx=-2;
			}
		}
			
	} 

	return idx;
}

tmpl(/***/)::VciCmdArbRspRout( sc_core::sc_module_name name,
							   uint32_t idx,
							   uint32_t nb_init,
							   tlmt_core::tlmt_time dl )
			: soclib::tlmt::BaseModule(name),
			m_index(idx),
			m_nbinit(nb_init),
			m_delay(dl),
			p_vci("vci", new tlmt_core::tlmt_callback<VciCmdArbRspRout,soclib::tlmt::vci_rsp_packet<vci_param> *>(
					this, &VciCmdArbRspRout<vci_param>::callback), &c0)
{
	m_selected_port=0;
	m_fifos_pkt.reserve(nb_init);	
	m_fifos_time.reserve(nb_init);	
	for (size_t i=0;i<nb_init;i++) {
		m_fifos_pkt[i]=NULL;
	}
	SC_THREAD(behavior);
}

}}
