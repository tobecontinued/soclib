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

#include "interconnect.h"                           // our header
                    
namespace soclib { namespace tlmdt {

#define tmpl(x) x Interconnect

/////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////
tmpl(/**/)::Interconnect
( sc_core::sc_module_name module_name               // module name
  , int id                                          // identifier
  , const routing_table_t &rt                       // routing table
  , const locality_table_t &lt                      // locality table
  , const resp_routing_table_t &rrt                 // response routing table
  , const resp_locality_table_t &rlt                // response locality table
  , size_t n_inits                                  // number of inits
  , size_t n_targets                                // number of targets
  , size_t delay                                    // interconnect delay
  ) 
  : sc_module(module_name)
  , m_id(id)
  , m_inits(n_inits)
  , m_targets(n_targets)
  , m_delay(delay)
  , m_is_local_crossbar(true)
  , m_waiting_from(-1)
  , m_centralized_buffer("centralized_buffer", n_inits)
  , m_routing_table(rt)
  , m_locality_table(lt)
  , m_resp_routing_table(rrt)
  , m_resp_locality_table(rlt)
  , m_msg_count(0)
  , m_local_msg_count(0)
  , m_non_local_msg_count(0)
  , m_token_msg_count(0)
{
  init();
}

tmpl(/**/)::Interconnect
( sc_core::sc_module_name module_name               // module name
  , const routing_table_t &rt                       // routing table
  , const locality_table_t &lt                      // locality table
  , const resp_routing_table_t &rrt                 // response routing table
  , const resp_locality_table_t &rlt                // response locality table
  , size_t n_inits                                  // number of inits
  , size_t n_targets                                // number of targets
  , size_t delay                                    // interconnect delay
  ) 
  : sc_module(module_name)
  , m_id(0)
  , m_inits(n_inits)
  , m_targets(n_targets)
  , m_delay(delay)
  , m_is_local_crossbar(true)
  , m_waiting_from(-1)
  , m_centralized_buffer("centralized_buffer", n_inits)
  , m_routing_table(rt)
  , m_locality_table(lt)
  , m_resp_routing_table(rrt)
  , m_resp_locality_table(rlt)
  , m_msg_count(0)
  , m_local_msg_count(0)
  , m_non_local_msg_count(0)
  , m_token_msg_count(0)
{
  init();
}

tmpl(/**/)::Interconnect
( sc_core::sc_module_name module_name               // module name
  , int id                                          // identifier
  , const routing_table_t &rt                       // routing table
  , const resp_routing_table_t &rrt                 // response routing table
  , size_t n_inits                                  // number of inits
  , size_t n_targets                                // number of targets
  , size_t delay                                    // interconnect delay
  ) 
  : sc_module(module_name)
  , m_id(id)
  , m_inits(n_inits)
  , m_targets(n_targets)
  , m_delay(delay)
  , m_is_local_crossbar(false)
  , m_waiting_from(-1)
  , m_centralized_buffer("centralized_buffer", n_inits)
  , m_routing_table(rt)
  , m_resp_routing_table(rrt)
  , m_msg_count(0)
  , m_local_msg_count(0)
  , m_non_local_msg_count(0)
  , m_token_msg_count(0)
{
  init();
}

tmpl(/**/)::Interconnect
( sc_core::sc_module_name module_name               // module name
  , const routing_table_t &rt                       // routing table
  , const resp_routing_table_t &rrt                 // response routing table
  , size_t n_inits                                  // number of inits
  , size_t n_targets                                // number of targets
  , size_t delay                                    // interconnect delay
  ) 
  : sc_module(module_name)
  , m_id(0)
  , m_inits(n_inits)
  , m_targets(n_targets)
  , m_delay(delay)
  , m_is_local_crossbar(false)
  , m_waiting_from(-1)
  , m_centralized_buffer("centralized_buffer", n_inits)
  , m_routing_table(rt)
  , m_resp_routing_table(rrt)
  , m_msg_count(0)
  , m_local_msg_count(0)
  , m_non_local_msg_count(0)
  , m_token_msg_count(0)
{
  init();
}

tmpl(/**/)::~Interconnect(){
}

tmpl(void)::init(){
  //std::cout << "USING INTERCONNECT wait(sc_core::SC_ZERO_TIME)" << std::endl;

  // bind VCI TARGET SOCKETS
  for(int i=0;i<m_inits;i++){
    std::ostringstream target_name;
    target_name << "target" << i;
    p_to_initiator.push_back(new tlm_utils::simple_target_socket_tagged<Interconnect,32,tlm::tlm_base_protocol_types>(target_name.str().c_str()));
    p_to_initiator[i]->register_nb_transport_fw(this, &Interconnect::nb_transport_fw, i);
  }

  // bind VCI INITIATOR SOCKETS
  for(int i=0;i<m_targets;i++){
    std::ostringstream init_name;
    init_name << "init" << i;
    p_to_target.push_back(new tlm_utils::simple_initiator_socket_tagged<Interconnect,32,tlm::tlm_base_protocol_types>(init_name.str().c_str()));
    p_to_target[i]->register_nb_transport_bw(this, &Interconnect::nb_transport_bw, i);
  }

  //minimal local latency
  m_local_delta_time = 2*m_delay;

  //minimal non local delay
  m_no_local_delta_time = 4*m_delay;

  // extension payload token
  m_extension_token = new soclib_payload_extension();

  //PDES local time
  m_pdes_local_time = new pdes_local_time(sc_core::SC_ZERO_TIME);

  if(m_is_local_crossbar){
    //create token in the beginning of simulation
    create_token();
  }

  // register thread process
  SC_THREAD(behavior);                  
}

/////////////////////////////////////////////////////////////////////////////////////
// Fuctions
/////////////////////////////////////////////////////////////////////////////////////
tmpl(uint32_t)::getLocalMsgCounter(){
  return m_local_msg_count;
}

tmpl(uint32_t)::getNonLocalMsgCounter(){
  return m_non_local_msg_count;
}

tmpl(uint32_t)::getTokenMsgCounter(){
  return m_token_msg_count;
}

tmpl(void)::print(){
  uint32_t local_msg_count     = getLocalMsgCounter();
  uint32_t non_local_msg_count = getNonLocalMsgCounter();
  uint32_t token_msg_count     = getTokenMsgCounter();
  uint32_t total_count         = local_msg_count + non_local_msg_count + token_msg_count;
 
  std::cout << "[" << name() << "] Total messages       = " << total_count << std::endl;
  std::cout << "Total of Local Messages     = " << local_msg_count << " " << (local_msg_count*100)/total_count << "%" << std::endl;
  std::cout << "Total of Non Local Messages = " << non_local_msg_count << " " << (non_local_msg_count*100)/total_count << "%" << std::endl;
  std::cout << "Total of Token Messages     = " << token_msg_count << " " << (token_msg_count*100)/total_count << "%" << std::endl;

}

tmpl(void)::routing
( size_t                       from,               // port source
  tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{

  bool send = false;
  int dest = 0;

#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] ROUTING from = %d\n", name(), from);
#endif

  //get payload extension
  soclib_payload_extension *extension_ptr;
  payload.get_extension(extension_ptr);

  assert(!(time<m_pdes_local_time->get()) && "Transaction time must not be smaller than the interconnect time");
  m_pdes_local_time->set(time);

  //if transaction command is active or inactive command then
  //the source is actived or desactived and the transaction is not sent
  if(extension_ptr->is_active() || extension_ptr->is_inactive()){
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] ROUTING from = %d ACTIVE OR INACTIVE MSG\n", name(), from);
#endif
    send = false;
    m_centralized_buffer.set_activity(from, extension_ptr->is_active());
  }
  //if transaction command is a token then it must be sent to the target[from]
  else if(extension_ptr->is_token_message()){
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] ROUTING from = %d TOKEN MSG time = %d\n", name(), from, (int)time.value());
#endif

    //set the delta_time which this init wont send another message
    m_centralized_buffer.set_delta_time(from, time);

    send = true;

    m_msg_count++;
    m_token_msg_count++;

    if(m_is_local_crossbar){
      dest = m_targets - 1;
      extension_ptr->set_pkt_id(extension_ptr->get_pkt_id()+1);
    }
    else{
      dest = from;
    }

  }
  //if transaction command is a null message then it must not be sent
  else if(extension_ptr->is_null_message()){
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] ROUTING from = %d NULL MSG time = %d\n", name(), from, (int)time.value());
#endif

    //set the delta_time which this init wont send another message
    m_centralized_buffer.set_delta_time(from, time);

    (*p_to_initiator[from])->nb_transport_bw(payload, phase, time);

    send = false;
  }
  //if transaction has a valid command then it must be sent to appropriated target
  else{
    send = true;
    //IF LOCAL_CROSSBAR
    if(m_is_local_crossbar){
      //if target is NOT local then ...
      if (!m_locality_table[payload.get_address()]){
#ifdef SOCLIB_MODULE_DEBUG
	printf("[%s] ROUTING from = %d NOT LOCAL MSG address = %x\n", name(), from, (int)payload.get_address());
#endif
	
	if(from == m_centralized_buffer.get_nslots()-1){
	  //set the delta_time which this init wont send another message
	  m_centralized_buffer.set_delta_time(from, time);
	}
	else{
	  //set the delta_time which this init wont send another message
	  m_centralized_buffer.set_delta_time(from, time + (m_no_local_delta_time*UNIT_TIME));
	}
	
	m_msg_count++;
	m_non_local_msg_count++;
	dest = m_targets - 1;
      }
      //if target IS local then the transaction is sent to appropriated target
      else{
#ifdef SOCLIB_MODULE_DEBUG
	printf("[%s] ROUTING from = %d LOCAL MSG\n", name(), from);
#endif
	
	if(from == m_centralized_buffer.get_nslots()-1){
	  //set the delta_time which this init wont send another message
	  m_centralized_buffer.set_delta_time(from, time);
	}
	else{
	  //set the delta_time which this init wont send another message
	  m_centralized_buffer.set_delta_time(from, time + (m_local_delta_time*UNIT_TIME));
	  m_msg_count++;
	  m_local_msg_count++;
	}
	
	dest = m_routing_table[payload.get_address()];
	assert( dest >= 0 && dest < m_targets );
      }
    }
    //IF GLOBAL INTERCONNECT
    else{
#ifdef SOCLIB_MODULE_DEBUG
      printf("[%s] ROUTING from = %d LOCAL MSG\n", name(), from);
#endif
      
      //set the delta_time which this init wont send another message
      m_centralized_buffer.set_delta_time(from, time);
      dest = m_routing_table[payload.get_address()];
      assert( dest >= 0 && dest < m_targets );
	
      m_msg_count++;
      m_local_msg_count++;
    }
  }

  if(send){
    time = time + (m_delay*UNIT_TIME);
    
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] SEND to %d src_id = %d pkt_id = %d time = %d\n", name(), dest, extension_ptr->get_src_id(), extension_ptr->get_pkt_id(),(int)time.value());
#endif
    (*p_to_target[dest])->nb_transport_fw(payload, phase, time);
  }
}
  
tmpl(void)::create_token(){
  // create token message in beginning of simulation
  m_extension_token->set_token_message();
  m_extension_token->set_src_id(m_id);
  m_extension_token->set_pkt_id(0);
  m_payload_token.set_extension(m_extension_token);
  m_phase_token = tlm::BEGIN_REQ;
  m_time_token = UNIT_TIME;

#ifdef SOCLIB_MODULE_DEBUG
  printf("[%s] send Token time = %d\n", name(), (int)m_time_token.value());
#endif

  //push a token in the centralized buffer
  m_centralized_buffer.push(m_inits-1, m_payload_token, m_phase_token, m_time_token);
#ifdef SOCLIB_MODULE_DEBUG
  printf("[%s] send Token time = %d\n", name(), (int)m_time_token.value());
#endif
}

/////////////////////////////////////////////////////////////////////////////////////
// THREAD
/////////////////////////////////////////////////////////////////////////////////////
tmpl(void)::behavior()   // initiator thread
{
  size_t                    from;
  tlm::tlm_generic_payload *payload_ptr;
  tlm::tlm_phase           *phase_ptr;
  sc_core::sc_time         *time_ptr;

  while (true){
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] WHILE CONSUMER\n", name());
#endif

    //pop the selected transaction from centralized buffer
    while(m_centralized_buffer.pop(from, payload_ptr, phase_ptr, time_ptr)){
      //put in correct target
      routing(from, *payload_ptr, *phase_ptr, *time_ptr);
      m_pop_count++;
    }
    
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] CONSUMER WAITING id = %d\n", name(), from);
#endif

    m_waiting_from = from;
    sc_core::wait(sc_core::SC_ZERO_TIME);
    m_waiting_from = -1;

#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] CONSUMER WAKE-UP\n", name());
#endif
  }//end while thread
}
    
/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw   
( int                          id,                 // register id
  tlm::tlm_generic_payload    &payload,            // payload
  tlm::tlm_phase              &phase,              // phase
  sc_core::sc_time            &time)               // time
{
  bool push = false;
  int try_push = 0;
  do{
#ifdef SOCLIB_MODULE_DEBUG
  printf("[%s] PUSH id = %d\n", name(), id);
#endif


    //push a transaction in the centralized buffer
    push = m_centralized_buffer.push(id, payload, phase, time);

    //assert(push && "NOT PUSH");
    if(!push){
      try_push++;
#ifdef SOCLIB_MODULE_DEBUG
      printf("[%s] PRODUCER id = %d <<<<<<<<< NOT PUSH >>>>>>>> try_push = %d \n",name(),id, try_push);
#endif
      sc_core::wait(sc_core::SC_ZERO_TIME);
    }
  }while(!push);

#ifdef SOCLIB_MODULE_DEBUG
  printf("[%s] PRODUCER id = %d PUSH CONSUMER WAITING from %d\n", name(), id, m_waiting_from);
#endif
 
#ifdef SOCLIB_MODULE_DEBUG
    printf("[%s] PRODUCER id = %d NOTIFY\n", name(), id);
#endif

  return  tlm::TLM_COMPLETED;

} //end nb_transport_fw

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_bw 
( int                        id,                  // register id
  tlm::tlm_generic_payload   &payload,            // payload
  tlm::tlm_phase             &phase,              // phase
  sc_core::sc_time           &time)               // time
{
#ifdef SOCLIB_MODULE_DEBUG
  printf("[%s] id = %d RECEIVE RESPONSE\n", name(), id);
#endif

  //get extension payload
  soclib_payload_extension *resp_extension_ptr;
  payload.get_extension(resp_extension_ptr);

  //get message source 
  unsigned int src = resp_extension_ptr->get_src_id();
  
  if(m_is_local_crossbar){
    //if source IS NOT local (only possible in vci_local_crossbar module)
    if (!m_resp_locality_table[src]){
      src = m_inits - 1;
    }
    // if source IS local
    else{
      src = m_resp_routing_table[src];
    }
  }
  else{
    src = m_resp_routing_table[src];
  }

  //updated the transaction time
  time = time + (m_delay*UNIT_TIME);
  
#ifdef SOCLIB_MODULE_DEBUG
  printf("[%s] SEND RESPONSE to %d src_id = %d pkt_id = %d time = %d\n", name(), src, resp_extension_ptr->get_src_id(), resp_extension_ptr->get_pkt_id(),(int)time.value());
#endif

  (*p_to_initiator[src])->nb_transport_bw(payload, phase, time);
  
  return tlm::TLM_COMPLETED;
} // end nb_transport_bw 

}}
