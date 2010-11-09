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
#include "vci_timer.h"

namespace soclib { namespace tlmdt {

#define tmpl(x) template<typename vci_param> x VciTimer<vci_param>

tmpl(/**/)::VciTimer
( sc_core::sc_module_name name,
  const soclib::common::IntTab &index,
  const soclib::common::MappingTable &mt,
  size_t ntimer)
	   : sc_module(name),
	   m_index(index),
	   m_mt(mt),
	   m_ntimer(ntimer),
	   p_vci("vci_target_socket")  // vci target socket name
{
  // bind target
  p_vci(*this);                     

  //segments
  m_segments = m_mt.getSegmentList(m_index);
  
  //timers
  m_timer = new timer_struct[m_ntimer];
  for(unsigned int i=0;i<m_ntimer;i++){
    m_timer[i].period     = sc_core::SC_ZERO_TIME;
    m_timer[i].value      = sc_core::SC_ZERO_TIME;
    m_timer[i].mode       = 0;
    m_timer[i].irq        = sc_core::SC_ZERO_TIME;
    m_timer[i].activation = sc_core::SC_ZERO_TIME;
    m_timer[i].counter    = sc_core::SC_ZERO_TIME;
   
    std::ostringstream irq_name;
    irq_name << "irq" << i;
    p_irq.push_back(new tlm_utils::simple_initiator_socket_tagged<VciTimer,32,tlm::tlm_base_protocol_types>(irq_name.str().c_str()));
  }

  //create payload to irq message
  m_irq_payload_ptr = new tlm::tlm_generic_payload();
}

tmpl(/**/)::~VciTimer(){}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if VCI TARGET SOCKET
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw     // receive command from initiator
( tlm::tlm_generic_payload &payload,          // payload
  tlm::tlm_phase           &phase,            // phase  
  sc_core::sc_time         &time)             // time  
{
  soclib_payload_extension *extension_pointer;
  payload.get_extension(extension_pointer);

  //this target does not treat the null message
  if(extension_pointer->is_null_message()){
    return tlm::TLM_COMPLETED;
  }

  // First, find the right segment using the first address of the packet
  std::list<soclib::common::Segment>::iterator seg;	
  size_t segIndex;

  uint32_t nwords = (uint32_t)(payload.get_data_length() / vci_param::nbytes);

  for (segIndex=0,seg = m_segments.begin(); seg != m_segments.end(); ++segIndex, ++seg ) {
    soclib::common::Segment &s = *seg;
    if (!s.contains(payload.get_address()))
      continue;
    
    switch(extension_pointer->get_command()){
    case VCI_READ_COMMAND:
      {
#if SOCLIB_MODULE_DEBUG
	//std::cout << "[" << name() << "] Receive a read packet with time = "  << time.value() << std::endl;
#endif
    
	int cell, reg, t;
	
	for (size_t i=0; i<nwords; i++){
	  //if(pkt->contig)
	  cell = (int)(payload.get_address() + (i*vci_param::nbytes) - s.baseAddress()) / vci_param::nbytes; //XXX contig = TRUE always
	  //else
	  //cell = (int)(payload.get_address() - s.baseAddress()) / vci_param::nbytes; //always the same address
	  
	  reg = cell % TIMER_SPAN;
	  t = cell / TIMER_SPAN;
	  
	  if (t>=(int)m_ntimer){
#if SOCLIB_MODULE_DEBUG
	    std::cout << " t value bigger than the acceptable  t = " << t << std::endl;
#endif

	    //send error message
	    payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
	    
	    phase = tlm::BEGIN_RESP;
	    time = time + (nwords * UNIT_TIME);
	    p_vci->nb_transport_bw(payload, phase, time);
	    return tlm::TLM_COMPLETED;
	  }
      
	  switch (reg) {
	  case TIMER_VALUE:
	    if(m_timer[t].mode & TIMER_RUNNING){
	      m_timer[t].value += time - m_timer[t].activation;
	      m_timer[t].activation = time;
	    }

	    utoa(m_timer[t].value.value(), payload.get_data_ptr(),(i * vci_param::nbytes));
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Read Timer " << t << " Value "  << std::dec << m_timer[t].value.value() << " time = "<< (time.value() + 1) << std::endl;
#endif
	    break;
	    
	  case TIMER_PERIOD:
	    utoa(m_timer[t].period.value(), payload.get_data_ptr(),(i * vci_param::nbytes));
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Read Timer " << t << " Period "  << std::dec << m_timer[t].period.value() << " time = "<< (time.value() + 1) << std::endl;
#endif
	    break;
	    
	  case TIMER_MODE:
	    utoa(m_timer[t].mode, payload.get_data_ptr(),(i * vci_param::nbytes));
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Read Timer " << t << " Mode "  << std::dec << m_timer[t].mode << " time = "<< (time.value() + 1) << std::endl;
#endif
	    break;
	    
	  case TIMER_RESETIRQ:
	    if(time >= m_timer[t].irq){
	      utoa(1, payload.get_data_ptr(),(i * vci_param::nbytes));
#if SOCLIB_MODULE_DEBUG
	      std::cout << "[" << name() << "] Read Timer " << t << " ResetIRQ 1 time = "<< (time.value() + 1) << std::endl;
#endif
	    }
	    else{
	      utoa(0, payload.get_data_ptr(),(i * vci_param::nbytes));
#if SOCLIB_MODULE_DEBUG
	      std::cout << "[" << name() << "] Read Timer " << t << " ResetIRQ 0 time = "<< time.value() << std::endl;
#endif
	    }
	    break;
	  }
	}

	payload.set_response_status(tlm::TLM_OK_RESPONSE);
	phase = tlm::BEGIN_RESP;
        time = time + (nwords * UNIT_TIME);

#if SOCLIB_MODULE_DEBUG
	//std::cout << "[" << name() << "] Send answer with time = " << time.value() << std::endl;
#endif

        p_vci->nb_transport_bw(payload, phase, time);
        return tlm::TLM_COMPLETED;
      }
      break;
    case VCI_WRITE_COMMAND:
      {    
#if SOCLIB_MODULE_DEBUG
	//std::cout << "[" << name() << "] Receive a write packet with time = "  << time.value() << std::endl;
#endif
	int cell, reg, t;

	// set the values in irq tlm payload
	uint32_t nwords= 1;
	uint32_t nbytes= nwords * vci_param::nbytes;
	uint32_t byte_enable = 0xFFFFFFFF;
	unsigned char data_ptr[nbytes];
	unsigned char byte_enable_ptr[nbytes];

	for (size_t i=0; i<nwords; i++){
	  //if(pkt->contig)
	  cell = (int)(payload.get_address() + (i*vci_param::nbytes) - s.baseAddress()) / vci_param::nbytes; //XXX contig = TRUE always
	  //else
	  //cell = (int)(payload.get_address() - s.baseAddress()) / vci_param::nbytes; //always the same address
	  
	  reg = cell % TIMER_SPAN;
	  t = cell / TIMER_SPAN;
    
	  if (t>=(int)m_ntimer){
#if SOCLIB_MODULE_DEBUG
	    std::cout << " t value bigger than the acceptable  t = " << t << std::endl;
#endif
	    //send error message
	    payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
	    
	    phase = tlm::BEGIN_RESP;
	    time = time + (nwords * UNIT_TIME);
	    p_vci->nb_transport_bw(payload, phase, time);
	    return tlm::TLM_COMPLETED;
	  }
      
	  switch (reg) {
	  case TIMER_VALUE:
	    m_timer[t].value = atou(payload.get_data_ptr(), (i*vci_param::nbytes)) * UNIT_TIME;

#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Write Timer " << t << " Value" << std::dec << m_timer[t].value.value() << " time = "<< (time.value() + 1) << std::endl;
#endif
	    break;
	
	  case TIMER_RESETIRQ:
	    //desactive the interruption
	    utoa(byte_enable, byte_enable_ptr, 0);
	    utoa(0, data_ptr, 0);
	    
	    // set the values in irq tlm payload
	    m_irq_payload_ptr->set_byte_enable_ptr(byte_enable_ptr);
	    m_irq_payload_ptr->set_byte_enable_length(nbytes);
	    m_irq_payload_ptr->set_data_ptr(data_ptr);
	    m_irq_payload_ptr->set_data_length(nbytes);
	    
	    // set the tlm phase
	    m_irq_phase = tlm::BEGIN_REQ;
	    // set the local time to transaction time
	    m_irq_time = time;
	    
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Desactive Interruption " << t <<" with time = " << time.value() << std::endl;
#endif
	    
	    // send the transaction
	    (*p_irq[t])->nb_transport_fw(*m_irq_payload_ptr, m_irq_phase, m_irq_time);
	    wait(sc_core::SC_ZERO_TIME);	      
	    
	    //generate a new interruption
	    m_timer[t].irq += m_timer[t].period + UNIT_TIME;
	    
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Write Timer " << t << " ResetIRQ time = "<< (time.value() + 1) << std::endl;
#endif

	    //send a new interruption
	    if((m_timer[t].mode & TIMER_RUNNING) && (m_timer[t].mode & TIMER_IRQ_ENABLED)){
	      
	      utoa(byte_enable, byte_enable_ptr, 0);
	      utoa(1, data_ptr, 0);
	      
	      // set the values in irq tlm payload
	      m_irq_payload_ptr->set_byte_enable_ptr(byte_enable_ptr);
	      m_irq_payload_ptr->set_byte_enable_length(nbytes);
	      m_irq_payload_ptr->set_data_ptr(data_ptr);
	      m_irq_payload_ptr->set_data_length(nbytes);
	      
	      // set the tlm phase
	      m_irq_phase = tlm::BEGIN_REQ;
	      // set the local time to transaction time 
	      // the new interruption cannot have a time inferior than desactivation
	      if(m_timer[t].irq < time)
		m_timer[t].irq = time;
	      
	      m_irq_time = m_timer[t].irq;
		
	      
#if SOCLIB_MODULE_DEBUG
	      std::cout << "[" << name() << "] Send Interruption " << t <<" with time = " << m_timer[t].irq.value() << std::endl;
#endif
	      
	      // send the transaction
	      (*p_irq[t])->nb_transport_fw(*m_irq_payload_ptr, m_irq_phase, m_irq_time);
	      wait(sc_core::SC_ZERO_TIME);	      
	    }
	    break;
	  case TIMER_MODE:
	    m_timer[t].mode = (atou(payload.get_data_ptr(), (i*vci_param::nbytes)) & 0x3);
	    
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Write Timer " << t << " Mode "  << std::dec << m_timer[t].mode << " time = "<< (time.value() + 1) << std::endl;
#endif
	      
	    if(m_timer[t].mode & TIMER_RUNNING){
	      m_timer[t].activation = time + (nwords * UNIT_TIME);
	      
	      if(m_timer[t].mode & TIMER_IRQ_ENABLED){
		m_timer[t].irq = time  + (3 * UNIT_TIME) + m_timer[t].period - m_timer[t].counter;
		utoa(1, data_ptr, 0);
	      }
	      else{
		utoa(0, data_ptr, 0);
	      }
	    }
	    else{
	      m_timer[t].value+= time - m_timer[t].activation + UNIT_TIME;
	      m_timer[t].counter = time - (m_timer[t].irq - m_timer[t].period - (3 * UNIT_TIME));
	      utoa(0, data_ptr, 0);
	    }
	    
	    utoa(byte_enable, byte_enable_ptr, 0);
	    // set the values in irq tlm payload
	    m_irq_payload_ptr->set_byte_enable_ptr(byte_enable_ptr);
	    m_irq_payload_ptr->set_byte_enable_length(nbytes);
	    m_irq_payload_ptr->set_data_ptr(data_ptr);
	    m_irq_payload_ptr->set_data_length(nbytes);
	    
	    // set the tlm phase
	    m_irq_phase = tlm::BEGIN_REQ;
	    // set the local time to transaction time
	    m_irq_time = m_timer[t].irq;
	    
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Send Interruption " << t <<" with time = " << m_timer[t].irq.value() << std::endl;
#endif
	    
	    // send the transaction
	    (*p_irq[t])->nb_transport_fw(*m_irq_payload_ptr, m_irq_phase, m_irq_time);
	    wait(sc_core::SC_ZERO_TIME);	      
	    
	    break;
	  case TIMER_PERIOD:
	    m_timer[t].period = atou(payload.get_data_ptr(), (i*vci_param::nbytes)) * UNIT_TIME;

#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] Write Timer " << t << " Period "  << std::dec << m_timer[t].period.value() << " time = "<< (time.value() + 1) << std::endl;
#endif
	    break;
	  }
	}

	payload.set_response_status(tlm::TLM_OK_RESPONSE);
 	phase = tlm::BEGIN_RESP;
	time = time + (nwords * UNIT_TIME);
    
#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Send answer with time = " << time.value() << std::endl;
#endif

	p_vci->nb_transport_bw(payload, phase, time);
	return tlm::TLM_COMPLETED;
      }
      break;
    default:
      break;
    }//end switch
  }//end for
  
  //send error message
  payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  
  phase = tlm::BEGIN_RESP;
  time = time + (nwords * UNIT_TIME);
  
#if SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Address " << std::hex << payload.get_address() << std::dec << " does not match any segment " << std::endl;
  std::cout << "[" << name() << "] Send a error packet with time = "  << time.value() << std::endl;
#endif
  
  p_vci->nb_transport_bw(payload, phase, time);
  return tlm::TLM_COMPLETED;
}
  
/// Not implemented for this example but required by interface
tmpl(void)::b_transport
( tlm::tlm_generic_payload &payload,                // payload
  sc_core::sc_time         &_time)                  //time
{
  return;
}

/// Not implemented for this example but required by interface
tmpl(bool)::get_direct_mem_ptr
( tlm::tlm_generic_payload &payload,                // address + extensions
  tlm::tlm_dmi             &dmi_data)               // DMI data
{ 
  return false;
}
    
/// Not implemented for this example but required by interface
tmpl(unsigned int):: transport_dbg                            
( tlm::tlm_generic_payload &payload)                // debug payload
{
  return false;
}
  
}}
