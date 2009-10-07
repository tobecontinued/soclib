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

#include "vci_icu.h"

//ICU_INT = 0 read-only
//ICU_MASK = 1 read-only
//ICU_MASK_SET = 2 write-only
//ICU_MASK_CLEAR = 3 write-only
//ICU_IT_VECTOR = 4 read-only

template<typename T>
T be2mask(T be)
{
	const T m = (1<<sizeof(T));
	T r = 0;

	for ( size_t i=0; i<sizeof(T); ++i ) {
		r <<= 8;
		be <<= 1;
		if ( be & m )
			r |= 0xff;
	}
	return r;
}

namespace soclib { namespace tlmdt {

#define tmpl(x) template<typename vci_param> x VciIcu<vci_param>

tmpl(/**/)::VciIcu
( sc_core::sc_module_name name,
  const soclib::common::IntTab &index,
  const soclib::common::MappingTable &mt,
  size_t nirq )
	   : sc_module(name),                  // module name
	   m_index(index),
	   m_mt(mt),
	   m_nirq(nirq),
	   p_vci_target("vci_target_socket"),  // vci target socket name
	   p_irq_initiator("irq_init_socket")  // irq initiator socket name
{
  // bind VCI TARGET SOCKET
  p_vci_target(*this);                     

  // bind IRQ INITIATOR SOCKET
  p_irq_initiator(*this);                     

  //maximum number of interruption equal 32
  if (m_nirq >= 32)
    m_nirq = 32;
      
  m_segments = m_mt.getSegmentList(m_index);
  
  r_mask = be2mask<data_t>(0x0);
  r_current = be2mask<data_t>(0x0);
  
  irq = new irq_struct[m_nirq];
  for(size_t i=0; i<m_nirq; i++){
    irq[i].val  = false;
    irq[i].time = sc_core::SC_ZERO_TIME;
    
    std::ostringstream irq_name;
    irq_name << "irqIn" << i;
    p_irq_target.push_back(new tlm_utils::simple_target_socket_tagged<VciIcu,32,tlm::tlm_base_protocol_types>(irq_name.str().c_str()));
    
    p_irq_target[i]->register_nb_transport_fw(this, &VciIcu::irq_nb_transport_fw, i);
    
  }
}
    
tmpl(/**/)::~VciIcu(){}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if VCI TARGET SOCKET
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw
( tlm::tlm_generic_payload &payload,
  tlm::tlm_phase           &phase,  
  sc_core::sc_time         &time)   
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

  data_t nwords = (data_t)(payload.get_data_length() / vci_param::nbytes);

  for (segIndex=0,seg = m_segments.begin(); seg != m_segments.end(); ++segIndex, ++seg ) {
    soclib::common::Segment &s = *seg;
    if (!s.contains(payload.get_address()))
      continue;
    
    switch(extension_pointer->get_command()){
    case VCI_READ_COMMAND:
      {
#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Receive a read packet with time = "  << time.value() << std::endl;
#endif
	int reg;
	for (data_t i=0;i<nwords;i++){
	  //if (payload.contig) 
	  reg = (int)((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress())/ vci_param::nbytes; //XXX contig = TRUE always
	  //else
	  //reg = (int)(payload.get_address()- s.baseAddress())/ vci_param::nbytes; //always the same address

    
	  switch (reg) {
	  case ICU_INT:
	    utoa(getActiveInterruptions(time), payload.get_data_ptr(),(i * vci_param::nbytes));
	    payload.set_response_status(tlm::TLM_OK_RESPONSE);
	    break;
	    
	  case ICU_MASK:
	    utoa(r_mask, payload.get_data_ptr(),(i * vci_param::nbytes));
	    payload.set_response_status(tlm::TLM_OK_RESPONSE);
	    break;
	  
	  case ICU_IT_VECTOR:
	    // give the highest priority interrupt
	    utoa(r_current, payload.get_data_ptr(),(i * vci_param::nbytes));
	    payload.set_response_status(tlm::TLM_OK_RESPONSE);
	    break;
	  default:
	    //send error message
	    payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
	    break;
	  }
	}
      
	phase = tlm::BEGIN_RESP;
        time = time + (nwords * UNIT_TIME);

#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() <<"] Send Answer Time = " << time.value() << std::endl;
#endif
        p_vci_target->nb_transport_bw(payload, phase, time);
        return tlm::TLM_COMPLETED;
      }
      break;
    case VCI_WRITE_COMMAND:
      {
#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() <<"] Receive a write packet with time = "  << time.value() << std::endl;
#endif
	int reg;
	for (data_t i=0; i<nwords; i++){
	  //if (payload.contig) 
	  reg = (int)((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress())/ vci_param::nbytes; //XXX contig = TRUE always
	  //else
	  //reg = (int)(payload.get_address()- s.baseAddress())/ vci_param::nbytes; //always the same address

	  data_t data = atou(payload.get_data_ptr(), (i * vci_param::nbytes));
   
	  switch (reg) {
	  case ICU_MASK_SET:
	    r_mask = r_mask | data;
#if SOCLIB_MODULE_DEBUG
	    printf("[%s] ICU_MASK_SET data = %08x  mask = %08x time = %d\n", name(), data, r_mask, (int)time.value());
#endif
	    payload.set_response_status(tlm::TLM_OK_RESPONSE);
	    break;
	  case ICU_MASK_CLEAR:
	    r_mask = r_mask & ~(data);
#if SOCLIB_MODULE_DEBUG
	    printf("[%s] ICU_MASK_CLEAR data = %08x mask = %08x time = %d\n", name(), data, r_mask, (int)time.value());
#endif
	    disable_interruption(data, time);
	    payload.set_response_status(tlm::TLM_OK_RESPONSE);
	    break;
	  default:
	    //send error message
	    payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
	    break;
	  }
	}

	phase = tlm::BEGIN_RESP;
	time = time + (nwords * UNIT_TIME);
 	
#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Send answer with time = " << time.value() << std::endl;
#endif
	p_vci_target->nb_transport_bw(payload, phase, time);
	return tlm::TLM_COMPLETED;

       }
      break;
    default:
      break;
    }
  }
  
  //send error message
  payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  
  phase = tlm::BEGIN_RESP;
  time = time + nwords * UNIT_TIME;
  
#if SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Address " << std::hex << payload.get_address() << std::dec << " does not match any segment " << std::endl;
  std::cout << "[" << name() << "] Send a error packet with time = "  << time.value() << std::endl;
#endif

  p_vci_target->nb_transport_bw(payload, phase, time);
  return tlm::TLM_COMPLETED;
}

// Not implemented for this example but required by interface
tmpl(void)::b_transport
( tlm::tlm_generic_payload &payload,                // payload
  sc_core::sc_time         &_time)                  //time
{
  return;
}

// Not implemented for this example but required by interface
tmpl(bool)::get_direct_mem_ptr
( tlm::tlm_generic_payload &payload,                // address + extensions
  tlm::tlm_dmi             &dmi_data)               // DMI data
{ 
  return false;
}

// Not implemented for this example but required by interface
tmpl(unsigned int):: transport_dbg                            
( tlm::tlm_generic_payload &payload)                // debug payload
{
  return false;
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (IRQ INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
// Not implemented for this example but required by interface
tmpl(tlm::tlm_sync_enum)::nb_transport_bw           // receive response from target
( tlm::tlm_generic_payload &payload,                // payload
  tlm::tlm_phase           &phase,                  // phase
  sc_core::sc_time         &time)                   // time
{
  return tlm::TLM_COMPLETED;
}

// Not implemented for this example but required by interface
tmpl(void)::invalidate_direct_mem_ptr               // invalidate_direct_mem_ptr
( sc_dt::uint64 start_range,                        // start range
  sc_dt::uint64 end_range                           // end range
) 
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (IRQ TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::irq_nb_transport_fw
( int                      id,         // interruption id
  tlm::tlm_generic_payload &payload,   // payload
  tlm::tlm_phase           &phase,     // phase
  sc_core::sc_time         &time)      // time
{
  irq[id].val  = (bool) atou(payload.get_data_ptr(), 0);
  irq[id].time = time;

#if SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Receive Interruption " << id << " val = " << irq[id].val << " with time = " << irq[id].time.value() << std::endl;
#endif

  data_t mask = 0;
  //verify if the interruption is r_mask=true
  mask = (1 << id);
  if((r_mask & mask) == mask){
    send_interruption(id);
  }
  return tlm::TLM_COMPLETED;
}

tmpl(void)::send_interruption(int idx)
{
#if SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Send Interruption " << idx << " with time = " << irq[idx].time.value() << std::endl;
#endif
  r_current = idx;
      
  // set the values in irq tlm payload
  data_t nwords= 1;
  data_t nbytes= nwords * vci_param::nbytes;
  data_t byte_enable = be2mask<data_t>(0xF);
  unsigned char data_ptr[nbytes];
  unsigned char byte_enable_ptr[nbytes];
  
  utoa(byte_enable, byte_enable_ptr, 0);
  utoa(irq[idx].val, data_ptr, 0);
  
  // set the values in irq tlm payload
  m_irq_payload.set_byte_enable_ptr(byte_enable_ptr);
  m_irq_payload.set_byte_enable_length(nbytes);
  m_irq_payload.set_data_ptr(data_ptr);
  m_irq_payload.set_data_length(nbytes);
  
  // set the tlm phase
  m_irq_phase = tlm::BEGIN_REQ;
  // set the local time to transaction time
  m_irq_time = irq[idx].time;
  // send the transaction
  p_irq_initiator->nb_transport_fw(m_irq_payload, m_irq_phase, m_irq_time);
}

tmpl(void)::disable_interruption(data_t mask, sc_core::sc_time t){
  data_t m;
  
  for(size_t i=0;i<m_nirq;i++){
    m = 0;
    //verify if the interruption is mask=true
    m = (1 << i);
    if((mask & m) == m){
      if(irq[i].val == 1 && irq[i].time > t){
#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Disable Interruption " << i << std::endl;
#endif
	irq[i].val = 0;
	send_interruption(i);
      }
    }
  }
}

tmpl(typename vci_param::data_t)::getActiveInterruptions(sc_core::sc_time time){
  data_t r_interrupt = be2mask<data_t>(0x0);
  
  // starting with interruption with higher priority
  for (size_t j=0;j<m_nirq;j++) {
    // If the interruption is active and time is greater or equals to m_fifos_time[j]
    if (irq[j].val && time.value() >= irq[j].time.value()){
      r_interrupt = r_interrupt | (1 << j);
    }
  }

  return r_interrupt;
}

}}


