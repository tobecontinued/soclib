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
 * Maintainers: fpecheux, nipo, alinev
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr> 
 */


#include "../include/vci_xcache_wrapper.h"

#ifndef XCACHE_DEBUG
#define XCACHE_DEBUG 0
#endif

#ifndef MY_XCACHE_DEBUG
#define MY_XCACHE_DEBUG 0
#endif

namespace soclib{ namespace tlmt {
    
#define tmpl(x) template<typename iss_t, typename vci_param> x VciXcacheWrapper<iss_t,vci_param>


  tmpl (tlmt_core::tlmt_return &)::rspReceived (soclib::tlmt:: vci_rsp_packet < vci_param > *pkt,
						const tlmt_core:: tlmt_time & time, void *private_data)
  {
    if (m_cmd.cmd == vci_param::CMD_WRITE)
      m_write_error = (pkt->error != 0);
    else
      m_write_error = false;
    
    if (m_cmd.cmd == vci_param::CMD_READ)
      m_read_error = (pkt->error != 0);
    else
      m_read_error = false;
    
    //The write vci transaction is not blocking because of this the response of write vci transaction does not update the processor time. 
    //However a write vci transaction is not blocking, any other vci transaction can be treated while it has not finished.
    //For this reason a write vci transaction blocks the VCI FSM while the processor time lesser than the response time (m_rsp_vci_time).
    m_rsp_vci_time = time;
    
    //m_vci_write is active when the vci transaction command is WRITE  (m_cmd.cmd == vci_param::CMD_WRITE)
    if(!m_vci_write)
      update_time(time);
    
    m_rsp_received.notify (sc_core::SC_ZERO_TIME);
    
    return m_return;
  }

  tmpl (tlmt_core::tlmt_return &)::irqReceived (bool v, const tlmt_core::
						tlmt_time & time, void *private_data)
  {
    int no = (int)(long)private_data;
    std::cout << name () << " callback_synchro " << no << " " << v << std::endl;
    return m_return;
  }

  tmpl (void)::update_time(tlmt_core::tlmt_time t){
    if(t > c0.time())
      m_iss.nullStep(t - c0.time());
    c0.update_time(t);
  }

  tmpl (void)::add_time(tlmt_core::tlmt_time t){
    m_iss.nullStep(t);
    c0.add_time(t);
  }


  tmpl (/**/)::VciXcacheWrapper(sc_core::sc_module_name name,
				int id,
				const soclib::common::MappingTable &mt,
				size_t icache_lines,
				size_t icache_words,
				size_t dcache_lines,
				size_t dcache_words )
    : soclib::tlmt::BaseModule (name),
      m_id(id),
      m_cacheability_table(mt.getCacheabilityTable()),
      m_iss(id),
      m_buf(100), 
      m_dcache(dcache_lines, dcache_words),
      m_icache(icache_lines, icache_words),
      p_vci("vci", new tlmt_core::tlmt_callback<VciXcacheWrapper,vci_rsp_packet<vci_param>*>(this,&VciXcacheWrapper<iss_t,vci_param>::rspReceived),&c0)
  {
    
    p_irq = (tlmt_core::tlmt_in<bool>*)malloc(sizeof(tlmt_core::tlmt_in<bool>)*iss_t::n_irq);
    for (int32_t i = 0 ; i < iss_t::n_irq ; i++) {
      std::ostringstream o;
      o << "irq[" << i << "]";
      new(&p_irq[i])tlmt_core::tlmt_in<bool> (o.str(), 
					      new tlmt_core::tlmt_callback<VciXcacheWrapper, bool>(this,&VciXcacheWrapper<iss_t, vci_param>::irqReceived, (void*)(long)i));
    }

    m_counter           = 0;
    m_lookahead         = 10; 
   
    m_dcache_fsm       = DCACHE_INIT;
    m_icache_fsm       = ICACHE_INIT;
    m_vci_cmd_fsm      = CMD_IDLE;
    m_vci_rsp_fsm      = RSP_IDLE;
    
    m_dcache_cpt_init  = dcache_lines;
    m_icache_cpt_init  = icache_lines;
    
    m_rsp_vci_time     = 0;
    m_req_icache_time  = 0;

    m_vci_write        = false;
    m_icache_req       = false;
    m_dcache_miss_req  = false;
    m_dcache_unc_req   = false;
    m_dcache_unc_valid = false;

    m_icache_frz       = false;
    m_dcache_frz       = false;

    m_read_error       = false;
    m_write_error      = false;

    //the last parameter(1) must be modified by dcache_lines
    m_iss.setDCacheInfo(32,1,1);
    m_iss.setICacheInfo(32,1,icache_lines);
    m_iss.reset ();

    SC_THREAD(execLoop);
  }

  tmpl (void)::execLoop ()
  {
    
    while(c0.time()<tlmt_core::tlmt_time(1000000000)){
      //for (;;){

      enum iss_t::DataAccessType data_type;


      typename vci_param::addr_t ins_addr = 0;
      typename vci_param::data_t ins_rdata;
      bool                       ins_asked = false;
      bool                       ins_ber = false;

      typename vci_param::addr_t data_addr = 0;
      typename vci_param::data_t data_wdata;
      typename vci_param::data_t data_rdata;
      bool                       data_asked = false;
      bool                       data_ber = false;
 
      m_iss.getInstructionRequest(ins_asked, ins_addr);
            
      m_iss.getDataRequest(data_asked, data_type, data_addr, data_wdata);
            
      xcacheAccess(ins_asked, ins_addr,
		   data_asked, data_type, data_addr, data_wdata,
		   data_rdata, data_ber, ins_rdata, ins_ber);
      
            
      if(ins_asked && !m_icache_frz)
	m_iss.setInstruction(ins_ber, ins_rdata);
      
      if(data_asked && !m_dcache_frz)
	m_iss.setDataResponse(data_ber, data_rdata);
      else if (data_ber)
	m_iss.setWriteBerr ();

      if(m_iss.isBusy() || m_icache_frz || m_dcache_frz){
	add_time(1);
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

	//m_iss.dump();
	m_iss.step();
	c0.add_time(1);
            
	m_counter++;
	if (m_counter >= m_lookahead){
	  m_counter = 0;
	  wait (sc_core::SC_ZERO_TIME);
	}
      }
    }
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

#if MY_XCACHE_DEBUG
    std::cout << "N_CYCLES " << std::dec << c0.time() << " IREQ = " << icache_req << std::hex << " I_ADDR = " << ins_addr << " DREQ = " << dcache_req << " D_ADDR = " << data_addr << " D_WRITE = " << data_wdata << std::dec;
#endif

    ///////////////////////////////////////////////////////////////////////////////////
    // The ICACHE FSM 
    ///////////////////////////////////////////////////////////////////////////////////

    switch((icache_fsm_state_e)m_icache_fsm) {

    case ICACHE_INIT:
#if MY_XCACHE_DEBUG
      std::cout << " ICACHE_INIT";
#endif
      m_icache_frz = true;
      m_icache_cpt_init = m_icache_cpt_init - 1;
      if (m_icache_cpt_init == 0)
	m_icache_fsm = ICACHE_IDLE;
      break;
    
    case ICACHE_IDLE:
#if MY_XCACHE_DEBUG
      std::cout << " ICACHE_IDLE";
#endif
      if ( icache_req ) {
	if (m_icache.miss (ins_addr)){
	  m_icache_frz = true;
	  m_icache_req = true;
	  m_req_icache_time = c0.time();
	  m_icache_fsm = ICACHE_WAIT;
	}
	else{
	  m_icache_frz = false;
	  ins_ber = false;
	  ins_rdata = m_icache.read(ins_addr & ~0x3);
	}
      }
      else
	m_icache_frz = false;
      break;

    case ICACHE_WAIT:
#if MY_XCACHE_DEBUG
      std::cout << " ICACHE_WAIT";
#endif
      if (m_vci_rsp_fsm == RSP_INS_OK)
	m_icache_fsm = ICACHE_UPDT;
      if (m_vci_rsp_fsm == RSP_INS_ERROR)
	m_icache_fsm = ICACHE_ERROR;
      break;

    case ICACHE_ERROR:
#if MY_XCACHE_DEBUG
      std::cout << " ICACHE_ERROR";
#endif

#if XCACHE_DEBUG
      std::cout << "[XCACHE] ins berr &" << ins_addr << std::endl;
#endif
      m_icache_frz = false;
      m_icache_fsm = ICACHE_IDLE;
      break;

    case ICACHE_UPDT:
#if MY_XCACHE_DEBUG
      std::cout << " ICACHE_UPDT";
#endif
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
    bool push_ok = (!m_buf.full() || m_vci_cmd_fsm==CMD_DATA_MISS || m_vci_cmd_fsm==CMD_DATA_UNC || m_vci_cmd_fsm==CMD_DATA_WRITE);

    switch ((dcache_fsm_state_e)m_dcache_fsm) {

    case DCACHE_INIT:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_INIT";
#endif
      m_dcache_frz = true;
      m_dcache_cpt_init = m_dcache_cpt_init - 1;
      if (m_dcache_cpt_init == 0)
	m_dcache_fsm = DCACHE_IDLE;
      break;

    case DCACHE_WRITE_REQ:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_WRITE_REQ";
#endif
      m_dcache_fsm = DCACHE_IDLE;
    case DCACHE_IDLE:
      {
#if MY_XCACHE_DEBUG
	std::cout << " DCACHE_IDLE";
#endif
	m_dcache_frz = false;

	if(!dcache_req)
	  break;

	switch(data_type) {
	case iss_t::READ_WORD:
	case iss_t::READ_HALF:
	case iss_t::READ_BYTE:

#if XCACHE_DEBUG
	  std::cout << "[XCACHE] dcache read @" << std::hex << data_addr << " " << ( m_cacheability_table[data_addr] ? "cached" : "uncached" ) << std::endl;
#endif
	  if(m_cacheability_table[data_addr]){
	    if (m_dcache.miss (data_addr)){
	      if(push_ok){
		m_buf.push (data_addr, data_type, data_wdata, c0.time() + tlmt_core::tlmt_time(1));
		m_dcache_miss_req = true;
		m_dcache_fsm = DCACHE_MISS_REQ;
	      }
	      m_dcache_frz = true;
	    }
	    else{
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
	case iss_t::STORE_COND:

	  if(m_dcache_unc_valid){
#if XCACHE_DEBUG
	    std::cout << "[XCACHE] Unc hit" << std::endl;
#endif
	    m_dcache_unc_valid = false;
	    m_dcache_fsm = DCACHE_IDLE;
	  }
	  else{
#if XCACHE_DEBUG
	    std::cout << "[XCACHE] Unc miss" << std::endl;
#endif
	    if(push_ok){
	      m_buf.push (data_addr, data_type, data_wdata, c0.time() + tlmt_core::tlmt_time(1));
	      m_dcache_unc_req = true;
	      m_dcache_fsm = DCACHE_UNC_REQ;
	    }
	    m_dcache_frz = true;
	  }
	  break;
	case iss_t::LINE_INVAL:
	  data_rdata = -1;
	  if(!m_dcache.miss(data_addr))
	    m_dcache_fsm = DCACHE_INVAL;
	  else
	    m_dcache_fsm = DCACHE_IDLE;
	  break;
	case iss_t::WRITE_WORD:
	case iss_t::WRITE_HALF:
	case iss_t::WRITE_BYTE:
	  if (!m_dcache.miss(data_addr)){
	    if(push_ok){
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

	      m_buf.push (data_addr, data_type, data_wdata, c0.time() + tlmt_core::tlmt_time(2));
	      m_dcache_frz = false;
	      m_dcache_fsm = DCACHE_WRITE_UPDT;
	    }
	    else
	      m_dcache_frz = true;
	  }
	  else{
	    if(push_ok){
	      m_buf.push (data_addr, data_type, data_wdata, c0.time() + tlmt_core::tlmt_time(1));
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
#if MY_XCACHE_DEBUG
	std::cout << " DCACHE_WRITE_UPDT";
#endif
	if(dcache_req)
	  m_dcache_frz = true;
	m_dcache_fsm = DCACHE_WRITE_REQ;
	break;
      }
        
    case DCACHE_MISS_REQ:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_MISS_REQ";
#endif
      m_dcache_fsm = DCACHE_MISS_WAIT;
      break;
        
    case DCACHE_MISS_WAIT:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_MISS_WAIT";
#endif
      if (m_vci_rsp_fsm == RSP_DATA_READ_ERROR)
	m_dcache_fsm = DCACHE_ERROR;
      if (m_vci_rsp_fsm == RSP_DATA_MISS_OK)
	m_dcache_fsm = DCACHE_MISS_UPDT;
      break;
        
    case DCACHE_MISS_UPDT:
      {
#if MY_XCACHE_DEBUG
	std::cout << " DCACHE_MISS_UPDT";
#endif
            
	data_ber = m_read_error;
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
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_UNC_REQ";
#endif
      m_dcache_fsm = DCACHE_UNC_WAIT;
      break;

    case DCACHE_UNC_WAIT:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_UNC_WAIT";
#endif
      data_ber = m_read_error;
      if (m_vci_rsp_fsm == RSP_DATA_READ_ERROR)
	m_dcache_fsm = DCACHE_ERROR;
      if (m_vci_rsp_fsm == RSP_DATA_UNC_OK)
	m_dcache_fsm = DCACHE_IDLE;
      break;

    case DCACHE_ERROR:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_ERROR";
#endif

#if XCACHE_DEBUG
      std::cout <<"[XCACHE] data berr " << std::endl;
#endif
      m_dcache_fsm = DCACHE_IDLE;
      break;
        
    case DCACHE_INVAL:
#if MY_XCACHE_DEBUG
      std::cout << " DCACHE_INVAL";
#endif
      m_dcache.inval (data_addr & ~0x3);
      data_ber = false;
      data_rdata = 0;
      m_dcache_fsm = DCACHE_IDLE;
      break;
    }
    
    ///////////////////////////////////////////////////////////////////////////////////
    // The VCI_CMD FSM 
    ///////////////////////////////////////////////////////////////////////////////////

    switch ((cmd_fsm_state_e)m_vci_cmd_fsm) {
    
    case CMD_IDLE:
#if MY_XCACHE_DEBUG
      std::cout << " CMD_IDLE";
#endif
      if (m_vci_rsp_fsm != RSP_IDLE)
	break;

      //In the CABA model, the request is affected for the icache and dcache state machine in the end of cycle. 
      //Therefore the request will be treated for the vci state machine only in the next cycle because all state machines are evaluated in the same cycle.
      //In the TLMT model, the request is directly affected, therefore the request can be treated in the same cycle.
      //However for TLMT model to keep the same precision than CABA model, a request time is stored and 
      //the request is treated only when the processor time is greater than request time.

      if (m_icache_req && c0.time()>m_req_icache_time) {
	m_vci_cmd_fsm = CMD_INS_MISS;
      }
      else if (!m_buf.empty()){
	if(c0.time()>m_buf.getTime()){

	  enum iss_t::DataAccessType req_type = m_buf.getType ();
	  switch(req_type) {
	  case iss_t::READ_WORD:
	  case iss_t::READ_HALF:
	  case iss_t::READ_BYTE:
	    if(m_cacheability_table[m_buf.getAddress()]){
	      m_vci_cmd_fsm = CMD_DATA_MISS;
	      break;
	    }
	  case iss_t::STORE_COND:
	  case iss_t::READ_LINKED:
	    m_vci_cmd_fsm = CMD_DATA_UNC;
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
#if MY_XCACHE_DEBUG
      std::cout << " CMD_INS_MISS";
#endif

      m_cmd.cmd     = vci_param::CMD_READ;
      m_cmd.nwords  = m_icache.get_nwords();
      m_cmd.address = ins_addr & m_icache.get_yzmask ();
      m_cmd.buf     = m_read_buffer_ins;
      m_cmd.be      = 0xF;
      m_cmd.contig  = true;
      m_cmd.srcid   = m_id;
      m_cmd.trdid   = 0;
      m_cmd.pktid   = 0;
	
      p_vci.send(&m_cmd, c0.time());
      m_vci_cmd_fsm = CMD_IDLE;
      break;
        
    case CMD_DATA_UNC:
#if MY_XCACHE_DEBUG
      std::cout << " CMD_DATA_UNC";
#endif

      if(m_buf.getType() == iss_t::STORE_COND){
	typename vci_param::addr_t address = m_buf.getAddress ();
	m_write_buffer[0] = m_buf.popData ();

	m_cmd.cmd     = vci_param::CMD_WRITE;
	m_cmd.nwords  = 1;
	m_cmd.address = address & ~0x3;
	m_cmd.buf     = m_write_buffer;
	m_cmd.be      = 0xF;
	m_cmd.contig  = true;
	m_cmd.srcid   = m_id;
	m_cmd.trdid   = 0;
	m_cmd.pktid   = 0;

	m_vci_write = true;
      }
      else{
	m_cmd.cmd     = vci_param::CMD_READ;
	m_cmd.nwords  = 1;
	m_cmd.address = m_buf.getAddress() & ~0x3;
	m_cmd.buf     = m_read_buffer;
	m_cmd.be      = 0xF;
	m_cmd.contig  = true;
	m_cmd.srcid   = m_id;
	m_cmd.trdid   = 0;
	m_cmd.pktid   = 0;
	m_buf.popData();
      }

      p_vci.send (&m_cmd, c0.time ());
      m_vci_cmd_fsm = CMD_IDLE;
      break;

    case CMD_DATA_MISS:
#if MY_XCACHE_DEBUG
      std::cout << " CMD_DATA_MISS";
#endif
      m_cmd.cmd     = vci_param::CMD_READ;
      m_cmd.nwords  = m_dcache.get_nwords();
      m_cmd.address = m_buf.getAddress() & m_dcache.get_yzmask ();
      m_cmd.buf     = m_read_buffer;
      m_cmd.be      = 0xF;
      m_cmd.contig  = true;
      m_cmd.srcid   = m_id;
      m_cmd.trdid   = 0;
      m_cmd.pktid   = 0;
	
      m_buf.popData ();

      p_vci.send (&m_cmd, c0.time ());
      m_vci_cmd_fsm = CMD_IDLE;
      break;

    case CMD_DATA_WRITE:
#if MY_XCACHE_DEBUG
      std::cout << " CMD_DATA_WRITE";
#endif

      typename vci_param::addr_t address = m_buf.getAddress ();
      const int subcell = address & 0x3;      

      if (m_buf.getType () == iss_t::WRITE_WORD)
	m_cmd.be = 0xF;
      else if (m_buf.getType () == iss_t::WRITE_HALF)
	m_cmd.be = 3 << subcell;
      else if (m_buf.getType () == iss_t::WRITE_BYTE)
	m_cmd.be = 1 << subcell;

      //the write vci transaction have only one word
      m_write_buffer[0] = m_buf.popData ();

      m_cmd.cmd     = vci_param::CMD_WRITE;
      m_cmd.nwords  = 1;
      m_cmd.address = address & ~0x3;
      m_cmd.buf     = m_write_buffer;
      m_cmd.contig  = true;
      m_cmd.srcid   = m_id;
      m_cmd.trdid   = 0;
      m_cmd.pktid   = 0;
      
      m_vci_write = true;
      p_vci.send (&m_cmd, c0.time());
      m_vci_cmd_fsm = CMD_IDLE;
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // The VCI_RSP FSM 
    ///////////////////////////////////////////////////////////////////////////////////

    switch ((rsp_fsm_state_e)m_vci_rsp_fsm) {

    case RSP_IDLE:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_IDLE" << std::endl;
#endif

      if (m_icache_req && c0.time()>m_req_icache_time) {
	m_vci_rsp_fsm = RSP_INS_MISS;
      } else if (!m_buf.empty()){
	if(c0.time()>m_buf.getTime()){
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
#if MY_XCACHE_DEBUG
      std::cout << " RSP_INS_MISS" << std::endl;
#endif
      wait (m_rsp_received);
        
      if ( m_read_error == 0) {
	m_vci_rsp_fsm = RSP_INS_OK;
      } else {
	m_vci_rsp_fsm = RSP_INS_ERROR;
      }
      break;

    case RSP_INS_OK:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_INS_OK" << std::endl;
#endif
      m_vci_rsp_fsm = RSP_IDLE;
      m_icache_req = false;
      break;

    case RSP_INS_ERROR_WAIT:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_INS_ERROR_WAIT" << std::endl;
#endif
      break;
        
    case RSP_INS_ERROR:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_INS_ERROR" << std::endl;
#endif
      m_vci_rsp_fsm = RSP_IDLE;
      m_icache_req = false;
      break;
        
    case RSP_DATA_MISS:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_MISS" << std::endl;
#endif
      wait (m_rsp_received);
        
      if ( m_read_error == 0) {
	m_vci_rsp_fsm = RSP_DATA_MISS_OK;
      } else {
	m_vci_rsp_fsm = RSP_DATA_READ_ERROR;
      }
      break;

    case RSP_DATA_WRITE:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_WRITE" << std::endl;
#endif
      wait (m_rsp_received);
        
#if XCACHE_DEBUG
      std::cout << "[XCACHE] RSP_DATA_WRITE err:" << m_write_error << std::endl;
#endif
      //Blocked until processor time greater or equal to vci response time
      //This is necessary the write vci transaction does not blocking the processor 
      //but any other vci transaction can be executed before write vci transaction has finished
      if(c0.time() >= m_rsp_vci_time){
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
#if MY_XCACHE_DEBUG
      std::cout << " RSP_WAIT_TIME" << std::endl;
#endif
      //Blocked until processor time greater or equal to vci_response time
      //This is necessary the write vci transaction does not blocking the processor 
      //but any other vci transaction can be executed before write vci transaction has finished
      if(c0.time() >= m_rsp_vci_time){
	m_vci_write = false;
	if ( m_write_error == 0)
	  m_vci_rsp_fsm = RSP_IDLE;
	else
	  m_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
      }
      break;
    case RSP_DATA_UNC:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_UNC" << std::endl;
#endif
      wait (m_rsp_received);
      
      if ( m_read_error == 0) {
	data_ber = m_read_error;
	typename vci_param::data_t data = m_read_buffer[0];
	
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
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_WRITE_UNC" << std::endl;
#endif
      wait (m_rsp_received);
      m_dcache_unc_valid = true;
	
      //Blocked until processor time greater or equal to vci_response time
      //This is necessary the write vci transaction does not blocking the processor 
      //but any other vci transaction can be executed before write vci transaction has finished
      if(c0.time() >= m_rsp_vci_time){
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
#if MY_XCACHE_DEBUG
      std::cout << " RSP_UNC_WAIT_TIME" << std::endl;
#endif
      //Blocked until processor time greater or equal to vci_response time
      //This is necessary the write vci transaction does not blocking the processor 
      //but any other vci transaction can be executed before write vci transaction has finished
      if(c0.time() >= m_rsp_vci_time){
	m_vci_write = false;
	if ( m_write_error == 0)
	  m_vci_rsp_fsm = RSP_DATA_UNC_OK;
	else
	  m_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
      }
      break;

    case RSP_DATA_WRITE_ERROR:
#if MY_XCACHE_DEBUG
      std::cout << " DATA_WRITE_ERROR" << std::endl;
#endif
#if XCACHE_DEBUG
      std::cout << "[XCACHE] data WRITE berr" << std::endl;
#endif
    case RSP_DATA_UNC_OK:
    case RSP_DATA_MISS_OK:
    case RSP_DATA_READ_ERROR:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_..." << std::endl;
#endif
      m_dcache_miss_req = false;
      m_dcache_unc_req = false;
      m_vci_rsp_fsm = RSP_IDLE;
      break;

    case RSP_DATA_READ_ERROR_WAIT:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_READ_ERROR_WAIT" << std::endl;
#endif
      break;

    case RSP_DATA_WRITE_ERROR_WAIT:
#if MY_XCACHE_DEBUG
      std::cout << " RSP_DATA_WRITE_ERROR_WAIT" << std::endl;
#endif
      break;
    }
  }
}}
