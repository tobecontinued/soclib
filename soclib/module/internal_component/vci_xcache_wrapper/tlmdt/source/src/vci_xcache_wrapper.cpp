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

//#define SOCLIB_MODULE_DEBUG

#include "alloc_elems.h"
#include "../include/vci_xcache_wrapper.h"

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

namespace soclib{ namespace tlmdt {

#define tmpl(x) template<typename vci_param, typename iss_t> x VciXcacheWrapper<vci_param, iss_t>
   
/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::my_nb_transport_bw     // inbound nb_transport_bw
( tlm::tlm_generic_payload           &payload,       // payload
  tlm::tlm_phase                     &phase,         // phase
  sc_core::sc_time                   &time)          // time
{

  soclib_payload_extension *extension_ptr;
  payload.get_extension(extension_ptr);

  m_error = payload.is_response_error();
    
  update_time(time);
    
  m_rsp_received.notify (sc_core::SC_ZERO_TIME);
  return tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (IRQ SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
/*
tmpl (tlm::tlm_sync_enum)::my_nb_transport_fw         // inbound nb_transport_bw
( int                                     id,         // interruption id
  soclib_irq_types::tlm_payload_type      &payload,   // payload
  soclib_irq_types::tlm_phase_type        &phase,     // phase
  sc_core::sc_time                        &time)      // time
{
}

tmpl (void)::irqReceived (
	bool v, const tlmt_core::
	tlmt_time & time, void *private_data)
{
    int no = (int)(long)private_data;
	std::cout
		<< name()
		<< " at " << c0.time()
		<< " Received irq " << no
		<< " dated " << time
		<< " val: " << v
		<< std::endl;

	m_pending_irqs[time] = std::pair<int, bool>(no, v);
}
*/

tmpl (void)::update_time(sc_core::sc_time t)
{
  if(t > m_pdes_local_time->get()){
    m_pdes_local_time->set(t);
  }
}

tmpl (/**/)::VciXcacheWrapper
(
 sc_core::sc_module_name name,
 int cpuid,
 const soclib::common::IntTab &index,
 const soclib::common::MappingTable &mt,
 size_t icache_lines,
 size_t icache_words,
 size_t dcache_lines,
 size_t dcache_words,
 sc_core::sc_time time_quantum,
 sc_core::sc_time simulation_time)
  : sc_module(name),
    m_id(mt.indexForId(index)),
    m_iss(this->name(), cpuid),
    m_irq(0),
    m_simulation_time(simulation_time),
    m_dcache(dcache_lines, dcache_words),
    m_icache(icache_lines, icache_words),
    m_cacheability_table(mt.getCacheabilityTable()),
    p_vci_initiator("socket")             // vci initiator socket name
{
  //register callback function VCI INITIATOR SOCKET
  p_vci_initiator.register_nb_transport_bw(this, &VciXcacheWrapper::my_nb_transport_bw);

  /*
    p_irq = (tlmt_core::tlmt_in<bool>*)malloc(sizeof(tlmt_core::tlmt_in<bool>)*iss_t::n_irq);
    for (int32_t i = 0 ; i < iss_t::n_irq ; i++) {
    std::ostringstream o;
    o << "irq[" << i << "]";
    new(&p_irq[i])tlmt_core::tlmt_in<bool> (
    o.str(),
    new tlmt_core::tlmt_callback<VciXcacheWrapper, bool>(
    this,&VciXcacheWrapper<iss_t, vci_param>::irqReceived, (void*)(long)i));
    }
  */
  
  m_error       = false;
  
  m_iss.setDCacheInfo(dcache_words*4,1,dcache_lines);
  m_iss.setICacheInfo(icache_words*4,1,icache_lines);
  m_iss.reset();
  
  //PDES local time
  m_pdes_local_time = new pdes_local_time(time_quantum);

  //PDES activity status
  m_pdes_activity_status = new pdes_activity_status();

  //create payload and extension to a normal message
  m_payload_ptr = new tlm::tlm_generic_payload();
  m_extension_ptr = new soclib_payload_extension();

  //create payload and extension to a null message
  m_null_payload_ptr = new tlm::tlm_generic_payload();
  m_null_extension_ptr = new soclib_payload_extension();

  //create payload and extension to an activity message
  m_activity_payload_ptr = new tlm::tlm_generic_payload();
  m_activity_extension_ptr = new soclib_payload_extension();

  SC_THREAD(execLoop);
}

tmpl (void)::execLoop ()
{
  while(m_pdes_local_time->get() < m_simulation_time){
    //while(1) {
    struct iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    struct iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    
    struct iss_t::InstructionResponse meanwhile_irsp = ISS_IRSP_INITIALIZER;
    struct iss_t::DataResponse meanwhile_drsp = ISS_DRSP_INITIALIZER;
    
    struct iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;
    struct iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;
    
    m_iss.getRequests(ireq, dreq);

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << std::endl;
    std::cout << name() << " before cache access: " << ireq << ' ' << dreq << std::endl;
#endif

    // While frozen, the only functionnal part of the cache is the
    // one which does not accesses external resources.
    // Preventively fetch responses from the cache without side
    // effects before actually trying to know if we can answer without delay.
    xcacheAccessInternal(ireq, dreq, meanwhile_irsp, meanwhile_drsp);
    
    // This call is _with_ side effects and gives delay information.
    uint32_t del = xcacheAccess(ireq, dreq, irsp, drsp);
    
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " after cache access: " << irsp << ' ' << drsp << std::endl;
#endif
    
    /*
    while ( ! m_pending_irqs.empty() &&
	    m_pending_irqs.begin()->first <= c0.time() ) {
      std::map<tlmt_core::tlmt_time, 
	std::pair<int, bool> >::iterator i = m_pending_irqs.begin();
      if ( i->second.second )
	m_irq |= 1<<i->second.first;
      else
	m_irq &= ~(1<<i->second.first);
      m_pending_irqs.erase(i);
    }
    */
    uint32_t nc = 0;
    // Now if we had a delay, give the information to the CPU,
    // with the cache state before fetching the answer.
    if ( del )
      nc += m_iss.executeNCycles(del, meanwhile_irsp, meanwhile_drsp, m_irq);
    nc += m_iss.executeNCycles(1, irsp, drsp, m_irq);
    m_pdes_local_time->add(nc * UNIT_TIME);
    
    // if initiator needs synchronize then it sends a null message
    if (m_pdes_local_time->need_sync()) {
      send_null_message();
    }

  }
  //sc_core::sc_stop();
  m_pdes_activity_status->set(false);
  send_activity();
}

tmpl (uint32_t)::xcacheAccess
(
 struct iss_t::InstructionRequest ireq,
 struct iss_t::DataRequest dreq,
 struct iss_t::InstructionResponse &irsp,
 struct iss_t::DataResponse &drsp )
{
  if ( ireq.valid ) {
    if ( m_icache.miss( ireq.addr ) ) {
      bool err = false;
      uint32_t del = fill_cache( m_icache, ireq.addr, err );
      if ( err ) {
	irsp.valid = true;
	irsp.error = true;
	return del;
      } else {
	irsp.valid = false;
	return del;
      }
    } else {
      irsp.valid = true;
      irsp.instruction = m_icache.read(ireq.addr);
    }
  } else {
    irsp.valid = false;
  }
  
  if ( dreq.valid ) {
    switch ( dreq.type ) {
    case iss_t::DATA_READ:
      if ( m_cacheability_table[dreq.addr] ) {
	if ( m_dcache.miss( dreq.addr ) ) {
	  bool err = false;
	  uint32_t del = fill_cache( m_dcache, dreq.addr, err );
	  if ( err ) {
	    drsp.valid = true;
	    drsp.error = true;
	    return del;
	  } else {
	    drsp.valid = false;
	    return del;
	  }
	} else {
	  drsp.valid = true;
	  drsp.rdata = m_dcache.read(dreq.addr);
	  drsp.error = false;
	}
      } else {
	drsp.valid = true;
	return ram_read(VCI_READ_COMMAND, dreq.addr,	&drsp.rdata, drsp.error );
      }
      break;
    case iss_t::DATA_WRITE: {
      drsp.valid = true;
      uint32_t del = ram_write( VCI_WRITE_COMMAND, dreq.addr, dreq.wdata, dreq.be, drsp.rdata, drsp.error );
      if ( ! m_dcache.miss(dreq.addr) ) {
	typename vci_param::data_t old = m_dcache.read(dreq.addr);
	typename vci_param::data_t mask = be2mask<typename vci_param::data_t>(dreq.be);
	m_dcache.write(dreq.addr, (dreq.wdata & mask) | (old & ~mask) );

#ifdef SOCLIB_MODULE_DEBUG
	std::cout << name() << " wdata update: " << dreq << " new data:" << std::hex << m_dcache.read(dreq.addr) << std::endl;
#endif
      }
      return del;
      break;
    }
    case iss_t::DATA_LL:
      drsp.valid = true;
      return ram_read( VCI_LINKED_READ_COMMAND, dreq.addr, &drsp.rdata, drsp.error );
    case iss_t::DATA_SC:
      drsp.valid = true;
      return ram_write(VCI_STORE_COND_COMMAND, dreq.addr, dreq.wdata, 0xf, drsp.rdata, drsp.error );
    case iss_t::XTN_READ:
    case iss_t::XTN_WRITE:
      switch (dreq.addr/4) {
      case iss_t::XTN_DCACHE_INVAL:
	if ( !m_dcache.miss( dreq.wdata ) )
	  m_dcache.inval( dreq.wdata );
      }
      drsp.valid = true;
      drsp.error = false;
    }
  } else {
    drsp.valid = false;
  }
  return 0;
}
   
tmpl (void)::xcacheAccessInternal
(
 struct iss_t::InstructionRequest ireq,
 struct iss_t::DataRequest dreq,
 struct iss_t::InstructionResponse &irsp,
 struct iss_t::DataResponse &drsp
 ) const
{
  if ( ireq.valid && ! m_icache.miss(ireq.addr) ) {
    irsp.valid = true;
    irsp.instruction = m_icache.read(ireq.addr);
  }
  
  if ( dreq.valid
       && dreq.type == iss_t::DATA_READ
       && m_cacheability_table[dreq.addr]
       && ! m_dcache.miss( dreq.addr ) ) {
    drsp.valid = true;
    drsp.rdata = m_dcache.read(dreq.addr);
    drsp.error = false;
  }
}

tmpl (uint32_t)::ram_write
(
 enum command command, 
 typename vci_param::addr_t address,
 typename vci_param::data_t wdata, 
 int be, 
 typename vci_param::data_t &rdata, 
 bool &rerror
 )
{
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << name() << " ram_write( " << command << ", " << std::hex << address << ", " << wdata << ", " << rdata << ", " << rerror << ")" << std::endl;
#endif

  typename vci_param::data_t byte_enable;
  switch(be){
  case 0x1:
    byte_enable = 0x000000FF;
    break;
  case 0x2:
    byte_enable = 0x0000FF00;
    break;
  case 0x3:
    byte_enable = 0x0000FFFF;
    break;
  case 0x4:
    byte_enable = 0x00FF0000;
    break;
  case 0x8:
    byte_enable = 0xFF000000;
    break;
  case 0xC:
    byte_enable = 0xFFFF0000;
    break;
  default:
    byte_enable = 0xFFFFFFFF;
    break;
  }

  m_nbytes = vci_param::nbytes;
  utoa(byte_enable, m_byte_enable_ptr, 0);
  utoa(wdata, m_data_ptr, 0);

  // set the values in tlm payload
  m_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
  m_payload_ptr->set_address(address & ~3);
  m_payload_ptr->set_byte_enable_ptr(m_byte_enable_ptr);
  m_payload_ptr->set_byte_enable_length(m_nbytes);
  m_payload_ptr->set_data_ptr(m_data_ptr);
  m_payload_ptr->set_data_length(m_nbytes);
  // set the values in payload extension
  m_extension_ptr->set_command(command);
  m_extension_ptr->set_src_id(m_id);
  m_extension_ptr->set_trd_id(0);
  m_extension_ptr->set_pkt_id(0);
  // set the extension to tlm payload
  m_payload_ptr->set_extension(m_extension_ptr);
  //set the tlm phase
  m_phase = tlm::BEGIN_REQ;
  //set the local time to transaction time
  m_time = m_pdes_local_time->get();
  sc_core::sc_time before = m_time;

  //send a write message
  p_vci_initiator->nb_transport_fw(*m_payload_ptr, m_phase, m_time);
  wait(m_rsp_received);

  //update response
  rdata = atou(m_payload_ptr->get_data_ptr(), 0);
  rerror = m_error;
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " time: " << std::dec << m_pdes_local_time->get().value() << " ram_write, error: " << rerror << " address: " << std::hex << m_payload_ptr->get_address() << " data: " << rdata << std::endl;
#endif
  return (m_pdes_local_time->get() - before).value();
}

tmpl (uint32_t)::ram_read
(
 enum command command, 
 typename vci_param::addr_t address,
 typename vci_param::data_t *rdata, 
 bool &rerror,
 size_t size
 )
{
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << name() << " ram_read( " << command << ", " << std::hex << address << ", " << rdata << ", " << rerror << ")" << std::endl;
#endif
  
  m_nbytes = size * vci_param::nbytes;
  for(uint32_t i=0; i<m_nbytes; i++){
    m_byte_enable_ptr[i] = 0xF;
  }

  // set the values in tlm payload
  m_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
  m_payload_ptr->set_address(address & ~3);
  m_payload_ptr->set_byte_enable_ptr(m_byte_enable_ptr);
  m_payload_ptr->set_byte_enable_length(m_nbytes);
  m_payload_ptr->set_data_ptr(m_data_ptr);
  m_payload_ptr->set_data_length(m_nbytes);
  // set the values in payload extension
  m_extension_ptr->set_command(command);
  m_extension_ptr->set_src_id(m_id);
  m_extension_ptr->set_trd_id(0);
  m_extension_ptr->set_pkt_id(0);
  // set the extension to tlm payload
  m_payload_ptr->set_extension(m_extension_ptr);
  //set the tlm phase
  m_phase = tlm::BEGIN_REQ;
  //set the local m_time to transaction time
  m_time = m_pdes_local_time->get();
  sc_core::sc_time before = m_time;

  //send a write message
  p_vci_initiator->nb_transport_fw(*m_payload_ptr, m_phase, m_time);
  wait(m_rsp_received);

  //update response
  for(uint32_t i=0; i<(m_payload_ptr->get_data_length()/vci_param::nbytes); i++)
     rdata[i] = atou(m_payload_ptr->get_data_ptr(), (i * vci_param::nbytes));
  rerror = m_error;

#ifdef SOCLIB_MODULE_DEBUG
  std::cout << name() << " time: " << m_pdes_local_time->get().value() << " ram_read, error: " << rerror << " address: " << std::hex << m_payload_ptr->get_address() << " data: " << rdata << std::endl;
#endif
  return (m_pdes_local_time->get() - before).value();
}

tmpl (uint32_t)::fill_cache
(
 soclib::tlmt::genericCache<vci_param> &cache,
 typename vci_param::addr_t address,
 bool &error
 )
{
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << name() << " fill_cache( " << &cache << ", " << std::hex << address << ", " << error << ")" << std::endl;
#endif
  typename vci_param::data_t data[cache.get_nwords()];
  
  uint32_t del = ram_read( VCI_READ_COMMAND, address & cache.get_yzmask(),
			   &data[0], error, cache.get_nwords());
  
  if ( ! error ) {
    cache.update( address, &data[0] );
  }
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << name() << " fill_cache, error: " << error << ", data: ";
  for ( size_t i=0; i<cache.get_nwords(); ++i )
    std::cout << std::hex << data[i] << ' ';
  std::cout << std::endl;
#endif
  return del;
}

tmpl (void)::send_activity()
{
  // set the active or inactive command
  if(m_pdes_activity_status->get())
    m_activity_extension_ptr->set_active();
  else
    m_activity_extension_ptr->set_inactive();
  m_activity_extension_ptr->set_src_id(m_id);
  // set the extension to tlm payload
  m_activity_payload_ptr->set_extension(m_activity_extension_ptr);
  //set the tlm phase
  m_activity_phase = tlm::BEGIN_REQ;
  //set the local time to transaction time
  m_activity_time = m_pdes_local_time->get();
   
  std::cout << std::endl << name() << " SEND ACTIVITY " << m_activity_extension_ptr->is_active() << std::endl;

  //send a message with command equals to PDES_ACTIVE or PDES_INACTIVE
  p_vci_initiator->nb_transport_fw(*m_activity_payload_ptr, m_activity_phase, m_activity_time);
  //deschedule the initiator thread
  wait(sc_core::SC_ZERO_TIME);
}

tmpl (void)::send_null_message()
{
  // set the null message command
  m_null_extension_ptr->set_null_message();
  m_null_extension_ptr->set_src_id(m_id);
  // set the extension to tlm payload
  m_null_payload_ptr->set_extension(m_null_extension_ptr);
  //set the tlm phase
  m_null_phase = tlm::BEGIN_REQ;
  //set the local time to transaction time
  m_null_time = m_pdes_local_time->get();
   
#if MY_INITIATOR_DEBUG
  std::cout << "[INITIATOR " << m_srcid << "] send NULL MESSAGE time = " << m_null_time.value() << std::endl;
#endif

  //send a null message
  p_vci_initiator->nb_transport_fw(*m_null_payload_ptr, m_null_phase, m_null_time);
  //deschedule the initiator thread
  wait(sc_core::SC_ZERO_TIME);
}
}}

