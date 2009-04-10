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
 * Maintainers: fpecheux, nipo, alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#ifndef SOCLIB_TLMT_VCI_TIMER_H
#define SOCLIB_TLMT_VCI_TIMER_H

#include <tlmdt>                          // TLM-DT headers
#include "mapping_table.h"
#include "timer.h"

struct timer_struct{
  sc_core::sc_time period;
  sc_core::sc_time value;
  int              mode;
};

namespace soclib { namespace tlmdt {

template <typename vci_param>
class VciTimer 
  : public sc_core::sc_module             // inherit from SC module base clase
{
 private:

  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  soclib::common::IntTab              m_index;
  soclib::common::MappingTable        m_mt;
  std::list<soclib::common::Segment>  m_segments;

  size_t                              m_ntimer;
  timer_struct                       *m_timer;

  //FIELDS OF AN IRQ TRANSACTION
  tlm::tlm_generic_payload            m_irq_payload;
  tlm::tlm_phase                      m_irq_phase;
  sc_core::sc_time                    m_irq_time;

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum vci_nb_transport_fw    // receive command from initiator
  ( tlm::tlm_generic_payload &payload,      // payload
    tlm::tlm_phase           &phase,        // phase
    sc_core::sc_time         &time);        // time

protected:
  SC_HAS_PROCESS(VciTimer);
public:
  tlm_utils::simple_target_socket<VciTimer,32,tlm::tlm_base_protocol_types> p_vci_target; // VCI TARGET socket
  std::vector<tlm_utils::simple_initiator_socket_tagged<VciTimer,32,tlm::tlm_base_protocol_types> *> p_irq_initiator; // IRQ INITIATOR socket

  VciTimer(
	   sc_core::sc_module_name name,
	   const soclib::common::IntTab &index,
	   const soclib::common::MappingTable &mt,
	   size_t ntimer);
  
  ~VciTimer();

};
}}

#endif /* SOCLIB_TLMT_VCI_TIMER_H */
