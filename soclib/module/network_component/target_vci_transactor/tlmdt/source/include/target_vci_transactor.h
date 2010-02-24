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
 * Maintainers: alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2010
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#ifndef __TARGET_VCI_TRANSACTOR_H__ 
#define __TARGET_VCI_TRANSACTOR_H__

#include <tlmdt>	   	             // TLM-DT headers
#include "target_vci_transactor_buffer.h"
#include "vci_initiator.h"

namespace soclib { namespace tlmdt {

template<typename vci_param_caba, typename vci_param_tlmdt>
class TargetVciTransactor                        
  : public sc_core::sc_module               // inherit from SC module base class
  , virtual public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types> // inherit from TLM "forward interface"
{
 private:
  //VCI port
  target_vci_transactor_buffer  m_buffer;         
  pdes_local_time          *m_pdes_local_time; // local time

  sc_core::sc_event         m_pop_event;   // pop event
  sc_core::sc_event         m_push_event;  // push event

  bool                      m_working;
  unsigned char             m_data[100];
  uint32_t                  m_clock_count;
  uint32_t                  m_initial_time;
  uint32_t                  m_final_time;

  bool                      m_cmd_working;
  uint32_t                  m_cmd_count;
  uint32_t                  m_cmd_be;
  uint32_t                  m_cmd_nwords;
  tlm::tlm_generic_payload *m_cmd_payload;
  soclib_payload_extension *m_cmd_extension;
  tlm::tlm_phase           *m_cmd_phase;
  sc_core::sc_time         *m_cmd_time;

  uint32_t                  m_rsp_count;
  tlm::tlm_generic_payload *m_rsp_payload;
  soclib_payload_extension *m_rsp_extension;
  tlm::tlm_phase           *m_rsp_phase;
  sc_core::sc_time         *m_rsp_time;

  unsigned int              m_rscrid;
  unsigned int              m_rpktid;
  unsigned int              m_rtrdid;

  //IRQ ports
  unsigned int              m_nirq;
  bool                     *m_irq;
  bool                      m_active_irq;

  //IRQ message
  tlm::tlm_generic_payload *m_irq_payload_ptr;
  soclib_payload_extension *m_irq_extension_ptr;
  tlm::tlm_phase            m_irq_phase;
  sc_core::sc_time          m_irq_time;

  /////////////////////////////////////////////////////////////////////////////////////
  // Fuctions
  /////////////////////////////////////////////////////////////////////////////////////
  void behavior(void);                      // thread
  void cmd();
  void rsp();
  //irq functions
  void behavior_irq(void);                      // thread
  void irq(void);
  void send_interrupt(int id, bool irq);

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_fw        // receive command from initiator
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
  SC_HAS_PROCESS(TargetVciTransactor);
 public:

  // PORTS CABA
  sc_core::sc_out<bool>                                   p_clk;
  sc_core::sc_out<bool>                                   p_resetn;
  sc_core::sc_in<bool>                                   *p_irq_target;

  // PORTS TLMDT
  soclib::caba::VciInitiator<vci_param_caba>              p_vci_initiator; // TLMDT -> CABA
  tlm::tlm_target_socket<32,tlm::tlm_base_protocol_types> p_vci_target;    // INTERCONNECT -> TLMDT
  std::vector<tlm_utils::simple_initiator_socket_tagged<TargetVciTransactor,32,tlm::tlm_base_protocol_types> *> p_irq_initiator;  // IRQ initiator port

  TargetVciTransactor(sc_core::sc_module_name name);
  
  TargetVciTransactor(sc_core::sc_module_name name, size_t n_irq);

  ~TargetVciTransactor();

};
}}
#endif
