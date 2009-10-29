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

#include "../include/vci_vgmn.h"

namespace soclib { namespace tlmdt {

/////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////
VciVgmn::VciVgmn
( sc_core::sc_module_name name,                      // module name
  const soclib::common::MappingTable &mt,            // mapping table
  size_t nb_init,                                    // number of initiators
  size_t nb_target,                                  // number of targets
  size_t min_latency,                                // minimal latency
  size_t fifo_depth )                                // parameter do not use 
  : sc_module(name)
  , m_centralized_buffer(nb_init)                    // centralized buffer
{
  init(name,mt,soclib::common::IntTab(),nb_init,nb_target,min_latency * UNIT_TIME);
}

VciVgmn::VciVgmn
( sc_core::sc_module_name name,                      // module name
  const soclib::common::MappingTable &mt,            // mapping table
  const soclib::common::IntTab &global_index,        // global index
  int nb_init,                                       // number of initiators
  int nb_target,                                     // number of targets
  sc_core::sc_time delay )                           // interconnect delay
  : sc_module(name)
  , m_centralized_buffer(nb_init)                    // centralized buffer
{
  init(name,mt,global_index,nb_init,nb_target,delay);
}
    
void VciVgmn::init
( sc_core::sc_module_name name,                      // module name
  const soclib::common::MappingTable &mt,            // mapping table
  const soclib::common::IntTab &index,               // index
  size_t nb_init,                                    // number of initiators
  size_t nb_target,                                  // number of targets
  sc_core::sc_time delay )                           // interconnect delay
{
  // Phase 1, allocate nb_target CmdArbRspRout blocks
  for (size_t i=0;i<nb_target;i++){
    std::ostringstream tmpName;
    tmpName << name << "_CmdArbRspRout" << i;
    m_CmdArbRspRout.push_back(new VciCmdArbRspRout(tmpName.str().c_str(), mt, index, delay, false));
  }
  
  // Phase 2, allocate nb_init RspArbCmdRout blocks
  for (size_t i=0;i<nb_init;i++){
    std::ostringstream tmpName;
    tmpName << name << "_RspArbCmdRout" << i;
    m_RspArbCmdRout.push_back(new VciRspArbCmdRout(tmpName.str().c_str() ,mt, index, i, delay, &m_centralized_buffer));
  }
  
  // Phase 3, each cmdArbRspRout sees all the RspArbCmdRout
  for (size_t i=0;i<nb_target;i++){
    m_CmdArbRspRout[i]->setRspArbCmdRout(m_RspArbCmdRout);
  }
  
  // Phase 4, each rspArbCmdRout sees all the CmdArbRspRout
  for (size_t i=0;i<nb_init;i++){
    m_RspArbCmdRout[i]->setCmdArbRspRout(m_CmdArbRspRout);
  }

  // bind VCI TARGET SOCKETS
  for(size_t i=0;i<nb_init;i++){
    std::ostringstream target_name;
    target_name <<  name << "target" << i;
    p_vci_target.push_back(new tlm_utils::simple_target_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types>(target_name.str().c_str()));
    p_vci_target[i]->register_nb_transport_fw(this, &VciVgmn::nb_transport_fw_down, i);

    std::ostringstream inits_name;
    inits_name <<  name << "inits" << i;
    p_vci_initiators.push_back(new tlm_utils::simple_initiator_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types>(inits_name.str().c_str()));
    p_vci_initiators[i]->register_nb_transport_bw(this, &VciVgmn::nb_transport_bw_up, i);

    (*p_vci_initiators[i])(m_RspArbCmdRout[i]->p_vci_target);
  }

  // bind VCI INITIATOR SOCKETS
  for(size_t i=0;i<nb_target;i++){
    std::ostringstream init_name;
    init_name <<  name << "init" << i;
    p_vci_initiator.push_back(new tlm_utils::simple_initiator_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types>(init_name.str().c_str()));
    p_vci_initiator[i]->register_nb_transport_bw(this, &VciVgmn::nb_transport_bw_down, i);

    std::ostringstream targets_name;
    targets_name <<  name << "targets" << i;
    p_vci_targets.push_back(new tlm_utils::simple_target_socket_tagged<VciVgmn,32,tlm::tlm_base_protocol_types>(targets_name.str().c_str()));
    p_vci_targets[i]->register_nb_transport_fw(this, &VciVgmn::nb_transport_fw_up, i);


    (*p_vci_targets[i])(m_CmdArbRspRout[i]->p_vci_initiator);
    
  }
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tlm::tlm_sync_enum VciVgmn::nb_transport_fw_up   
( int                          id,                 // register id
  tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{
  (*p_vci_initiator[id])->nb_transport_fw(payload, phase, time);
  return  tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tlm::tlm_sync_enum VciVgmn::nb_transport_fw_down   
( int                          id,                 // register id
  tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{
  (*p_vci_initiators[id])->nb_transport_fw(payload, phase, time);
  return  tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tlm::tlm_sync_enum VciVgmn::nb_transport_bw_up   
( int                          id,                 // register id
  tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{
  (*p_vci_target[id])->nb_transport_bw(payload, phase, time);
  return  tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tlm::tlm_sync_enum VciVgmn::nb_transport_bw_down   
( int                          id,                 // register id
  tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{
  (*p_vci_targets[id])->nb_transport_bw(payload, phase, time);
  return  tlm::TLM_COMPLETED;
}

}}
