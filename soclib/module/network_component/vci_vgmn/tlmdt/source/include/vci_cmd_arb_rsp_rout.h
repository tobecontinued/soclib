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
 * Maintainers: fpecheux, alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#ifndef __VCI_CMD_ARB_RSP_ROUT_H__
#define __VCI_CMD_ARB_RSP_ROUT_H__

#include <tlmdt>			          // TLM-DT headers
#include "vci_rsp_arb_cmd_rout.h"                 // Our header

struct packet_struct{
  tlm::tlm_generic_payload *payload;
  sc_core::sc_time time;
} ;

namespace soclib { namespace tlmdt {

class VciRspArbCmdRout;

class VciCmdArbRspRout                        // 
  : public sc_core::sc_module                 // inherit from SC module base clase
{
private:
  
  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  std::vector <VciRspArbCmdRout *>                           m_RspArbCmdRout;     // vector of rsp_arb_cmd_rout
  const soclib::common::AddressMaskingTable<uint32_t>        m_routing_table;     // routing table
  const soclib::common::AddressDecodingTable<uint32_t, bool> m_locality_table;    // locality table
  std::list<packet_struct>                                   packet_fifo;         // fifo
  sc_core::sc_event                                          m_fifo_event;        // fifo event
 
  sc_core::sc_time                                           m_delay;             // interconnect delay
  pdes_local_time                                           *m_pdes_local_time;   // local time
  bool                                                       m_external_access;   // true if module has external access (crossbar parameter)

  // FIELDS OF A NORMAL TRANSACTION
  tlm::tlm_generic_payload                                   m_payload;           // payload
  soclib_payload_extension                                  *m_extension_pointer; // payload extension
  tlm::tlm_phase                                             m_phase;             // phase
  sc_core::sc_time                                           m_time;              // time

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum my_nb_transport_bw
  ( tlm::tlm_generic_payload &payload,    // transaction payload
    tlm::tlm_phase           &phase,      // transaction phase
    sc_core::sc_time         &time);      // transaction time
  
protected:
  SC_HAS_PROCESS(VciCmdArbRspRout);
  
public:
  tlm_utils::simple_initiator_socket<VciCmdArbRspRout,32,tlm::tlm_base_protocol_types> p_vci_initiator; // VCI initiator port

  VciCmdArbRspRout(                                                  // constructor
		   sc_core::sc_module_name name                      // module name
		   , const soclib::common::MappingTable &mt          // mapping table
		   , const soclib::common::IntTab &index             // initiator index
		   , sc_core::sc_time delay                          // interconnect delay
		   , bool external_access);                          // true if module has external access (crossbar parameter)
  
  void behavior (void);                                              // initiator thread
  
  void setRspArbCmdRout(std::vector<VciRspArbCmdRout *> &RspArbCmdRout);

  void put(tlm::tlm_generic_payload *payload, const sc_core::sc_time &time);
 
}; 
}}
#endif /* __VCI_CMD_ARB_RSP_ROUT_H__ */
