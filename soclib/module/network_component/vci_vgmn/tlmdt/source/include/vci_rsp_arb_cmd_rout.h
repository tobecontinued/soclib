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

#ifndef __VCI_RSP_ARB_CMD_ROUT_H__ 
#define __VCI_RSP_ARB_CMD_ROUT_H__

#include <tlmdt>	                                        // TLM-DT headers
#include "mapping_table.h"                                      // mapping table
#include "centralized_buffer.h"                                 // centralized buffer

namespace soclib { namespace tlmdt {

class VciCmdArbRspRout;

class VciRspArbCmdRout                                           // VciRspArbCmdRout
:         public sc_core::sc_module           	                 // inherit from SC module base clase
{
private:
 
  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  uint32_t                                                   m_index;              // local index
  sc_core::sc_time                                           m_delay;              // interconnect delay
  centralized_buffer                                        *m_centralized_buffer; // centralized buffer
  const soclib::common::AddressDecodingTable<uint32_t, int>  m_routing_table;      // routing table
  const soclib::common::AddressDecodingTable<uint32_t, bool> m_locality_table;     // locality table
  std::vector<VciCmdArbRspRout *>                            m_CmdArbRspRout;      // cmd_arb_rsp_rout blocks

  //FIELDS OF A NULL MESSAGE
  tlm::tlm_generic_payload                                  *m_null_payload_ptr;
  soclib_payload_extension                                  *m_null_extension_ptr;

  FILE * myFile;

  /////////////////////////////////////////////////////////////////////////////////////
  // Fuction  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum my_nb_transport_fw                    
  ( tlm::tlm_generic_payload &payload,               // payload
    tlm::tlm_phase           &phase,                 // phase
    sc_core::sc_time         &time);                 // time
 
 public:  
  
  tlm_utils::simple_target_socket<VciRspArbCmdRout,32,tlm::tlm_base_protocol_types>  p_vci_target; // VCI TARGET port
  
  VciRspArbCmdRout(                                                // constructor
		   sc_core::sc_module_name module_name             // SC module name
		   , const soclib::common::MappingTable &mt        // mapping table
		   , const soclib::common::IntTab &global_index    // global initiator index
		   , uint32_t local_index                          // local initiator index
		   , sc_core::sc_time delay                        // interconnect delay
		   , centralized_buffer *cb);                      // centralized buffer
  
  void setCmdArbRspRout(std::vector<VciCmdArbRspRout *> &CmdArbRspRout);

  void set_external_access(unsigned int index, bool b);

};

}}

#endif /* __VCI_RSP_ARB_CMD_ROUT_H__ */
