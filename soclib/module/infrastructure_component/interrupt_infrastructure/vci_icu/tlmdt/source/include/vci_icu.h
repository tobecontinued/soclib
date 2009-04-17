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

#ifndef SOCLIB_TLMT_VCI_ICU_H
#define SOCLIB_TLMT_VCI_ICU_H

#include <tlmdt>                          // TLM-DT headers
#include "mapping_table.h"
#include "icu.h"

struct irq_struct{
  bool val;
  sc_core::sc_time time;
};

namespace soclib { namespace tlmdt {

template <typename vci_param>
class VciIcu
  : public sc_core::sc_module             // inherit from SC module base clase
{
 private:

  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  irq_struct                         *irq;

  soclib::common::IntTab              m_index;
  soclib::common::MappingTable        m_mt;
  std::list<soclib::common::Segment>  m_segments;

  size_t                              m_nirq;
  unsigned int                        r_mask;
  unsigned int                        r_current;

  //FIELDS OF AN IRQ TRANSACTION
  tlm::tlm_generic_payload            m_irq_payload;
  tlm::tlm_phase                      m_irq_phase;
  sc_core::sc_time                    m_irq_time;

  /////////////////////////////////////////////////////////////////////////////////////
  // Fuctions
  /////////////////////////////////////////////////////////////////////////////////////
  void behavior(void);                        // initiator thread
  int getInterruption();
  unsigned int getActiveInterruptions(const sc_core::sc_time time);

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if  (VCI TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum vci_nb_transport_fw      // receive vci command from initiator
    ( tlm::tlm_generic_payload &payload,      // payload
      tlm::tlm_phase           &phase,        // phase
      sc_core::sc_time         &time);        // time

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if (IRQ TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum irq_nb_transport_fw      // receive interruption from initiator
  ( int                         id,           // interruption id
    tlm::tlm_generic_payload   &payload,      // payload
    tlm::tlm_phase             &phase,        // phase
    sc_core::sc_time           &time);        // time


protected:
  SC_HAS_PROCESS(VciIcu);
public:
  tlm_utils::simple_target_socket<VciIcu,32,tlm::tlm_base_protocol_types> p_vci_target; // VCI TARGET socket
  tlm_utils::simple_initiator_socket<VciIcu,32,tlm::tlm_base_protocol_types> p_irq_initiator; // IRQ INITIATOR socket
  std::vector<tlm_utils::simple_target_socket_tagged<VciIcu,32,tlm::tlm_base_protocol_types> *> p_irq_target; // IRQ TARGET socket

  VciIcu(
	 sc_core::sc_module_name name,
	 const soclib::common::IntTab &index,
	 const soclib::common::MappingTable &mt,
	 size_t nirq);
  
  ~VciIcu();
};
}}

#endif /* SOCLIB_TLMT_VCI_ICU_H */