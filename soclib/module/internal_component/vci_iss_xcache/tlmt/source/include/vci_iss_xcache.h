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


#include <tlmt>
#include "tlmt_base_module.h"
#include "vci_ports.h"

#include <inttypes.h>
#include "soclib_endian.h"

#include "write_buffer.h"
#include "generic_cache.h"

namespace soclib { namespace tlmt {

template<typename iss_t,typename vci_param>
class VciIssXcache
    : public soclib::tlmt::BaseModule
{
    tlmt_core::tlmt_thread_context c0;
    sc_core::sc_event e0;
    sc_core::sc_event m_rsp_received;
	tlmt_core::tlmt_return m_return;
    iss_t m_iss;
    writeBuffer<iss_t,vci_param> m_wbuf;
    soclib::tlmt::vci_cmd_packet<vci_param> m_cmd;
    uint32_t m_addresses_tab[100];
    uint32_t m_write_buffer[100];
    uint32_t m_read_buffer[100];
    uint32_t m_read_buffer_ins[100];
    bool m_vci_pending;
    bool m_read_error;
    bool m_write_error;
    tlmt_core::tlmt_time m_rsptime;
    uint32_t m_counter;
    uint32_t m_lookahead;
    genericCache<vci_param> m_dcache ;
    genericCache<vci_param> m_icache ;

protected:
    SC_HAS_PROCESS(VciIssXcache);

public:
    soclib::tlmt::VciInitiator<vci_param> p_vci;
    tlmt_core::tlmt_in<bool> *p_irq;

    VciIssXcache( sc_core::sc_module_name name, int id );

    tlmt_core::tlmt_return &rspReceived(soclib::tlmt::vci_rsp_packet<vci_param> *pkt,
									 const tlmt_core::tlmt_time &time,
									 void *private_data);
    tlmt_core::tlmt_return &irqReceived(bool,
									 const tlmt_core::tlmt_time &time,
									 void *private_data);
    void execLoop();

void xcacheAccess(
        bool &ins_asked,
        uint32_t &ins_addr,
        bool &mem_asked,
        enum iss_t::DataAccessType &mem_type,
        uint32_t &mem_addr,
        uint32_t &mem_wdata,

        uint32_t &mem_rdata,
	bool &mem_dber,
        uint32_t &ins_rdata,
	bool &ins_iber
        );

};

}}

