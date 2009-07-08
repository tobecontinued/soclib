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

#include "vci_cmd_arb_rsp_rout.h"

namespace soclib { namespace tlmdt {

#define tmpl(x) x VciCmdArbRspRout

/////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////
tmpl(/***/)::VciCmdArbRspRout                       // constructor
( sc_core::sc_module_name name                      // module name
  , const soclib::common::MappingTable &mt          // mapping table
  , const soclib::common::IntTab &index             // initiator index
  , sc_core::sc_time delay                          // interconnect delay
  , bool external_access
  )
  : sc_module(name)                        
  , m_routing_table(mt.getIdMaskingTable(index.level()))
  , m_locality_table(mt.getIdLocalityTable(index))
  , m_delay(delay)
  , m_external_access(external_access)
  , p_vci_initiator("vcisocket")
{ 
  // bind INITIATOR VCI SOCKET
  p_vci_initiator(*this);                     

  //PDES local time
  m_pdes_local_time = new pdes_local_time(sc_core::SC_ZERO_TIME);

  //payload extension
  m_extension_pointer = new soclib_payload_extension();

  // register thread process
  SC_THREAD(behavior);                  
}

/////////////////////////////////////////////////////////////////////////////////////
// THREAD
/////////////////////////////////////////////////////////////////////////////////////
tmpl(void)::behavior(void)   // initiator thread
{
  // send null messages in the simulation begin
  m_extension_pointer->set_null_message();
  m_payload.set_extension(m_extension_pointer);

  m_phase = tlm::BEGIN_REQ;
  m_time = m_pdes_local_time->get() + m_delay;

  p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_time);

  packet_struct packet;
  while (true){
    if (!packet_fifo.empty()) {
      do{
	packet = packet_fifo.front();
	packet_fifo.pop_front();
	packet.payload->get_extension(m_extension_pointer);
	//optimization
	//if the current transaction is a null message and the fifo is no empty then get next transaction
      }while(m_extension_pointer->is_null_message() && !packet_fifo.empty());
    }
    else {
      sc_core::wait(m_fifo_event);
      continue;
    }

    m_phase = tlm::BEGIN_REQ;
    m_time = packet.time;
    if ( m_time < m_pdes_local_time->get() ){
#ifdef SOCLIB_MODULE_DEBUG
      std::cout << "[" << name() << "] UPDATE MESSAGE TIME time = " << m_time.value() << std::endl;
#endif
      m_time = m_pdes_local_time->get();
    }

#ifdef SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] send message to target time = " << m_time.value() << std::endl;
#endif

    p_vci_initiator->nb_transport_fw(*packet.payload, m_phase, m_time);

  } // end while true
} // end initiator_thread 

tmpl(void)::setRspArbCmdRout(std::vector<VciRspArbCmdRout *> &RspArbCmdRout)
{
  m_RspArbCmdRout=RspArbCmdRout;
}

tmpl(void)::put(tlm::tlm_generic_payload *payload, const sc_core::sc_time &time)
{
  packet_struct packet;
  packet.payload = payload;
  packet.time = time;
  
  packet_fifo.push_back(packet);

  m_fifo_event.notify(sc_core::SC_ZERO_TIME);
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_bw 
( tlm::tlm_generic_payload   &payload,            // payload
  tlm::tlm_phase             &phase,              // phase
  sc_core::sc_time           &time)               // time
{
  soclib_payload_extension *resp_extension_ptr;
  payload.get_extension(resp_extension_ptr);

  unsigned int src = resp_extension_ptr->get_src_id();

  //if source IS NOT local (only possible in vci_local_crossbar module)
  if (!m_locality_table[src]){
    src = m_RspArbCmdRout.size() - 1;
  }
  // if source IS local
  else{
    src = m_routing_table[src];
    //if message arrive from an external initiator (initiator is not local to cluster)
    if(m_external_access){
      m_RspArbCmdRout[src]->set_external_access(src, false);
    }
  }
  
  //updated the local time
  m_pdes_local_time->set(time + UNIT_TIME);

  //updated the transaction time
  time = time + m_delay;
  
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] send to " << src << " rsp time = " << time.value() << std::endl;
#endif

  m_RspArbCmdRout[src]->p_vci_target->nb_transport_bw(payload, phase, time);
  
  return tlm::TLM_COMPLETED;
} // end backward nb transport 

// Not implemented for this example but required by interface
tmpl(void)::invalidate_direct_mem_ptr            // invalidate_direct_mem_ptr
( sc_dt::uint64 start_range,                     // start range
  sc_dt::uint64 end_range                        // end range
) 
{
}

}}
