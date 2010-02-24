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
 * Maintainers: alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2010
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

/////////////////////////////////////////////////////////////////
// General hypothesis:
//
// 1) The wrapper contains one single thread. 
// This thread is modeling the PDES process associated
// to the VCI target. This thread controls the local time
// of the VCI target, and it behaves as the topcell of the CABA 
// subsystem: It controls all CABA ports of the VCI target,
// including the CK & NRESET ports.
//
// 2) The VCI target has a multi-transactions capability:
// it can accept several VCI commands before sending a
// response, and the response ordering can be different from 
// the commands ordering.
//
// 3) The target thread is activated when it is in the wait state,
// and the the wrapper receives a command on the tlmdt port. 
// As the target supports several simultaneous transactions,
// and the read data must be written in the buffer transported
// in the command, all pending transactions must be registered
// in  transaction table where a transaction is identified
// by the srcid/trdid/pktid triplet.
//
// 4) This table plays the rôle of a pseudo-FIFO between the
// initiator thead (executing the interface function), and
// the target thread.
// When the wrapper receives a command, the interface function
// stores the command in a free slot of the pseudo-FIFO
// and notifies the thread. If the pseudo-FIFO is full,
// the initiator thread is descheduled.
// The target thread handles the pending commands by increasing time
// and send each command sequencially to the CABA VCI target.
// A transaction entry in the associative table is released 
// by the target thread only when the response has been succesfully 
// transmited to the initiator thread. 
// Entries can be allocated and released in different orders
// and this explain the "pseudo-FIFO" name. 
// Thre is actually three possible for an entry:
// - EMPTY      : no transaction is registered
// - OPEN       : a command is registered and has not been sent 
// - COMPLETED  : command has been send and response is expected
//
// 4) The target thread keeps running as long as the pseudo-FIFO
// is not empty, and goes to wait state when the last response 
// has been returned.
////////////////////////////////////////////////////////////////

#include "target_vci_transactor.h"

template<typename T>
T mask2be(T mask)
{
  T be = 0;
  T m = 0xFF;
  for(size_t i=1; i<sizeof(T); i++)
    m <<= 8;

  for (size_t i=0; i<sizeof(T); i++) {
    be <<= 1;
    if((mask & m) == m )
      be|=1;
    mask <<= 8;
  }
  return be;
}

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

#define tmpl(x) template<typename vci_param_caba,typename vci_param_tlmdt> x TargetVciTransactor<vci_param_caba,vci_param_tlmdt>

//////////////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR
//////////////////////////////////////////////////////////////////////////////////////////
tmpl(/**/)::TargetVciTransactor( sc_core::sc_module_name name )
	   : sc_module(name)
	  , m_buffer()
	  , m_nirq(0)
	  , p_clk("p_clk")
	  , p_resetn("p_resetn")
	  , p_vci_initiator("vci_initiator")
	  , p_vci_target("vci_target")
{
  //PDES local time
  m_pdes_local_time = new pdes_local_time(sc_core::SC_ZERO_TIME);
  m_working     = false;
  m_clock_count = 0;
  m_cmd_count   = 0;
  m_rsp_count   = 0;
  m_active_irq  = false;

  // bind target vci socket
  p_vci_target(*this);                     

  // register thread process
  SC_THREAD(behavior);    
}

tmpl(/**/)::TargetVciTransactor( sc_core::sc_module_name name, size_t nirq)
	   : sc_module(name)
	  , m_buffer()
	  , m_nirq(nirq)
	  , p_clk("p_clk")
	  , p_resetn("p_resetn")
	  , p_vci_initiator("vci_initiator")
	  , p_vci_target("vci_target")
{
  //PDES local time
  m_pdes_local_time = new pdes_local_time(sc_core::SC_ZERO_TIME);
  m_working     = false;
  m_cmd_working = false;
  m_clock_count = 0;
  m_cmd_count   = 0;
  m_rsp_count   = 0;

  // bind target vci socket
  p_vci_target(*this);                     

  //IRQ
  p_irq_target  = new sc_core::sc_in<bool>[m_nirq];
  m_irq         = new bool[m_nirq];
  m_active_irq  = false;
  
  //create irq sockets
  for(size_t i=0; i<m_nirq; i++){
    m_irq[i] = 0;

    std::ostringstream irq_name;
    irq_name << "irq" << i;
    p_irq_initiator.push_back(new tlm_utils::simple_initiator_socket_tagged<TargetVciTransactor,32,tlm::tlm_base_protocol_types>(irq_name.str().c_str()));
  }

  //create payload and extension of a irq transaction
  m_irq_payload_ptr = new tlm::tlm_generic_payload();
  m_irq_extension_ptr = new soclib_payload_extension();

  // register thread process
  SC_THREAD(behavior_irq);    
}

tmpl(/**/)::~TargetVciTransactor(){}
    
/////////////////////////////////////////////////////////////////////////////////////
// BEHAVIOR THREAD
/////////////////////////////////////////////////////////////////////////////////////
tmpl(void)::behavior(void)
{
  p_vci_initiator.cmdval  = false;
  p_vci_initiator.cmd     = vci_param_caba::CMD_NOP;
  p_vci_initiator.address = 0;
  p_vci_initiator.wdata   = 0;
  p_vci_initiator.be      = 0;
  p_vci_initiator.plen    = 0;
  p_vci_initiator.trdid   = 0;
  p_vci_initiator.pktid   = 0;
  p_vci_initiator.srcid   = 0;
  p_vci_initiator.cons    = false;
  p_vci_initiator.wrap    = false;
  p_vci_initiator.contig  = true;
  p_vci_initiator.clen    = 0;
  p_vci_initiator.cfixed  = false;
  p_vci_initiator.eop     = true;

  p_vci_initiator.rspack  = false;

  p_clk.write(true);
  p_resetn.write(false);
 
  sc_core::wait(sc_core::SC_ZERO_TIME);

  p_clk.write(false);
  sc_core::wait(sc_core::SC_ZERO_TIME);

  p_resetn.write(true);

  while (true){
    if(!m_cmd_working){
   
      if(m_buffer.get_cmd_payload(m_cmd_payload, m_cmd_phase, m_cmd_time)){
	m_cmd_payload->get_extension(m_cmd_extension);

#if SOCLIB_MODULE_DEBUG
	printf("[%s] RECEIVE COMMAND src_id = %d nwords = %d time = %d\n", name(), m_cmd_extension->get_src_id(), m_cmd_nwords, (int)m_pdes_local_time->get().value());
#endif

	if ( m_pdes_local_time->get() < *m_cmd_time){
	  m_pdes_local_time->set(*m_cmd_time);
	}
	
	m_cmd_nwords = m_cmd_payload->get_data_length() / vci_param_tlmdt::nbytes;
	m_cmd_count = 0;
	m_rsp_count = 0;
	m_working = true;
	m_cmd_working = true;
      }
      else if(!m_working){
#if SOCLIB_MODULE_DEBUG
	printf("[%s] WAITING PUSH\n", name());
#endif
	sc_core::wait(m_push_event);
      }
    }//end if !m_cmd_working

    if(m_working){
      m_clock_count++;
      m_pdes_local_time->add(UNIT_TIME);

      p_clk.write(true);
      sc_core::wait(sc_core::SC_ZERO_TIME);
      rsp();
      p_clk.write(false);
      sc_core::wait(sc_core::SC_ZERO_TIME);
      cmd();
    }
  }
}

tmpl(void)::behavior_irq(void)
{
  p_vci_initiator.cmdval  = false;
  p_vci_initiator.cmd     = vci_param_caba::CMD_NOP;
  p_vci_initiator.address = 0;
  p_vci_initiator.wdata   = 0;
  p_vci_initiator.be      = 0;
  p_vci_initiator.plen    = 0;
  p_vci_initiator.trdid   = 0;
  p_vci_initiator.pktid   = 0;
  p_vci_initiator.srcid   = 0;
  p_vci_initiator.cons    = false;
  p_vci_initiator.wrap    = false;
  p_vci_initiator.contig  = true;
  p_vci_initiator.clen    = 0;
  p_vci_initiator.cfixed  = false;
  p_vci_initiator.eop     = true;

  p_vci_initiator.rspack  = false;

  p_clk.write(true);
  p_resetn.write(false);
 
  sc_core::wait(sc_core::SC_ZERO_TIME);

  p_clk.write(false);
  sc_core::wait(sc_core::SC_ZERO_TIME);

  p_resetn.write(true);

  while (true){
    if(!m_cmd_working){
      if(m_buffer.get_cmd_payload(m_cmd_payload, m_cmd_phase, m_cmd_time)){
	m_cmd_payload->get_extension(m_cmd_extension);

#if SOCLIB_MODULE_DEBUG
	printf("[%s] RECEIVE COMMAND src_id = %d nwords = %d time = %d\n", name(), m_cmd_extension->get_src_id(), m_cmd_nwords, (int)m_pdes_local_time->get().value());
#endif

	if ( m_pdes_local_time->get() < *m_cmd_time){
	  m_pdes_local_time->set(*m_cmd_time);
	}
	
	m_cmd_nwords = m_cmd_payload->get_data_length() / vci_param_tlmdt::nbytes;
	m_cmd_count = 0;
	m_rsp_count = 0;
	m_working = true;
	m_cmd_working = true;
      }
      else if(!m_working){
#if SOCLIB_MODULE_DEBUG
	printf("[%s] WAITING PUSH\n", name());
#endif
	sc_core::wait(m_push_event);
	
	p_clk.write(true);
	sc_core::wait(sc_core::SC_ZERO_TIME);
	irq();
	p_clk.write(false);
	sc_core::wait(sc_core::SC_ZERO_TIME);
      }
    }//end if !m_cmd_working
    
    if(m_working){
      m_clock_count++;
      m_pdes_local_time->add(UNIT_TIME);

      p_clk.write(true);
      sc_core::wait(sc_core::SC_ZERO_TIME);
      rsp();
      irq();
      p_clk.write(false);
      sc_core::wait(sc_core::SC_ZERO_TIME);
      cmd();
    }
  }
}

tmpl(void)::cmd()
{
  if(m_working){
    if(m_cmd_count==0 || (p_vci_initiator.cmdack && m_cmd_count<m_cmd_nwords)){
      if(m_cmd_count==0){
	m_initial_time =  m_clock_count + 1;
      }
      p_vci_initiator.cmdval  = true;
      p_vci_initiator.address = m_cmd_payload->get_address() + (m_cmd_count*vci_param_tlmdt::nbytes);
      p_vci_initiator.wdata   = atou(m_cmd_payload->get_data_ptr(),(m_cmd_count*vci_param_tlmdt::nbytes));
      
      m_cmd_be = atou(m_cmd_payload->get_byte_enable_ptr(),(m_cmd_count*vci_param_tlmdt::nbytes));
      typename vci_param_caba::be_t be = mask2be<typename vci_param_tlmdt::data_t>(m_cmd_be);
      p_vci_initiator.be = be;
      
      p_vci_initiator.plen = 0;
      
      switch(m_cmd_extension->get_command()){
      case VCI_READ_COMMAND:
	p_vci_initiator.cmd   = vci_param_caba::CMD_READ;
	break;
      case VCI_WRITE_COMMAND:
	p_vci_initiator.cmd   = vci_param_caba::CMD_WRITE;
	break;
      case VCI_STORE_COND_COMMAND:
	p_vci_initiator.cmd   = vci_param_caba::CMD_STORE_COND;
	break;
      case VCI_LINKED_READ_COMMAND:
	p_vci_initiator.cmd   = vci_param_caba::CMD_LOCKED_READ;
	break;
      default:
	p_vci_initiator.cmd   = vci_param_caba::CMD_NOP;
	break;
      }       
      
      p_vci_initiator.trdid   = m_cmd_extension->get_trd_id();
      p_vci_initiator.pktid   = m_cmd_extension->get_pkt_id();
      p_vci_initiator.srcid   = m_cmd_extension->get_src_id();
      p_vci_initiator.cons    = false;
      p_vci_initiator.wrap    = false;
      p_vci_initiator.contig  = true;
      p_vci_initiator.clen    = 0;
      p_vci_initiator.cfixed  = false;
      
#if SOCLIB_MODULE_DEBUG
      printf("[%s] SEND COMMAND address = %x src_id = %d pkt_id = %d trdid = %d\n", name(), (int)(m_cmd_payload->get_address() + (m_cmd_count*vci_param_tlmdt::nbytes)), m_cmd_extension->get_src_id(), m_cmd_extension->get_pkt_id(), m_cmd_extension->get_trd_id());
#endif
      
      if(m_cmd_count==m_cmd_nwords-1){
	p_vci_initiator.eop   = true;
	m_cmd_working = false;
      }
      else{
	p_vci_initiator.eop   = false;
      }
      m_cmd_count++;
    }
  }
  else{
    p_vci_initiator.cmdval = false;
  }
}

tmpl(void)::rsp()
{
  p_vci_initiator.rspack = true;
  if(p_vci_initiator.rspval){
    m_rscrid = p_vci_initiator.rsrcid.read();
    m_rpktid = p_vci_initiator.rpktid.read();
    m_rtrdid = p_vci_initiator.rtrdid.read();
    
#if SOCLIB_MODULE_DEBUG
    printf("[%s] RECEIVE RESPONSE src_id = %d\n", name(), m_rscrid);
#endif
    
    int idx = m_buffer.get_rsp_payload( m_rscrid, m_rsp_payload, m_rsp_phase, m_rsp_time); 
    assert(idx!=-1 && "response do not compatible with any command");
    
    if(m_rsp_count == 0){
      m_final_time =  m_clock_count;
    }
    
    if(p_vci_initiator.rerror.read())
      m_rsp_payload->set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    else
      m_rsp_payload->set_response_status(tlm::TLM_OK_RESPONSE);
    
    utoa((int)p_vci_initiator.rdata.read(), m_rsp_payload->get_data_ptr(), m_rsp_count*vci_param_tlmdt::nbytes);
    
    if(p_vci_initiator.reop.read()){
      //modify the phase
      *m_rsp_phase = tlm::BEGIN_RESP;
      
      //update the message time
      if ( *m_rsp_time < m_pdes_local_time->get() ){
	*m_rsp_time = m_pdes_local_time->get();
      }
      
      //increment the target processing time 
      *m_rsp_time = *m_rsp_time + ((m_final_time - m_initial_time + m_rsp_count) * UNIT_TIME);
      m_pdes_local_time->set(*m_rsp_time + UNIT_TIME);
      
      //m_working = m_buffer.waiting_response();
      m_working = false;
      
#if SOCLIB_MODULE_DEBUG
      printf("[%s] SEND RESPONSE src_id = %d pkt_id = %d trdid = %d time = %d\n", name(), m_rscrid, m_rpktid, m_rtrdid, (int)(*m_rsp_time).value());
#endif
      
      //send the response
      p_vci_target->nb_transport_bw(*m_rsp_payload, *m_rsp_phase, *m_rsp_time);
      //pop transaction
      m_buffer.pop( idx );
      m_pop_event.notify(sc_core::SC_ZERO_TIME);
    }
    else{
      m_rsp_count++;
    }
  }
}

tmpl(void)::irq()
{
  bool i;
  for (size_t n=0; n<m_nirq; n++){
    i = p_irq_target[n].read();
    if(i != m_irq[n]){
#ifdef SOCLIB_MODULE_DEBUG
      std::cout << "[" << name() << "] Receive Interrupt " << n << " val = " << i << std::endl;
#endif
      m_irq[n] = i;
      send_interrupt(n, i);
    }
  }
}

tmpl(void)::send_interrupt(int id, bool irq){
  size_t nbytes = vci_param_tlmdt::nbytes; //1 word
  unsigned char data_ptr[nbytes];
  unsigned char byte_enable_ptr[nbytes];
  unsigned int bytes_enabled = be2mask<unsigned int>(0xF);

  m_irq[id] = irq;

  // set the all bytes to enabled
  utoa(bytes_enabled, byte_enable_ptr, 0);
  // set the val to data
  utoa(m_irq[id], data_ptr, 0);

  // set the values in tlm payload
  m_irq_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
  m_irq_payload_ptr->set_byte_enable_ptr(byte_enable_ptr);
  m_irq_payload_ptr->set_byte_enable_length(nbytes);
  m_irq_payload_ptr->set_data_ptr(data_ptr);
  m_irq_payload_ptr->set_data_length(nbytes);
  // set the values in payload extension
  m_irq_extension_ptr->set_write();
  // set the extension to tlm payload
  m_irq_payload_ptr->set_extension(m_irq_extension_ptr);
  
  // set the tlm phase
  m_irq_phase = tlm::BEGIN_REQ;
  // set the local time to transaction time
  m_irq_time = m_pdes_local_time->get();
  //m_irq_time = sc_core::SC_ZERO_TIME;
  
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Send Interrupt " << id << " val = " << m_irq[id] <<  " time = " << m_irq_time.value() << std::endl;
#endif
  
  // send the transaction
  (*p_irq_initiator[id])->nb_transport_fw(*m_irq_payload_ptr, m_irq_phase, m_irq_time);
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if VCI SOCKET
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw
( tlm::tlm_generic_payload &payload,
  tlm::tlm_phase           &phase,  
  sc_core::sc_time         &time)   
{
  soclib_payload_extension *extension;
  payload.get_extension(extension);
  if(extension->is_null_message()){
    if ( m_pdes_local_time->get() < time)
      m_pdes_local_time->set(time);

#if SOCLIB_MODULE_DEBUG
    printf("[%s] RECEIVE NULL MESSAGE\n", name());
#endif

    m_push_event.notify(sc_core::SC_ZERO_TIME);
    return tlm::TLM_COMPLETED;
  }

  bool push = false;
  int try_push = 0;
  do{

    push = m_buffer.push(payload, phase, time);

    if(!push){
      try_push++;
#if SOCLIB_MODULE_DEBUG
      printf("[%s] <<<<<<<<< NOT PUSH >>>>>>>> try_push = %d \n", name(), try_push);
#endif
      sc_core::wait(m_pop_event);
    }
  }while(!push);
#if SOCLIB_MODULE_DEBUG
  printf("[%s] push\n", name());
#endif
  m_push_event.notify(sc_core::SC_ZERO_TIME);
  return tlm::TLM_COMPLETED;
}

// Not implemented for this example but required by interface
tmpl(void)::b_transport
( tlm::tlm_generic_payload &payload,                // payload
  sc_core::sc_time         &time)                   //time
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

}}
