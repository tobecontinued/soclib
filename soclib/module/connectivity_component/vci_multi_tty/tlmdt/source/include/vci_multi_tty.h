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
 * Maintainers: fpecheux, alinev
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#ifndef __VCI_MULTI_TTY_H
#define __VCI_MULTI_TTY_H

#include <stdarg.h>
#include <tlmdt>                                 // TLM-DT headers
#include "mapping_table.h"                       // mapping table
#include "tty.h"
#include "tty_wrapper.h"

namespace soclib { namespace tlmdt {

template <typename vci_param>
class VciMultiTty
  : public sc_core::sc_module
  , virtual public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types> // inherit from TLM "forward interface"
{
 private:
  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  std::vector<soclib::common::TtyWrapper*> m_term;
  
  soclib::common::IntTab                   m_index;
  soclib::common::MappingTable             m_mt;
  std::list<soclib::common::Segment>       segList;
  int                                      m_n_irq;
  
  size_t m_cpt_read;
  size_t m_cpt_write;
  size_t m_cpt_cycle;
  size_t m_cpt_idle;
  
  /////////////////////////////////////////////////////////////////////////////////////
  // Local Fuctions
  /////////////////////////////////////////////////////////////////////////////////////
  void init(const std::vector<std::string> &names);

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if  (VCI TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_fw       // receive command from initiator
    ( tlm::tlm_generic_payload &payload,      // payload
      tlm::tlm_phase           &phase,        // phase
      sc_core::sc_time         &time);        // time

  // Not implemented for this example but required by interface
  void b_transport                          // b_transport() - Blocking Transport
  ( tlm::tlm_generic_payload &payload,      // payload
    sc_core::sc_time         &time);        // time
  
  // Not implemented for this example but required by interface
  bool get_direct_mem_ptr
  ( tlm::tlm_generic_payload &payload,      // payload
    tlm::tlm_dmi             &dmi_data);    // DMI data
  
  // Not implemented for this example but required by interface
  unsigned int transport_dbg                            
  ( tlm::tlm_generic_payload &payload);     // payload

 protected:
  SC_HAS_PROCESS(VciMultiTty);
 public:
  tlm::tlm_target_socket<32,tlm::tlm_base_protocol_types> p_vci_target;   // VCI TARGET socket

  //std::vector<tlm_utils::simple_initiator_socket_tagged<VciMultiTty,32,tlm::tlm_base_protocol_types> *> p_irq_initiator; // IRQ INITIATOR socket

  VciMultiTty(sc_core::sc_module_name name,
	      const soclib::common::IntTab &index,
	      const soclib::common::MappingTable &mt,
	      const char *first_name,
	      ...);

  VciMultiTty(sc_core::sc_module_name name,
	      const soclib::common::IntTab &index,
	      const soclib::common::MappingTable &mt,
	      const std::vector<std::string> &names);
 
  size_t getNRead();

  size_t getNWrite();

  size_t getTotalCycles();

  size_t getActiveCycles();

  size_t getIdleCycles();

  void print_stats();
};
}}
#endif /* __VCI_MULTI_TTY_H */
