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
 * Maintainers: fpecheux, alinev
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#include "vci_multi_tty.h"

#ifndef MULTI_TTY_DEBUG
#define MULTI_TTY_DEBUG 0
#endif

namespace soclib { namespace tlmdt {

#define tmpl(x) template<typename vci_param> x VciMultiTty<vci_param>

tmpl(/**/)::VciMultiTty
( sc_core::sc_module_name name,
  const soclib::common::IntTab &index,
  const soclib::common::MappingTable &mt,
  const char *first_name,
  ...)
  : sc_core::sc_module(name),
    m_index(index),
    m_mt(mt),
    p_vci_target("socket")
{
  // bind target
  p_vci_target(*this);                     
  
  va_list va_tty;
  va_start (va_tty, first_name);
  std::vector<std::string> names;
  const char *cur_tty = first_name;
  while (cur_tty) {
    names.push_back(cur_tty);
    cur_tty = va_arg( va_tty, char * );
  }
  va_end( va_tty );
  init(names);
}

tmpl(/**/)::VciMultiTty
( sc_core::sc_module_name name,
  const soclib::common::IntTab &index,
  const soclib::common::MappingTable &mt,
  const std::vector<std::string> &names)
  : sc_core::sc_module(name),
    m_index(index),
    m_mt(mt),
    p_vci_target("socket")
{
  // bind target
  p_vci_target(*this);                     
  
  init(names);
}

tmpl(void)::init(const std::vector<std::string> &names){
  segList=m_mt.getSegmentList(m_index);
  int j=0;

  for(std::vector<std::string>::const_iterator i = names.begin(); i != names.end();++i){
    m_term.push_back(soclib::common::allocateTty(*i));
    /*
    std::ostringstream irq_name;
    irq_name << "irq" << j;
    p_irq_initiator.push_back(new tlm_utils::simple_initiator_socket_tagged<VciMultiTty,32,tlm::tlm_base_protocol_types>(irq_name.str().c_str()));
    */
    j++;
  }
  
  m_n_irq = j;
  
  m_cpt_cycle = 0;
  m_cpt_idle  = 0;
  m_cpt_read  = 0;
  m_cpt_write = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw 
( tlm::tlm_generic_payload &payload, // payload
  tlm::tlm_phase           &phase,   // phase
  sc_core::sc_time         &time)    // time
{
  soclib_payload_extension *extension_pointer;
  payload.get_extension(extension_pointer);
  
  //this target does not treat the null message
  if(extension_pointer->is_null_message()){
#if MULTI_TTY_DEBUG
	std::cout << "[TTY] Receive NULL MESSAGE time = "  << time.value() << std::endl;
#endif
    return tlm::TLM_COMPLETED;
  }
  
  int cell, reg, term_no;
  uint32_t nwords = payload.get_data_length() / vci_param::nbytes;
  
  std::list<soclib::common::Segment>::iterator seg;
  size_t segIndex;
  for (segIndex=0,seg = segList.begin();seg != segList.end(); ++segIndex, ++seg ){
    soclib::common::Segment &s = *seg;
    
    if (!s.contains(payload.get_address()))
      continue;
    switch(extension_pointer->get_command()){
    case VCI_READ_COMMAND:
      {
#if MULTI_TTY_DEBUG
	std::cout << "[TTY] Receive a read packet with time = "  << time.value() << std::endl;
#endif
	
	m_cpt_idle  = m_cpt_idle + (time.value() - m_cpt_cycle);

	for(unsigned int i=0; i<nwords; i++){
	  
	  //if (payload.contig)
	  cell = (int)(((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes); // XXX contig = true always
	    //else
	    //cell = (int)((payload.address - s.baseAddress()) / vci_param::nbytes); // always write in the same address
	  
	  reg = cell % TTY_SPAN;
	  term_no = cell / TTY_SPAN;
	  
#if MULTI_TTY_DEBUG
	  std::cout << "[TTY] term_no=" << term_no << " reg=" << reg << std::endl;
#endif
	  
	  if (term_no>=(int)m_term.size()){
	    
#if MULTI_TTY_DEBUG
	    std::cout << "term_no (" << term_no <<") greater than the maximum (" << m_term.size() << ")" << std::endl;
#endif
			    
	    // remplir paquet d'erreur
	    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
	  }
	  else{
	    
	    switch (reg) {
	    case TTY_STATUS:

	      utoa(m_term[term_no]->hasData(), payload.get_data_ptr(),(i * vci_param::nbytes));
 	      payload.set_response_status(tlm::TLM_OK_RESPONSE);
 	      break;
	    case TTY_READ:
	      if (m_term[term_no]->hasData()) {
		char tmp = m_term[term_no]->getc();
		utoa(tmp, payload.get_data_ptr(),(i * vci_param::nbytes));
	      }	
	      payload.set_response_status(tlm::TLM_OK_RESPONSE);
 	      break;
	    default:
	      //error message
	      payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
 	      break;
	    }
	  }
	}
			
	phase = tlm::BEGIN_RESP;
	time = time + (nwords * UNIT_TIME);
	
	m_cpt_cycle = time.value();
	m_cpt_read+=nwords;
	
#if MULTI_TTY_DEBUG
	std::cout << "[TTY] Send answer with time = " << time.value() << std::endl;
#endif
	
        p_vci_target->nb_transport_bw(payload, phase, time);
 	return tlm::TLM_COMPLETED;
	break;
      }
    case VCI_WRITE_COMMAND:
      {
	char data;
	
#if MULTI_TTY_DEBUG
	std::cout << "[TTY] Receive a write packet with time = "  << time.value() << std::endl;
#endif
	
	for(unsigned int i=0; i<nwords; i++){
	  //if (payload.contig)
	  cell = (int)(((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes); /// XXX contig = true always
	    //else
	    //cell = (int)((payload.address - s.baseAddress()) / vci_param::nbytes); // always write in the same address
	  
	  reg = cell % TTY_SPAN;
	  term_no = cell / TTY_SPAN;


          data = atou(payload.get_data_ptr(), (i * vci_param::nbytes));
	  
#if MULTI_TTY_DEBUG
	  std::cout << "[TTY] term_no=" << term_no << " reg=" << reg << " data=" << data << std::endl;
#endif
	  
	  if (term_no>=(int)m_term.size()){
#if MULTI_TTY_DEBUG
	    std::cout << "term_no (" << term_no <<") greater than the maximum (" << m_term.size() << ")" << std::endl;
#endif
	    
	    // remplir paquet d'erreur
	    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
 	  }
	  else{
	    
	    switch (reg) {
	    case TTY_WRITE:
	      if ( data == '\a' ) {
		char tmp[32];
		size_t ret = snprintf(tmp, sizeof(tmp), "[%d] ", (int)time.value());
		
		for ( size_t i=0; i<ret; ++i )
		  m_term[term_no]->putc( tmp[i] );
	      } 
	      else
		m_term[term_no]->putc( data );
		
              payload.set_response_status(tlm::TLM_OK_RESPONSE);
 	      break;
	    default:
	      //error message
              payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
 	      break;
	    }
	  }
	}
	
	m_cpt_idle  = m_cpt_idle + (time.value() - m_cpt_cycle);

	phase = tlm::BEGIN_RESP;
	time = time + (nwords * UNIT_TIME);

	m_cpt_cycle = time.value();
	m_cpt_write+=nwords;
	
#if MULTI_TTY_DEBUG
	std::cout << "[TTY] Send answer with time = " << time.value() << std::endl;
#endif
	
	p_vci_target->nb_transport_bw(payload, phase, time);
 	return tlm::TLM_COMPLETED;
	break;
      }
    default:
      break;
    }
  }

  //send error message
  payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
   
  phase  = tlm::BEGIN_RESP;
  time = time + UNIT_TIME;
  
#if MULTI_TTY_DEBUG
  std::cout << "[TTY] Address " << payload.get_address() << " does not match any segment " << std::endl;
  std::cout << "[TTY] Send to source "<< extension_pointer->get_src_id() << " a error packet with time = "  << time.value() << std::endl;
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
// local Fuctions
/////////////////////////////////////////////////////////////////////////////////////
tmpl(size_t)::getTotalCycles(){
  return m_cpt_cycle;
}

tmpl(size_t)::getActiveCycles(){
  return (m_cpt_cycle - m_cpt_idle);
}

tmpl(size_t)::getIdleCycles(){
  return m_cpt_idle;
}

tmpl(size_t)::getNRead(){
  return m_cpt_read;
}

tmpl(size_t)::getNWrite(){
  return m_cpt_write;
}

tmpl(void)::print_stats(){
  std::cout << name() << std::endl;
  std::cout << "- READ               = " << m_cpt_read << std::endl;
  std::cout << "- WRITE              = " << m_cpt_write << std::endl;
}


}}
