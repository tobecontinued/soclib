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

#ifndef VCI_VGMN_H
#define VCI_VGMN_H

#include <vector>
#include <sstream>
#include <tlmdt>                          // TLM-DT headers

#include "mapping_table.h"                //mapping table
#include "vci_cmd_arb_rsp_rout.h"         //Our header
#include "vci_rsp_arb_cmd_rout.h"         //Our header
#include "centralized_buffer.h"           //centralized buffer

namespace soclib { namespace tlmdt {

class VciVgmn
  : public sc_core::sc_module            // inherit from SC module base clase
{
private:
  std::vector<VciCmdArbRspRout *> m_CmdArbRspRout;       //vci_cmd_arb_rsp_rout modules
  std::vector<VciRspArbCmdRout *> m_RspArbCmdRout;       //vci_rsp_arb_cmd_rout modules
  centralized_buffer              m_centralized_buffer;  //centralized buffer

  std::vector<tlm_utils::simple_target_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types> *> p_vci_targets; // VCI target ports
  
  std::vector<tlm_utils::simple_initiator_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types> *> p_vci_initiators; // VCI initiator ports


  /////////////////////////////////////////////////////////////////////////////////////
  // Fuction  tlm::tlm_fw_transport_if (VCI TARGET PORTS UP)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_fw_up   // receive command from initiator
  ( int                       id,         // register id
    tlm::tlm_generic_payload &payload,    // payload
    tlm::tlm_phase           &phase,      // phase
    sc_core::sc_time         &time);      // time

  /////////////////////////////////////////////////////////////////////////////////////
  // Fuction  tlm::tlm_fw_transport_if (VCI TARGET PORTS DOWN)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_fw_down // receive command from initiator
  ( int                       id,         // register id
    tlm::tlm_generic_payload &payload,    // payload
    tlm::tlm_phase           &phase,      // phase
    sc_core::sc_time         &time);      // time
 
  /////////////////////////////////////////////////////////////////////////////////////
  // Fuction  tlm::tlm_bw_transport_if (VCI INITIATOR PORTS UP)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_bw_up   // receive command from initiator
  ( int                       id,         // register id
    tlm::tlm_generic_payload &payload,    // payload
    tlm::tlm_phase           &phase,      // phase
    sc_core::sc_time         &time);      // time

  /////////////////////////////////////////////////////////////////////////////////////
  // Fuction  tlm::tlm_bw_transport_if (VCI INITIATOR PORTS DOWN)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_bw_down // receive command from initiator
  ( int                       id,         // register id
    tlm::tlm_generic_payload &payload,    // payload
    tlm::tlm_phase           &phase,      // phase
    sc_core::sc_time         &time);      // time

  void init(
	    sc_core::sc_module_name name,                      // module name
	    const soclib::common::MappingTable &mt,            // mapping table
	    const soclib::common::IntTab &index,               // index
	    size_t nb_init,                                    // number of initiators
	    size_t nb_target,                                  // number of targets
	    sc_core::sc_time delay );                           // interconnect delay

public:

  std::vector<tlm_utils::simple_target_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types> *> p_vci_target; // VCI target ports
  
  std::vector<tlm_utils::simple_initiator_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types> *> p_vci_initiator; // VCI initiator ports

  VciVgmn(                                               //constructor
	   sc_core::sc_module_name            name,      //module name
	   const soclib::common::MappingTable &mt,       //mapping table
	   const soclib::common::IntTab       &index,    //index of mapping table
	   int nb_init,                                  //number of initiators connect to VGMN
	   int nb_target,                                //number of targets connect to VGMN
	   sc_core::sc_time delay);                      //VGMN delay


  VciVgmn(                                               //constructor
	   sc_core::sc_module_name            name,      //module name
	   const soclib::common::MappingTable &mt,       //mapping table
	   size_t nb_init,                               //number of initiators connect to VGMN
	   size_t nb_target,                             //number of targets connect to VGMN
	   size_t min_latency,                           //VGMN minimal latency
	   size_t fifo_depth );                          //parameter do not use


};

}}
#endif
