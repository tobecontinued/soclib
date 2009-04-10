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
 * Maintainers: fpecheux, alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */


#include "vci_xcache.h"

//#define SOCLIB_MODULE_DEBUG

#ifdef SOCLIB_MODULE_DEBUG
namespace {
  const char *dcache_fsm_state_str[] = {
    "DCACHE_INIT",
    "DCACHE_IDLE",
    "DCACHE_WRITE_UPDT",
    "DCACHE_WRITE_REQ",
    "DCACHE_MISS_REQ",
    "DCACHE_MISS_WAIT",
    "DCACHE_MISS_UPDT",
    "DCACHE_UNC_REQ",
    "DCACHE_UNC_WAIT",
    "DCACHE_INVAL",
    "DCACHE_ERROR"
  };
  
  const char *icache_fsm_state_str[] = {
    "ICACHE_INIT",
    "ICACHE_IDLE",
    "ICACHE_WAIT",
    "ICACHE_UPDT",
    "ICACHE_ERROR"
  };
  
  const char *cmd_fsm_state_str[] = {
    "CMD_IDLE",
    "CMD_DATA_MISS",
    "CMD_DATA_UNC_READ",
    "CMD_DATA_UNC_STORE_COND",
    "CMD_DATA_UNC_READ_LINKED",
    "CMD_DATA_WRITE",
    "CMD_INS_MISS"
  };
  
  const char *rsp_fsm_state_str[] = {
    "RSP_IDLE",
    "RSP_INS_MISS",
    "RSP_INS_ERROR_WAIT",
    "RSP_INS_ERROR",
    "RSP_INS_OK",
    "RSP_DATA_MISS",
    "RSP_DATA_UNC",
    "RSP_DATA_WRITE_UNC",
    "RSP_DATA_WRITE",
    "RSP_WAIT_TIME",
    "RSP_UNC_WAIT_TIME",
    "RSP_DATA_READ_ERROR_WAIT",
    "RSP_DATA_READ_ERROR",
    "RSP_DATA_MISS_OK",
    "RSP_DATA_UNC_OK",
    "RSP_DATA_WRITE_ERROR",
    "RSP_DATA_WRITE_ERROR_WAIT"
  };
}
#endif

namespace soclib{ namespace tlmdt {
    
#define tmpl(x) template<typename vci_param, typename iss_t> x VciXcache<vci_param, iss_t>

///Constructor
tmpl (/**/)::VciXcache
( sc_core::sc_module_name name,           // module name
  int id,                                 // iss id
  const soclib::common::IntTab &index,    // index of mapping table
  const soclib::common::MappingTable &mt, // mapping table
  size_t icache_lines,
  size_t icache_words,
  size_t dcache_lines,
  size_t dcache_words,
  sc_core::sc_time time_quantum,          // time quantum
  sc_core::sc_time simulation_time)       // simulation time
  : sc_module(name),                      // init module name
    m_iss(id),
    m_buf(100), 
    m_dcache(dcache_lines, dcache_words),
    m_icache(icache_lines, icache_words),
    p_vci_initiator("socket")             // vci initiator socket name
{
  //register callback function VCI INITIATOR SOCKET
  p_vci_initiator.register_nb_transport_bw(this, &VciXcache::my_nb_transport_bw);
  /*
  //register callback function IRQ TARGET SOCKET
  //for(int i=0; i<iss_t::n_irq; i++){
  for(int i=0; i<1; i++){
    std::ostringstream irq_name;
    irq_name << "irq" << i;
    p_irq_target.push_back(new tlm_utils::simple_target_socket_tagged<VciXcache,32,tlm::tlm_base_protocol_types>(irq_name.str().c_str()));
    
    p_irq_target[i]->register_nb_transport_fw(this, &VciXcache::my_nb_transport_fw, i);
  }
  */
  m_cacheability_table = mt.getCacheabilityTable();

  // initiator identification
  m_srcid                 = mt.indexForId(index);

  //PDES local time
  m_pdes_local_time = new pdes_local_time(time_quantum);

  //PDES activity status
  m_pdes_activity_status = new pdes_activity_status();

  //determine the simulation time
  m_simulation_time = simulation_time;

  //activity counters
  m_cpt_lookhead             = 0;
  m_cpt_idle                 = 0;
  
  m_icache_cpt_init          = icache_lines;
  m_icache_cpt_cache_read    = 0;
  m_icache_cpt_uncache_read  = 0;
  
  m_dcache_cpt_init          = dcache_lines;
  m_dcache_cpt_cache_read    = 0;
  m_dcache_cpt_uncache_read  = 0;
  m_dcache_cpt_cache_write   = 0;
  m_dcache_cpt_uncache_write = 0;
  
  m_cpt_fifo_read            = 0;
  m_cpt_fifo_write           = 0;
  
  m_dcache_fsm       = DCACHE_IDLE;
  m_icache_fsm       = ICACHE_IDLE;
  //m_dcache_fsm       = DCACHE_INIT;
  //m_icache_fsm       = ICACHE_INIT;
  m_vci_cmd_fsm      = CMD_IDLE;
  m_vci_rsp_fsm      = RSP_IDLE;
  
  m_rsp_vci_time     = sc_core::SC_ZERO_TIME;
  m_req_icache_time  = sc_core::SC_ZERO_TIME;
  
  m_vci_write        = false;
  m_icache_req       = false;
  m_dcache_miss_req  = false;
  m_dcache_unc_req   = false;
  m_dcache_unc_valid = false;
  
  m_icache_frz       = false;
  m_dcache_frz       = false;
  
  m_read_error       = false;
  m_write_error      = false;
  
  m_iss.setDCacheInfo(dcache_words*4,1,dcache_lines);
  m_iss.setICacheInfo(icache_words*4,1,icache_lines);
  m_iss.reset ();

  //create extension to normal message
  m_extension_ptr = new soclib_payload_extension();

  //create payload and extension to null message
  m_null_payload_ptr = new tlm::tlm_generic_payload();
  m_null_extension_ptr = new soclib_payload_extension();

  //create payload and extension to activity message
  m_activity_payload_ptr = new tlm::tlm_generic_payload();
  m_activity_extension_ptr = new soclib_payload_extension();

  // register thread process
  SC_THREAD(behavior);

}

/////////////////////////////////////////////////////////////////////////////////////
// Fuctions
/////////////////////////////////////////////////////////////////////////////////////
tmpl (sc_core::sc_time)::getLocalTime()
{
  return m_pdes_local_time->get();
}

tmpl (void)::setLocalTime(sc_core::sc_time t)
{
  m_pdes_local_time->set(t);
}

tmpl (void)::updateTime(sc_core::sc_time t){
  if(t > m_pdes_local_time->get()){
    m_iss.nullStep(t.value() - m_pdes_local_time->get().value());
    //m_iss.nullStep(t - m_pdes_local_time->get());
    m_pdes_local_time->set(t);
  }
}

tmpl (void)::addTime(sc_core::sc_time t)
{
  m_iss.nullStep(t.value());
  //m_iss.nullStep(t);
  m_pdes_local_time->add(t);
}

tmpl (size_t)::getTotalCycles(){
  return m_pdes_local_time->get().value();
}

tmpl (size_t)::getActiveCycles(){
  return (m_pdes_local_time->get().value() - m_cpt_idle);
}

tmpl (size_t)::getIdleCycles(){
  return m_cpt_idle;
}

tmpl (size_t)::getNLookhead(){
  return m_cpt_lookhead;
}

tmpl (size_t)::getNIcache_Cache_Read(){
  return m_icache_cpt_cache_read;
}

tmpl (size_t)::getNIcache_Uncache_Read(){
  return m_icache_cpt_uncache_read;
}

tmpl (size_t)::getNDcache_Cache_Read(){
  return m_dcache_cpt_cache_read;
}

tmpl (size_t)::getNDcache_Uncache_Read(){
  return m_dcache_cpt_uncache_read;
}

tmpl (size_t)::getNDcache_Cache_Write(){
  return m_dcache_cpt_cache_write;
}

tmpl (size_t)::getNDcache_Uncache_Write(){
  return m_dcache_cpt_uncache_write;
}

tmpl (size_t)::getNFifo_Read(){
  return m_cpt_fifo_read;
}

tmpl (size_t)::getNFifo_Write(){
  return m_cpt_fifo_write;
}

tmpl (size_t)::getNTotal_Cache_Read(){
  return (m_icache_cpt_cache_read + m_dcache_cpt_cache_read);
}

tmpl (size_t)::getNTotal_Uncache_Read(){
  return (m_icache_cpt_uncache_read + m_dcache_cpt_uncache_read);
}

tmpl (size_t)::getNTotal_Cache_Write(){
  return m_dcache_cpt_cache_write;
}

tmpl (size_t)::getNTotal_Uncache_Write(){
  return m_dcache_cpt_uncache_write;
}

tmpl (void)::sendActivity()
{
  // set the active or inactive command
  if(m_pdes_activity_status->get())
    m_activity_extension_ptr->set_active();
  else
    m_activity_extension_ptr->set_inactive();
  m_activity_extension_ptr->set_src_id(m_srcid);
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

tmpl (void)::sendNullMessage()
{
  // set the null message command
  m_null_extension_ptr->set_null_message();
  m_null_extension_ptr->set_src_id(m_srcid);
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

tmpl (void)::behavior()
{
  while(m_pdes_local_time->get() < m_simulation_time){
  
    enum iss_t::DataAccessType data_type;
    typename vci_param::addr_t          ins_addr = 0;
    typename vci_param::data_t          ins_rdata;
    bool                       ins_asked = false;
    bool                       ins_ber = false;

    typename vci_param::addr_t          data_addr = 0;
    typename vci_param::data_t          data_wdata;
    typename vci_param::data_t          data_rdata;
    bool                       data_asked = false;
    bool                       data_ber = false;
 
    m_iss.getInstructionRequest(ins_asked, ins_addr);
    
    m_iss.getDataRequest(data_asked, data_type, data_addr, data_wdata);

#ifdef SOCLIB_MODULE_DEBUG
    if(m_srcid != 0){
      std::cout
        << name()
	//<< " time: " << std::dec << (m_pdes_local_time->get().value() + 1) << std::hex 
        << " dcache fsm: " << dcache_fsm_state_str[m_dcache_fsm]
        << " icache fsm: " << icache_fsm_state_str[m_icache_fsm]
        << " cmd fsm: " << cmd_fsm_state_str[m_vci_cmd_fsm]
        << " rsp fsm: " << rsp_fsm_state_str[m_vci_rsp_fsm] << std::endl;
      std::cout
	<< " i.valid: " << std::dec << ins_asked
	<< " i.addr: " << std::hex << ins_addr << std::dec
	<< " d.valid: " << data_asked
	<< " d.addr: " << std::hex << data_addr << std::dec
        << std::endl;
    }
#endif

        
    xcacheAccess(ins_asked, ins_addr,
		 data_asked, data_type, data_addr, data_wdata,
		 data_rdata, data_ber, ins_rdata, ins_ber);
    
            
    if(ins_asked && !m_icache_frz){
      m_iss.setInstruction(ins_ber, ins_rdata);

#ifdef SOCLIB_MODULE_DEBUG
      if(m_srcid != 0){
	std::cout << " i.instruction: " << std::hex << ins_rdata << std::dec << std::endl;
      }
#endif

    }
    
    if(data_asked && !m_dcache_frz){
      m_iss.setDataResponse(data_ber, data_rdata);

#ifdef SOCLIB_MODULE_DEBUG
      if(m_srcid != 0){
	std::cout << " d.rdata: " << std::hex << data_rdata << std::dec << std::endl;
      }
#endif

    }
    else if (data_ber)
      m_iss.setWriteBerr ();
    
    if(m_iss.isBusy() || m_icache_frz || m_dcache_frz){
      m_cpt_idle++;
      addTime(UNIT_TIME);
    }
    else{
 
      //the Interruption is not treated yet
      uint32_t it = 0;
      /*
	for ( size_t i=0; i<(size_t)iss_t::n_irq; i++ )
	if (p_irq[i].read())
	it |= (1<<i);
      */
      m_iss.setIrq(it);
      
#ifdef SOCLIB_MODULE_DEBUG
      if(m_srcid != 0){
      	m_iss.dump();
      }
#endif

      m_iss.step();
      m_pdes_local_time->add(UNIT_TIME);

      // if initiator needs synchronize then it sends a null message
      if (m_pdes_local_time->need_sync()) {
	sendNullMessage();
      }
    }
  }
  m_pdes_activity_status->set(false);
  sendActivity();
  //sc_core::sc_stop();
}

tmpl (void)::xcacheAccess (bool & icache_req,
			   uint32_t & ins_addr,
			   bool & dcache_req,
			   enum iss_t::DataAccessType & data_type,
			   uint32_t & data_addr,
			   uint32_t & data_wdata,
			   uint32_t & data_rdata,
			   bool & data_ber,
			   uint32_t & ins_rdata, bool & ins_ber)
{
  unsigned int WORD_ENABLED = 0xffffffff;
  unsigned int LO_WORD_ENABLED = 0x0000ffff;
  unsigned int HI_WORD_ENABLED = 0xffff0000;
  unsigned int BYTE0_ENABLED = 0x000000ff;
  unsigned int BYTE1_ENABLED = 0x0000ff00;
  unsigned int BYTE2_ENABLED = 0x00ff0000;
  unsigned int BYTE3_ENABLED = 0xff000000;

///////////////////////////////////////////////////////////////////////////////////
// The ICACHE FSM 
///////////////////////////////////////////////////////////////////////////////////

switch((icache_fsm_state_e)m_icache_fsm) {
  
 case ICACHE_INIT:
   m_icache_frz = true;
   m_icache_cpt_init = m_icache_cpt_init - 1;
   if (m_icache_cpt_init == 0)
     m_icache_fsm = ICACHE_IDLE;
   break;
   
 case ICACHE_IDLE:
   if ( icache_req ) {
     if (m_icache.miss (ins_addr)){
       m_icache_cpt_uncache_read++;
       m_icache_frz = true;
       m_icache_req = true;
       m_req_icache_time = m_pdes_local_time->get();
       m_icache_fsm = ICACHE_WAIT;
     }
     else{
       m_icache_cpt_cache_read++;
       m_icache_frz = false;
       ins_ber = false;
       ins_rdata = m_icache.read(ins_addr & ~0x3);
     }
   }
   else
     m_icache_frz = false;
   break;
   
 case ICACHE_WAIT:
   if (m_vci_rsp_fsm == RSP_INS_OK)
     m_icache_fsm = ICACHE_UPDT;
   if (m_vci_rsp_fsm == RSP_INS_ERROR)
     m_icache_fsm = ICACHE_ERROR;
   break;
   
 case ICACHE_ERROR:
   m_icache_frz = false;
   m_icache_fsm = ICACHE_IDLE;
   break;
   
 case ICACHE_UPDT:
   ins_ber   = m_read_error;

   m_icache.update(ins_addr & m_icache.get_yzmask (), m_read_buffer_ins);
   ins_rdata = m_icache.read(ins_addr & ~0x3);
   m_icache_fsm = ICACHE_IDLE;
   break;
   
 }

///////////////////////////////////////////////////////////////////////////////////
// The DCACHE FSM 
///////////////////////////////////////////////////////////////////////////////////

//A read or write data can be inserted in the buffer when the buffer is not full or 
//the buffer is full but the VCI FSM is in a state that one data will be removed the buffer
bool push_ok = (!m_buf.full() || m_vci_cmd_fsm==CMD_DATA_MISS || m_vci_cmd_fsm==CMD_DATA_UNC_READ || m_vci_cmd_fsm==CMD_DATA_UNC_READ_LINKED || m_vci_cmd_fsm==CMD_DATA_UNC_STORE_COND || m_vci_cmd_fsm==CMD_DATA_WRITE);

switch ((dcache_fsm_state_e)m_dcache_fsm) {
  
 case DCACHE_INIT:
   m_dcache_frz = true;
   m_dcache_cpt_init = m_dcache_cpt_init - 1;
   if (m_dcache_cpt_init == 0)
     m_dcache_fsm = DCACHE_IDLE;
   break;
   
 case DCACHE_WRITE_REQ:
   if(!push_ok){
     if(dcache_req)
       m_dcache_frz = true;
     break;
   }
   
   m_dcache_fsm = DCACHE_IDLE;
 case DCACHE_IDLE:
   {
     m_dcache_frz = false;
     
     if(!dcache_req)
       break;
     
     switch(data_type) {
     case iss_t::READ_WORD:
     case iss_t::READ_HALF:
     case iss_t::READ_BYTE:
      
       if(m_cacheability_table[data_addr]){
	 if (m_dcache.miss (data_addr)){
	   if(push_ok){
	     m_dcache_cpt_uncache_read++;
	     m_cpt_fifo_write++;
	     m_buf.push (data_addr, data_type, data_wdata, (m_pdes_local_time->get() + UNIT_TIME));
	     m_dcache_miss_req = true;
	     m_dcache_fsm = DCACHE_MISS_REQ;
	   }
	   m_dcache_frz = true;
	 }
	 else{
	   m_dcache_cpt_cache_read++;
	   data_ber = false;
	   typename vci_param::data_t data = m_dcache.read (data_addr & ~0x3);
	   
	   if (data_type == iss_t::READ_HALF){
	     data = 0xffff & (data >> (8 * (data_addr & 0x3)));
	     data = data | (data << 16);
	   }
	   else if (data_type == iss_t::READ_BYTE){
	     data = 0xff & (data >> (8 * (data_addr & 0x3)));
	     data = data | (data << 8) | (data << 16) | (data << 24);
	   }
           
	   data_rdata = data;
	   m_dcache_fsm = DCACHE_IDLE;
	 }
	 break;
       }
     case iss_t::READ_LINKED:
       if(m_dcache_unc_valid){
	 m_dcache_unc_valid = false;
	 m_dcache_fsm = DCACHE_IDLE;
       }
       else{
	 if(push_ok){
	   m_dcache_cpt_uncache_read++;
	   m_cpt_fifo_write++;
	   m_buf.push (data_addr, data_type, data_wdata, (m_pdes_local_time->get() + UNIT_TIME));
	   m_dcache_unc_req = true;
	   m_dcache_fsm = DCACHE_UNC_REQ;
	 }
	 m_dcache_frz = true;
       }
       break;
     case iss_t::STORE_COND:
       
       if(m_dcache_unc_valid){
	 m_dcache_unc_valid = false;
	 m_dcache_fsm = DCACHE_IDLE;
       }
       else{
	 if(push_ok){
	   m_dcache_cpt_uncache_write++;
	   m_cpt_fifo_write++;
	   m_buf.push (data_addr, data_type, data_wdata, (m_pdes_local_time->get() + UNIT_TIME));
	   m_dcache_unc_req = true;
	   m_dcache_fsm = DCACHE_UNC_REQ;
	 }
	 m_dcache_frz = true;
       }
       break;
     case iss_t::LINE_INVAL:
       data_rdata = -1;
       if(!m_dcache.miss(data_addr)){
	 m_dcache_cpt_cache_write++;
	 m_dcache.inval (data_addr & ~0x3);
	 m_dcache_fsm = DCACHE_INVAL;
       }
       else
	 m_dcache_fsm = DCACHE_IDLE;
       break;
     case iss_t::WRITE_WORD:
     case iss_t::WRITE_HALF:
     case iss_t::WRITE_BYTE:
       if (!m_dcache.miss(data_addr)){
	 if(push_ok){
	   m_dcache_cpt_cache_write++;
	   m_dcache_cpt_uncache_write++;
	   m_cpt_fifo_write++;
	   
	   typename vci_param::data_t previous_data = m_dcache.read(data_addr & ~0x3);
	   typename vci_param::data_t mask, data, new_data = 0;
	   int byte = data_addr & 0x3;
	   
	   if(data_type == iss_t::WRITE_WORD){
	     new_data = data_wdata;
	   }
	   else if(data_type == iss_t::WRITE_HALF){
	     mask = 0xffff << (byte*8);
	     data = data_wdata << (byte*8);
	     new_data = (previous_data & ~mask) | (data & mask);
	   }
	   else if(data_type == iss_t::WRITE_BYTE){
	     mask = 0xff << (byte*8);
	     data = data_wdata << (byte*8);
	     new_data = (previous_data & ~mask) | (data & mask);
	   }
	   m_dcache.write (data_addr & ~0x3, new_data);
	   
	   m_buf.push (data_addr, data_type, data_wdata,  (m_pdes_local_time->get() + (2*UNIT_TIME)));
	   m_dcache_frz = false;
	   m_dcache_fsm = DCACHE_WRITE_UPDT;
	 }
	 else
	   m_dcache_frz = true;
       }
       else{
	 if(push_ok){
	   m_dcache_cpt_uncache_write++;
	   m_cpt_fifo_write++;
	   m_buf.push (data_addr, data_type, data_wdata,  (m_pdes_local_time->get() + UNIT_TIME));
	   m_dcache_frz = false;
	   m_dcache_fsm = DCACHE_WRITE_REQ;
	 }
	 else
	   m_dcache_frz = true;
       }
       break;
     }
     break;
   }
   
 case DCACHE_WRITE_UPDT:
   {
     if(dcache_req)
       m_dcache_frz = true;
     m_dcache_fsm = DCACHE_WRITE_REQ;
     break;
   }
   
 case DCACHE_MISS_REQ:
   m_dcache_fsm = DCACHE_MISS_WAIT;
   break;
   
 case DCACHE_MISS_WAIT:
   if (m_vci_rsp_fsm == RSP_DATA_READ_ERROR)
     m_dcache_fsm = DCACHE_ERROR;
   if (m_vci_rsp_fsm == RSP_DATA_MISS_OK)
     m_dcache_fsm = DCACHE_MISS_UPDT;
   break;
   
 case DCACHE_MISS_UPDT:
   {
     data_ber = m_read_error;

     for(unsigned int i=0;i<(m_payload.get_data_length()/vci_param::nbytes); i++)
       m_read_buffer[i] = atou(m_payload.get_data_ptr(), (i * vci_param::nbytes));

     m_dcache.update ((data_addr & m_dcache.get_yzmask ()), m_read_buffer);
     
     typename vci_param::data_t data = m_dcache.read (data_addr & ~0x3);
     if (data_type == iss_t::READ_HALF){
       data = 0xffff & (data >> (8 * (data_addr & 0x3)));
       data = data | (data << 16);
     }
     else if (data_type == iss_t::READ_BYTE){
       data = 0xff & (data >> (8 * (data_addr & 0x3)));
       data = data | (data << 8) | (data << 16) | (data << 24);
     }
     
     data_rdata = data;
     m_dcache_fsm = DCACHE_IDLE;
     break;
   }
   
 case DCACHE_UNC_REQ:
   m_dcache_fsm = DCACHE_UNC_WAIT;
   break;
   
 case DCACHE_UNC_WAIT:
   data_ber = m_read_error;
   if (m_vci_rsp_fsm == RSP_DATA_READ_ERROR)
     m_dcache_fsm = DCACHE_ERROR;
   if (m_vci_rsp_fsm == RSP_DATA_UNC_OK)
     m_dcache_fsm = DCACHE_IDLE;
   break;
   
 case DCACHE_ERROR:
   m_dcache_fsm = DCACHE_IDLE;
   break;
   
 case DCACHE_INVAL:
   if(dcache_req)
     m_dcache_frz = true;
   data_ber = false;
   m_dcache_fsm = DCACHE_IDLE;
   break;
 }

///////////////////////////////////////////////////////////////////////////////////
// The VCI_CMD FSM 
///////////////////////////////////////////////////////////////////////////////////

 switch ((cmd_fsm_state_e)m_vci_cmd_fsm) {
 case CMD_IDLE:
   if (m_vci_rsp_fsm != RSP_IDLE)
     break;
   
   //In the CABA model, the request is affected for the icache and dcache state machine in the end of cycle. 
   //Therefore the request will be treated for the vci state machine only in the next cycle because all state machines are evaluated in the same cycle.
   //In the TLMT model, the request is directly affected, therefore the request can be treated in the same cycle.
   //However for TLMT model to keep the same precision than CABA model, a request time is stored and 
   //the request is treated only when the processor time is greater than request time.
   
   if (m_icache_req && m_pdes_local_time->get()>m_req_icache_time) {
     m_vci_cmd_fsm = CMD_INS_MISS;
   }
   else if (!m_buf.empty()){
     if(m_pdes_local_time->get()>m_buf.getTime()){
       enum iss_t::DataAccessType req_type = m_buf.getType ();
       switch(req_type) {
       case iss_t::READ_WORD:
       case iss_t::READ_HALF:
       case iss_t::READ_BYTE:
	 if(m_cacheability_table[m_buf.getAddress()])
	   m_vci_cmd_fsm = CMD_DATA_MISS;
	 else
	   m_vci_cmd_fsm = CMD_DATA_UNC_READ;
	 break;
       case iss_t::STORE_COND:
	 m_vci_cmd_fsm = CMD_DATA_UNC_STORE_COND;
	 break;
       case iss_t::READ_LINKED:
	 m_vci_cmd_fsm = CMD_DATA_UNC_READ_LINKED;
	 break;
       case iss_t::WRITE_WORD:
       case iss_t::WRITE_HALF:
       case iss_t::WRITE_BYTE:
	 m_vci_cmd_fsm = CMD_DATA_WRITE;
	 break;
       case iss_t::LINE_INVAL:
	 assert(0&&"This should not happen");
       }
     }
   }
   break;
   
 case CMD_INS_MISS:
   m_nbytes = m_icache.get_nwords() * vci_param::nbytes;
   for(unsigned int i=0, j=0; i<m_icache.get_nwords(); i++, j+=vci_param::nbytes){
     utoa(WORD_ENABLED, m_byte_enable_ptr, j);
     utoa(m_read_buffer_ins[i], m_data_ptr, j);
   }

   // set the values in tlm payload
   m_payload.set_command(tlm::TLM_IGNORE_COMMAND);
   m_payload.set_address(ins_addr & m_icache.get_yzmask ());
   m_payload.set_byte_enable_ptr(m_byte_enable_ptr);
   m_payload.set_byte_enable_length(m_nbytes);
   m_payload.set_data_ptr(m_data_ptr);
   m_payload.set_data_length(m_nbytes);
   // set the values in payload extension
   m_extension_ptr->set_read();
   m_extension_ptr->set_src_id(m_srcid);
   m_extension_ptr->set_trd_id(0);
   m_extension_ptr->set_pkt_id(0);
   // set the extension to tlm payload
   m_payload.set_extension(m_extension_ptr);
   // set the tlm phase
   m_phase = tlm::BEGIN_REQ;
   // set the local time to transaction time
   m_send_time = m_pdes_local_time->get();
   // send the transaction
   p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_send_time);

   m_vci_cmd_fsm = CMD_IDLE;
   break;
   
 case CMD_DATA_UNC_READ:
#if FSM_DEBUG
   fprintf (pFile," CMD_DATA_UNC");
   std::cout << " CMD_DATA_UNC";
#endif

   utoa(WORD_ENABLED, m_byte_enable_ptr, 0);
   utoa(m_read_buffer[0], m_data_ptr, 0);
   m_nbytes = vci_param::nbytes;

    // set the values in tlm payload
   m_payload.set_command(tlm::TLM_IGNORE_COMMAND);
   m_payload.set_address(m_buf.getAddress() & ~0x3);
   m_payload.set_byte_enable_ptr(m_byte_enable_ptr);
   m_payload.set_byte_enable_length(m_nbytes);
   m_payload.set_data_ptr(m_data_ptr);
   m_payload.set_data_length(m_nbytes);
   // set the values in payload extension
   m_extension_ptr->set_read();
   m_extension_ptr->set_src_id(m_srcid);
   m_extension_ptr->set_trd_id(0);
   m_extension_ptr->set_pkt_id(0);
   // set the extension to tlm payload
   m_payload.set_extension(m_extension_ptr);

   m_buf.popData();
   m_cpt_fifo_read++;

   // set the tlm phase
   m_phase = tlm::BEGIN_REQ;
   // set the local time to transaction time
   m_send_time = m_pdes_local_time->get();
   // send the transaction
   p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_send_time);

   m_vci_cmd_fsm = CMD_IDLE;
   break;
   
 case CMD_DATA_UNC_READ_LINKED:
#if FSM_DEBUG
   fprintf (pFile," CMD_DATA_UNC");
   std::cout << " CMD_DATA_UNC_READ_LINKED";
#endif

   m_nbytes = vci_param::nbytes;
   utoa(WORD_ENABLED, m_byte_enable_ptr, 0);
   utoa(m_read_buffer[0], m_data_ptr, 0);

   // set the values in tlm payload
   m_payload.set_command(tlm::TLM_IGNORE_COMMAND);
   m_payload.set_address(m_buf.getAddress() & ~0x3);
   m_payload.set_byte_enable_ptr(m_byte_enable_ptr);
   m_payload.set_byte_enable_length(m_nbytes);
   m_payload.set_data_ptr(m_data_ptr);
   m_payload.set_data_length(m_nbytes);
   // set the values in payload extension
   m_extension_ptr->set_locked_read();
   m_extension_ptr->set_src_id(m_srcid);
   m_extension_ptr->set_trd_id(0);
   m_extension_ptr->set_pkt_id(0);
   // set the extension to tlm payload
   m_payload.set_extension(m_extension_ptr);

   m_buf.popData();
   m_cpt_fifo_read++;

   // set the tlm phase
   m_phase = tlm::BEGIN_REQ;
   // set the local time to transaction time
   m_send_time = m_pdes_local_time->get();
   // send the transaction
   p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_send_time);

   m_vci_cmd_fsm = CMD_IDLE;
   break;
   
 case CMD_DATA_UNC_STORE_COND:
   {
#if FSM_DEBUG
     fprintf (pFile," CMD_DATA_UNC");
     std::cout << " CMD_DATA_UNC_STORE_COND";
#endif
     typename vci_param::addr_t address = m_buf.getAddress ();
     m_write_buffer[0] = m_buf.popData ();
     m_cpt_fifo_read++;

     m_nbytes = vci_param::nbytes;
     utoa(WORD_ENABLED, m_byte_enable_ptr, 0);
     utoa(m_write_buffer[0], m_data_ptr, 0);
      
     // set the values in tlm payload
     m_payload.set_command(tlm::TLM_IGNORE_COMMAND);
     m_payload.set_address(address & ~0x3);
     m_payload.set_byte_enable_ptr(m_byte_enable_ptr);
     m_payload.set_byte_enable_length(m_nbytes);
     m_payload.set_data_ptr(m_data_ptr);
     m_payload.set_data_length(m_nbytes);
     // set the values in payload extension
     m_extension_ptr->set_store_cond();
     m_extension_ptr->set_src_id(m_srcid);
     m_extension_ptr->set_trd_id(0);
     m_extension_ptr->set_pkt_id(0);
     // set the extension to tlm payload
     m_payload.set_extension(m_extension_ptr);
     
     // set the tlm phase
     m_phase = tlm::BEGIN_REQ;
     // set the local time to transaction time
     m_send_time = m_pdes_local_time->get();

     //this transaction is a WRITE command
     m_vci_write = true;

     // send the transaction
     p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_send_time);

     m_vci_cmd_fsm = CMD_IDLE;
     break;
   }
   
 case CMD_DATA_MISS:
#if FSM_DEBUG
   fprintf (pFile," CMD_DATA_MISS");
   std::cout << " CMD_DATA_MISS";
#endif

   m_nbytes = m_dcache.get_nwords() * vci_param::nbytes;
   for(unsigned int i=0, j=0; i<m_dcache.get_nwords(); i++, j+=vci_param::nbytes){
     utoa(WORD_ENABLED, m_byte_enable_ptr, j);
     utoa(m_read_buffer[i], m_data_ptr, j);
   }
 
   // set the values in tlm payload
   m_payload.set_command(tlm::TLM_IGNORE_COMMAND);
   m_payload.set_read();
   m_payload.set_address(m_buf.getAddress() & m_dcache.get_yzmask());
   m_payload.set_byte_enable_ptr(m_byte_enable_ptr);
   m_payload.set_byte_enable_length(m_nbytes);
   m_payload.set_data_ptr(m_data_ptr);
   m_payload.set_data_length(m_nbytes);
   // set the values in payload extension
   m_extension_ptr->set_read();
   m_extension_ptr->set_src_id(m_srcid);
   m_extension_ptr->set_trd_id(0);
   m_extension_ptr->set_pkt_id(0);
   // set the extension to tlm payload
   m_payload.set_extension(m_extension_ptr);

   m_buf.popData();
   m_cpt_fifo_read++;

   // set the tlm phase
   m_phase = tlm::BEGIN_REQ;
   // set the local time to transaction time
   m_send_time = m_pdes_local_time->get();
   // send the transaction
   p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_send_time);

   m_vci_cmd_fsm = CMD_IDLE;
   break;
   
 case CMD_DATA_WRITE:
   {
#if FSM_DEBUG
     fprintf (pFile," CMD_DATA_WRITE");
     std::cout << " CMD_DATA_WRITE";
#endif

     //the write vci transaction have only one word

     typename vci_param::addr_t address = m_buf.getAddress ();
     const int subcell = address & 0x3;      
     
     if (m_buf.getType () == iss_t::WRITE_WORD)
       utoa(WORD_ENABLED, m_byte_enable_ptr, 0);
     else if (m_buf.getType () == iss_t::WRITE_HALF){
       if(subcell == 0)
	 utoa(LO_WORD_ENABLED, m_byte_enable_ptr, 0);
       else 
	 utoa(HI_WORD_ENABLED, m_byte_enable_ptr, 0);
     }
     else if (m_buf.getType () == iss_t::WRITE_BYTE){
       if(subcell == 0)
	 utoa(BYTE0_ENABLED, m_byte_enable_ptr, 0);
       else if(subcell == 1)
         utoa(BYTE1_ENABLED, m_byte_enable_ptr, 0);
       else if(subcell == 2)
         utoa(BYTE2_ENABLED, m_byte_enable_ptr, 0);
       else
         utoa(BYTE3_ENABLED, m_byte_enable_ptr, 0);
     }

     m_write_buffer[0] = m_buf.popData();
     m_cpt_fifo_read++;

     m_nbytes = vci_param::nbytes;
     utoa(m_write_buffer[0], m_data_ptr, 0);

     // set the values in tlm payload
     m_payload.set_command(tlm::TLM_IGNORE_COMMAND);
     m_payload.set_address(address & ~0x3);
     m_payload.set_byte_enable_ptr(m_byte_enable_ptr);
     m_payload.set_byte_enable_length(m_nbytes);
     m_payload.set_data_ptr(m_data_ptr);
     m_payload.set_data_length(m_nbytes);
     // set the values in payload extension
     m_extension_ptr->set_write();
     m_extension_ptr->set_src_id(m_srcid);
     m_extension_ptr->set_trd_id(0);
     m_extension_ptr->set_pkt_id(0);
     // set the extension to tlm payload
     m_payload.set_extension(m_extension_ptr);
     
     // set the tlm phase
     m_phase = tlm::BEGIN_REQ;
     // set the local time to transaction time
     m_send_time = m_pdes_local_time->get();
     //this transaction is a WRITE command
     m_vci_write = true;
     // send the transaction
     p_vci_initiator->nb_transport_fw(m_payload, m_phase, m_send_time);
     
     m_vci_cmd_fsm = CMD_IDLE;
     break;
   }
 }

///////////////////////////////////////////////////////////////////////////////////
// The VCI_RSP FSM 
///////////////////////////////////////////////////////////////////////////////////

switch ((rsp_fsm_state_e)m_vci_rsp_fsm) {
  
 case RSP_IDLE:
#if FSM_DEBUG
   fprintf (pFile," RSP_IDLE\n");
   std::cout << " RSP_IDLE" << std::endl;
#endif
   
   if (m_icache_req && m_pdes_local_time->get()>m_req_icache_time) {
     m_vci_rsp_fsm = RSP_INS_MISS;
   } else if (!m_buf.empty()){
     if(m_pdes_local_time->get()>m_buf.getTime()){
       enum iss_t::DataAccessType req_type = m_buf.getType ();
       switch(req_type) {
       case iss_t::READ_WORD:
       case iss_t::READ_HALF:
       case iss_t::READ_BYTE:
	 if(m_cacheability_table[m_buf.getAddress()]){
	   m_vci_rsp_fsm = RSP_DATA_MISS;
	   break;
	 }
       case iss_t::READ_LINKED:
	 m_vci_rsp_fsm = RSP_DATA_UNC;
	 break;
       case iss_t::STORE_COND:
	 m_vci_rsp_fsm = RSP_DATA_WRITE_UNC;
	 break;
       case iss_t::WRITE_WORD:
       case iss_t::WRITE_HALF:
       case iss_t::WRITE_BYTE:
	 m_vci_rsp_fsm = RSP_DATA_WRITE;
	 break;
       case iss_t::LINE_INVAL:
	 assert(0&&"This should not happen");
       }
     }
   }
   break;
   
 case RSP_INS_MISS:
   wait (m_rsp_received);
   for(unsigned int i=0;i<(m_payload.get_data_length()/vci_param::nbytes); i++)
     m_read_buffer_ins[i] = atou(m_payload.get_data_ptr(), (i * vci_param::nbytes));
   
   if ( m_read_error == 0) {
     m_vci_rsp_fsm = RSP_INS_OK;
   } else {
     m_vci_rsp_fsm = RSP_INS_ERROR;
   }
   break;
   
 case RSP_INS_OK:
   m_vci_rsp_fsm = RSP_IDLE;
   m_icache_req = false;
   break;
   
 case RSP_INS_ERROR_WAIT:
   break;
   
 case RSP_INS_ERROR:
   m_vci_rsp_fsm = RSP_IDLE;
   m_icache_req = false;
   break;
        
 case RSP_DATA_MISS:
   wait (m_rsp_received);
   for(unsigned int i=0;i<(m_payload.get_data_length()/vci_param::nbytes); i++)
     m_read_buffer[i] = atou(m_payload.get_data_ptr(), (i * vci_param::nbytes));
   
   if ( m_read_error == 0) {
     m_vci_rsp_fsm = RSP_DATA_MISS_OK;
   } else {
     m_vci_rsp_fsm = RSP_DATA_READ_ERROR;
   }
   break;
   
 case RSP_DATA_WRITE:
   wait (m_rsp_received);
   
   for(unsigned int i=0;i<(m_payload.get_data_length()/vci_param::nbytes); i++)
     m_write_buffer[i] = atou(m_payload.get_data_ptr(), (i * vci_param::nbytes));

   //Blocked until processor time greater or equal to vci response time
   //This is necessary the write vci transaction does not blocking the processor 
   //but any other vci transaction can be executed before write vci transaction has finished
   if(m_pdes_local_time->get() >= m_rsp_vci_time){
	m_vci_write = false;
	if ( m_write_error == 0)
	  m_vci_rsp_fsm = RSP_IDLE;
	else
	  m_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
   }
   else
     m_vci_rsp_fsm = RSP_WAIT_TIME;
   break;
 case RSP_WAIT_TIME:
   //Blocked until processor time greater or equal to vci_response time
   //This is necessary the write vci transaction does not blocking the processor 
   //but any other vci transaction can be executed before write vci transaction has finished
   if(m_pdes_local_time->get() >= m_rsp_vci_time){
     m_vci_write = false;
     if ( m_write_error == 0)
       m_vci_rsp_fsm = RSP_IDLE;
     else
       m_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
   }
   break;
 case RSP_DATA_UNC:
   wait (m_rsp_received);
   
   if ( m_read_error == 0) {
     data_ber = m_read_error;
     typename vci_param::data_t data =  atou(m_payload.get_data_ptr(), 0);
     
     if (data_type == iss_t::READ_HALF){
       data = 0xffff & (data >> (8 * (data_addr & 0x3)));
       data = data | (data << 16);
     }
     else if (data_type == iss_t::READ_BYTE){
       data = 0xff & (data >> (8 * (data_addr & 0x3)));
       data = data | (data << 8) | (data << 16) | (data << 24);
     }
     
     data_rdata = data;
     m_dcache_unc_valid = true;
     m_vci_rsp_fsm = RSP_DATA_UNC_OK;
   } else {
     m_vci_rsp_fsm = RSP_DATA_READ_ERROR;
   }
   break;
 case RSP_DATA_WRITE_UNC:
   wait (m_rsp_received);
   data_rdata = atou(m_payload.get_data_ptr(), 0);;
   m_dcache_unc_valid = true;
	
   //Blocked until processor time greater or equal to vci_response time
   //This is necessary the write vci transaction does not blocking the processor 
   //but any other vci transaction can be executed before write vci transaction has finished
   if(m_pdes_local_time->get() >= m_rsp_vci_time){
     m_vci_write = false;
     if ( m_write_error == 0){
       m_vci_rsp_fsm = RSP_DATA_UNC_OK;
     }
     else
       m_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
   }
   else
     m_vci_rsp_fsm = RSP_UNC_WAIT_TIME;
   break;
 case RSP_UNC_WAIT_TIME:
   //Blocked until processor time greater or equal to vci_response time
   //This is necessary the write vci transaction does not blocking the processor 
   //but any other vci transaction can be executed before write vci transaction has finished
   if(m_pdes_local_time->get() >= m_rsp_vci_time){
     m_vci_write = false;
     if ( m_write_error == 0)
       m_vci_rsp_fsm = RSP_DATA_UNC_OK;
     else
       m_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
   }
   break;
   
 case RSP_DATA_WRITE_ERROR:
 case RSP_DATA_UNC_OK:
 case RSP_DATA_MISS_OK:
 case RSP_DATA_READ_ERROR:
   m_dcache_miss_req = false;
   m_dcache_unc_req = false;
   m_vci_rsp_fsm = RSP_IDLE;
   break;
   
 case RSP_DATA_READ_ERROR_WAIT:
   break;
   
 case RSP_DATA_WRITE_ERROR_WAIT:
   break;
 }
}

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

  m_write_error = false;
  m_read_error = false;

  if (extension_ptr->is_write())
    m_write_error = payload.is_response_error();
    
  if (extension_ptr->is_read())
    m_read_error = payload.is_response_error();
    
  //The write vci transaction is not blocking because of this the response of write vci transaction does not update the processor time. 
  //However a write vci transaction is not blocking, any other vci transaction can be treated while it has not finished.
  //For this reason a write vci transaction blocks the VCI FSM while the processor time lesser than the response time (m_rsp_vci_time).
  m_rsp_vci_time = time;
  
  //m_vci_write is active when the vci transaction command is WRITE  (payload.cmd == vci_param::CMD_WRITE)
  if(!m_vci_write){
    m_cpt_idle = m_cpt_idle + (time.value() - m_pdes_local_time->get().value());
    //std::cout << "XCACHE RECEIVE REPONSE time = " << std::dec << time.value() << std::endl;
    updateTime(time);
  }
    
  m_rsp_received.notify (sc_core::SC_ZERO_TIME);
  return tlm::TLM_COMPLETED;
} // end backward nb transport 

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (IRQ SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::my_nb_transport_fw
( int                      id,         // interruption id
  tlm::tlm_generic_payload &payload,   // payload
  tlm::tlm_phase           &phase,     // phase
  sc_core::sc_time         &time)      // time
{
#if XCACHE_DEBUG
  std::cout << "[XCACHE " << m_srcid << "] receive Interruption " << id << " time = " << time << std::endl;
#endif

  return tlm::TLM_COMPLETED;
} // end backward nb transport 


}}
