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
 *     Fran�ois P�cheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#include "vci_rsp_arb_cmd_rout.h"                        // our header
#include "vci_cmd_arb_rsp_rout.h"                        // our header
                    
#ifndef VCI_RSP_ARB_CMD_ROUT_DEBUG
#define VCI_RSP_ARB_CMD_ROUT_DEBUG 0
#endif

namespace soclib { namespace tlmdt {

#define tmpl(x) x VciRspArbCmdRout

/////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////
tmpl(/**/)::VciRspArbCmdRout
( sc_core::sc_module_name module_name               // module name
  , const soclib::common::MappingTable &mt          // mapping table
  , const soclib::common::IntTab &global_index      // global index
  , uint32_t local_index                            // local index
  , sc_core::sc_time delay                          // interconnect delay
  , centralized_buffer *cb                          // centralized buffer
  ) 
  : sc_module(module_name)
  , m_index(local_index)
  , m_delay(delay)
  , m_centralized_buffer(cb)
  , m_routing_table(mt.getRoutingTable(global_index))
  , m_locality_table(mt.getLocalityTable(global_index))
  , p_vci_target("socket")
{
  //register callback fuction
  p_vci_target.register_nb_transport_fw(this, &VciRspArbCmdRout::my_nb_transport_fw);

#if VCI_RSP_ARB_CMD_ROUT_DEBUG
  std::ostringstream file_name;
  file_name << name() << ".txt";
  myFile = fopen ((file_name.str()).c_str(),"w");
#endif

  //create payload and extension to a null message  
  m_null_payload_ptr = new tlm::tlm_generic_payload();
  m_null_extension_ptr = new soclib_payload_extension();
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::my_nb_transport_fw   
( tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{
  //push a transaction in the centralized buffer
  m_centralized_buffer->push(m_index, payload, phase, time);

#if VCI_RSP_ARB_CMD_ROUT_DEBUG
  fprintf(myFile,"[%s] PUT time = %d CAN POP %d\n", name(), (int)time.value(), m_centralized_buffer->can_pop());
  //std::cout << "[" << name() << "] PUT time=" << std::dec << time.value();
  //std::cout << " CAN POP " <<  m_centralized_buffer->can_pop() << std::endl;
#endif

  //if all buffer positions are filled then a transaction can be sent 
  while ( m_centralized_buffer->can_pop() ) {
    uint32_t from;
    tlm::tlm_generic_payload *m_payload_ptr;
    soclib_payload_extension *m_extension_ptr;
    tlm::tlm_phase            m_phase;
    sc_core::sc_time          m_time;

    //false pop get the selected transaction from centralized buffer but NOT POP
    m_centralized_buffer->get_selected_transaction(from, m_payload_ptr, m_phase, m_time);

    //get payload extension
    m_payload_ptr->get_extension(m_extension_ptr);

    //if transaction command is active or inactive command then
    //the source is actived or desactived and the transaction is not sent
    if(m_extension_ptr->is_active() || m_extension_ptr->is_inactive()){
      //pop a transaction from centralized buffer
      m_centralized_buffer->pop(from);
      m_centralized_buffer->set_activity(from, m_extension_ptr->is_active());
    }
    //if transaction command is a null message then it must be sent to all target
    else if(m_extension_ptr->is_null_message()){
      //pop a transaction from centralized buffer
      m_centralized_buffer->pop(from);

      m_time = m_time + m_delay;

#if VCI_RSP_ARB_CMD_ROUT_DEBUG
      fprintf(myFile,"[%s] POP NULL MESSAGE time = %d\n", name(), (int)m_time.value());
      //std::cout << "[" << name() << "] POP NULL MESSAGE time=" << m_time.value() << std::endl;
#endif

      for(unsigned int i = 0; i < m_CmdArbRspRout.size(); i++){
	m_CmdArbRspRout[i]->put(m_payload_ptr, m_time);
      }
    }
    //if transaction has a valid command then
    //it must be sent to appropriated target and the null messages must be sent to the other targets
    else{
      //if target is NOT local then ... (only possible in vci_local_crossbar) 
      if (!m_locality_table[m_payload_ptr->get_address()]){
        unsigned int dest =  m_CmdArbRspRout.size() - 1;

	m_centralized_buffer->pop(from);
	m_time = m_time + m_delay;

#if VCI_RSP_ARB_CMD_ROUT_DEBUG
	fprintf(myFile,"[%s] POP IS NOT LOCAL from %d dest %d pktid = %d time = %d\n", name(), m_extension_ptr->get_src_id(), dest, m_extension_ptr->get_pkt_id(), (int)m_time.value());
	//std::cout << "[" << name() << "] POP IS NOT LOCAL from " << m_extension_ptr->get_src_id() << " dest " << dest << " time=" << m_time.value() <<  std::endl;
#endif

	m_centralized_buffer->set_external_access(from, true);
	m_CmdArbRspRout[dest]->put(m_payload_ptr, m_time);

      }
      //if target IS local then
      //the transaction is sent to appropriated target and null messages are sent to other targets
      else{
	//pop a transaction from centralized buffer
	m_centralized_buffer->pop(from);

	m_null_extension_ptr->set_null_message();
	m_null_payload_ptr->set_extension(m_null_extension_ptr);
	
	unsigned int dest = m_routing_table[m_payload_ptr->get_address()];
	assert( dest >= 0 && dest < m_CmdArbRspRout.size() );
	m_time = m_time + m_delay;
	
	for(unsigned int i = 0; i < m_CmdArbRspRout.size(); i++){
	  if(i==dest){
	    
#if VCI_RSP_ARB_CMD_ROUT_DEBUG
	    fprintf(myFile,"[%s] POP from %d dest %d pktid = %d time = %d\n", name(), m_extension_ptr->get_src_id(), dest, m_extension_ptr->get_pkt_id(), (int)m_time.value());
	    //std::cout << "[" << name() << "] POP from " << m_extension_ptr->get_src_id() << " dest " << dest << " time=" << m_time.value() <<  std::endl;
#endif
	    
	    m_CmdArbRspRout[i]->put(m_payload_ptr,m_time);
	  }
	  else{
#if VCI_RSP_ARB_CMD_ROUT_DEBUG
	    fprintf(myFile,"[%s] POP from %d dest %d pktid = %d NULL MESSAGE time = %d\n", name(), m_extension_ptr->get_src_id(), i, m_extension_ptr->get_pkt_id(), (int)m_time.value());

	    //std::cout << "[" << name() << "] POP from " << m_extension_ptr->get_src_id() << " dest " << i << " NULL MESSAGE time=" << m_time.value() <<  std::endl;
#endif
	    m_CmdArbRspRout[i]->put(m_null_payload_ptr, m_time);
	  }
	}
      }
    }
  }
  return  tlm::TLM_COMPLETED;
} //end nb_transport_fw

/////////////////////////////////////////////////////////////////////////////////////
// Fuctions
/////////////////////////////////////////////////////////////////////////////////////
tmpl(void)::setCmdArbRspRout(std::vector<VciCmdArbRspRout *> &CmdArbRspRout) 
{
  m_CmdArbRspRout=CmdArbRspRout;
}

tmpl(void)::set_external_access(unsigned int index, bool external_access)
{
  m_centralized_buffer->set_external_access(index, external_access);
}

}}