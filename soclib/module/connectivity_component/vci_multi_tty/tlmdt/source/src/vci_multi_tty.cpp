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

#define tmpl(x) template<typename vci_param> x VciMultiTty<vci_param>

tmpl(/**/)::VciMultiTty
( sc_core::sc_module_name name,
  const soclib::common::IntTab &index,
  const soclib::common::MappingTable &mt,
  const char *first_name,
  ...)
	   : TlmdtTarget(name)
	  , m_index(index)
	  , m_mt(mt)
{
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
	   : TlmdtTarget(name)
	  , m_index(index)
	  , m_mt(mt)
{
  init(names);
}

tmpl(void)::init(const std::vector<std::string> &names){

  segList=m_mt.getSegmentList(m_index);
  int j=0;

  for(std::vector<std::string>::const_iterator i = names.begin(); i != names.end();++i){
    m_term.push_back(soclib::common::allocateTty(*i));

    std::ostringstream irq_name;
    irq_name << "irq" << j;
    p_irq.push_back(new tlm_utils::simple_initiator_socket_tagged<VciMultiTty,32,tlm::tlm_base_protocol_types>(irq_name.str().c_str()));

    j++;
  }

  m_irq = new bool[m_term.size()];
  for(size_t i=0; i<m_term.size(); i++){
    m_irq[i] = 0;
  }
  
  //PDES local time
  m_pdes_local_time = new pdes_local_time(sc_core::SC_ZERO_TIME);

  //create payload and extension to irq message
  m_payload_ptr = new tlm::tlm_generic_payload();
  m_extension_ptr = new soclib_payload_extension();
  
  m_cpt_cycle = 0;
  m_cpt_idle  = 0;
  m_cpt_read  = 0;
  m_cpt_write = 0;

  SC_THREAD(behavior_target);

}

tmpl(void)::behavior_target()
{
#if SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Behavior target" << std::endl;
#endif

  while (1) {
    for ( size_t i=0; i<m_term.size(); ++i ) {
      bool val = m_term[i]->hasData();
      if ( val != m_irq[i] ) {
	send_interruption(i,val);
      }
    }
    wait(sc_core::SC_ZERO_TIME);
  }
}

tmpl(void)::send_interruption(size_t idx, bool val)
{
  size_t nbytes = vci_param::nbytes; //1 word
  unsigned char data_ptr[nbytes];
  unsigned char byte_enable_ptr[nbytes];
  typename vci_param::data_t be = be2mask<typename vci_param::data_t>(0xF);

  m_irq[idx] = val;

  // set the all bytes to enabled
  utoa(be, byte_enable_ptr, 0);
  // set the val to data
  utoa(val, data_ptr, 0);

  // set the values in tlm payload
  m_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
  m_payload_ptr->set_byte_enable_ptr(byte_enable_ptr);
  m_payload_ptr->set_byte_enable_length(nbytes);
  m_payload_ptr->set_data_ptr(data_ptr);
  m_payload_ptr->set_data_length(nbytes);
  // set the values in payload extension
  m_extension_ptr->set_write();
  // set the extension to tlm payload
  m_payload_ptr->set_extension(m_extension_ptr);
  
  // set the tlm phase
  m_phase = tlm::BEGIN_REQ;
  // set the local time to transaction time
  m_time = m_pdes_local_time->get();
  
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Send Interrupt " << idx << " : " << val << " time : " << m_time.value() << " hex : " << std::hex << m_time.value() << std::dec << std::endl;
#endif
  
  // send the transaction
  (*p_irq[idx])->nb_transport_fw(*m_payload_ptr, m_phase, m_time);
  
}

/////////////////////////////////////////////////////////////////////////////////////
// PROCESSING (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(void)::target_processing
( tlm::tlm_generic_payload &payload,
  tlm::tlm_phase           &phase,  
  sc_core::sc_time         &time)   
{
  soclib_payload_extension *extension_pointer;
  payload.get_extension(extension_pointer);
  
  //this target does not treat the null message
  if(extension_pointer->is_null_message()){
    return;
  }
  
  //update the message time
  if ( time < m_pdes_local_time->get() ){
    time = m_pdes_local_time->get();
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
#ifdef SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() <<"] Receive a read packet with time = "  << time.value() << std::endl;
#endif
	
	m_cpt_idle  = m_cpt_idle + (time.value() - m_cpt_cycle);

	for(unsigned int i=0; i<nwords; i++){
	  
	  //if (payload.contig)
	  cell = (int)(((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes); // XXX contig = true always
	    //else
	    //cell = (int)((payload.address - s.baseAddress()) / vci_param::nbytes); // always write in the same address
	  
	  reg = cell % TTY_SPAN;
	  term_no = cell / TTY_SPAN;
	  
#ifdef SOCLIB_MODULE_DEBUG
	  std::cout << "[" << name() <<"] term_no=" << term_no << " reg=" << reg << std::endl;
#endif
	  
	  if (term_no>=(int)m_term.size()){
	    
#ifdef SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() <<"] term_no (" << term_no <<") greater than the maximum (" << m_term.size() << ")" << std::endl;
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
		//printf("[%d] = %c\n", i, tmp);
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
	
#ifdef SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Send answer with time = " << time.value() << std::endl;
#endif
	
	//update the target local time
	m_pdes_local_time->set(time + UNIT_TIME);

        p_vci->nb_transport_bw(payload, phase, time);
 	return;
	break;
      }
    case VCI_WRITE_COMMAND:
      {
	char data;
	
#ifdef SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Receive a write packet nwords = " << nwords << " with time = "  << time.value() << std::endl;
#endif
	
	for(unsigned int i=0; i<nwords; i++){
	  //if (payload.contig)
	  cell = (int)(((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes); /// XXX contig = true always
	    //else
	    //cell = (int)((payload.address - s.baseAddress()) / vci_param::nbytes); // always write in the same address
	  
	  reg = cell % TTY_SPAN;
	  term_no = cell / TTY_SPAN;


          data = atou(payload.get_data_ptr(), (i * vci_param::nbytes));
	  
#if SOCLIB_MODULE_DEBUG
	  std::cout << "[" << name() << "] term_no=" << term_no << " reg=" << reg << " data=" << data << std::endl;
#endif
	  
	  if (term_no>=(int)m_term.size()){
#if SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] term_no (" << term_no <<") greater than the maximum (" << m_term.size() << ")" << std::endl;
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
	      else{
		//std::cout << "[" << name() << "] data = " << data << std::endl;
		m_term[term_no]->putc( data );
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
	
	m_cpt_idle  = m_cpt_idle + (time.value() - m_cpt_cycle);

	phase = tlm::BEGIN_RESP;
	time = time + (nwords * UNIT_TIME);

	m_cpt_cycle = time.value();
	m_cpt_write+=nwords;
	
#if SOCLIB_MODULE_DEBUG
	std::cout << "[" << name() << "] Send answer with time = " << time.value() << std::endl;
#endif
	
	//update the target local time
	m_pdes_local_time->set(time + UNIT_TIME);

	p_vci->nb_transport_bw(payload, phase, time);
 	return;
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
  
#if SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Address " << payload.get_address() << " does not match any segment " << std::endl;
  std::cout << "[" << name() << "] Send to source "<< extension_pointer->get_src_id() << " a error packet with time = "  << time.value() << std::endl;
#endif
  
  //update the target local time
  m_pdes_local_time->set(time + UNIT_TIME);
    
  p_vci->nb_transport_bw(payload, phase, time);
  return;
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
