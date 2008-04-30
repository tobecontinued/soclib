#include "vci_param.h"
#include "mwmr_controller.h"
#include "vci_mwmr_controller.h"

#ifndef MWMR_CONTROLLER_DEBUG
#define MWMR_CONTROLLER_DEBUG 0
#endif

namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciMwmrController<vci_param>

  tmpl(tlmt_core::tlmt_return&)::vciRspReceived(soclib::tlmt::vci_rsp_packet<vci_param> *pkt,
						const tlmt_core::tlmt_time &time,
						void *private_data)
  {
    //Update the time local
    c0.update_time(time);
    m_vci_event.notify(sc_core::SC_ZERO_TIME);
    return m_return;
  }

  tmpl(tlmt_core::tlmt_return&)::vciCmdReceived(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
						const tlmt_core::tlmt_time &time,
						void *private_data)
  {
    std::list<soclib::common::Segment>::iterator seg;	
    size_t segIndex;

    for (segIndex=0,seg = m_target_segments.begin(); seg != m_target_segments.end(); ++segIndex, ++seg ) {
      soclib::common::Segment &s = *seg;
      if (!s.contains(pkt->address))
	continue;

      switch(pkt->cmd){
      case vci_param::CMD_READ:
	return vciCmdReceived_read(segIndex,s,pkt,time,private_data);
	break;
      case vci_param::CMD_WRITE:
	return vciCmdReceived_write(segIndex,s,pkt,time,private_data);
	break;
      default:
	break;
      }
      return m_return;
    }
    //Update the time local
    c0.update_time(time);
    //add the processing time
    c0.add_time(5);

    //send error message
    m_rsp.error  = true;
    m_rsp.nwords = pkt->nwords;
    m_rsp.srcid  = pkt->srcid;
    m_rsp.trdid  = pkt->trdid;
    m_rsp.pktid  = pkt->pktid;

#if RAM_DEBUG
    std::cout << "[MWMR Target " << m_destid << "] Address " << pkt->address << " does not match any segment " << std::endl;
    std::cout << "[MWMR Target " << m_destid << "] Send to source "<< pkt->srcid << " a error packet with time = "  << c0.time() << std::endl;
#endif

    p_vci_target.send(&m_rsp, c0.time());
    m_return.set_time(c0.time());
    return m_return;
  }

  tmpl(tlmt_core::tlmt_return&)::vciCmdReceived_read(size_t segIndex,
						     soclib::common::Segment &s,
						     soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
						     const tlmt_core::tlmt_time &time,
						     void *private_data)
  {
    int reg;

    //Update the time local
    c0.update_time(time);

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Target " << m_destid <<"] Receive from source " << pkt->srcid << " a read packet " << pkt->pktid << " with time = "  << c0.time() << std::endl;
#endif

    //add the processing time
    c0.add_time(5);
    
    for(unsigned int i=0; i<pkt->nwords;i++){
      reg = (int)((pkt->address + (i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes;
      if ( reg < MWMR_IOREG_MAX ) { // coprocessor IO register access
	//add the reading time (reading time equals to number of words, in this case 1)
	c0.add_time(1);
	p_status[reg]->send(&pkt->buf[i], c0.time());
      }
      else {                      // MWMR channel configuration access (or Reset)
	switch (reg) {
	case MWMR_CONFIG_FIFO_NO :
	  pkt->buf[i] = m_channel_index;
	  break;
	case MWMR_CONFIG_FIFO_WAY :
	  pkt->buf[i] = m_channel_read;
	  break;
	case MWMR_CONFIG_STATUS_ADDR :
	  if(m_channel_read)
	    pkt->buf[i] = m_read_channel[m_channel_index].status_address;
	  else
	    pkt->buf[i] = m_write_channel[m_channel_index].status_address;
	  break;
	case MWMR_CONFIG_DEPTH :
	  if(m_channel_read)
	    pkt->buf[i] = m_read_channel[m_channel_index].depth;
	  else
	    pkt->buf[i] = m_write_channel[m_channel_index].depth;
	  break;
	case MWMR_CONFIG_BUFFER_ADDR :
	  if(m_channel_read)
	    pkt->buf[i] = m_read_channel[m_channel_index].base_address;
	  else
	    pkt->buf[i] = m_write_channel[m_channel_index].base_address;
	  break;
	case MWMR_CONFIG_RUNNING :
	  if(m_channel_read)
	    pkt->buf[i] = m_read_channel[m_channel_index].running;
	  else
	    pkt->buf[i] = m_write_channel[m_channel_index].running;
	  break;
	}
      }
    }
    
    //add the reading time (reading time equals to number of words)
    c0.add_time(pkt->nwords);

    //send anwser
    m_rsp.error  = false;
    m_rsp.nwords = pkt->nwords;
    m_rsp.srcid  = pkt->srcid;
    m_rsp.trdid  = pkt->trdid;
    m_rsp.pktid  = pkt->pktid;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Target " << m_destid <<"] Send answer packet " << pkt->pktid << " with time = " << c0.time() << std::endl;
#endif

    p_vci_target.send(&m_rsp, c0.time()) ;
    m_return.set_time(c0.time());
    return m_return;
  }

  tmpl(tlmt_core::tlmt_return&)::vciCmdReceived_write(size_t segIndex,
						      soclib::common::Segment &s,
						      soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
						      const tlmt_core::tlmt_time &time,
						      void *private_data)
  {
    int reg;

    //Update the time local
    c0.update_time(time);

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Target " << m_destid <<"] Receive from source " << pkt->srcid << " a write packet " << pkt->pktid << " with time = "  << c0.time() << std::endl;
#endif

    //add the processing time
    c0.add_time(5);
    
    for(unsigned int i=0; i<pkt->nwords;i++){
      reg = (int)((pkt->address + (i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes;
      if ( reg < MWMR_IOREG_MAX ) { // coprocessor IO register access
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Target " << m_destid <<"] CONFIGURE ASSOCIATED COPROCESSOR" << std::endl;
#endif
	//add the writing time (writing time equals to number of words, in this case 1)
	c0.add_time(1);
	p_config[reg]->send(pkt->buf[i], c0.time());
      }
      else {                      // MWMR channel configuration access (or Reset)
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Target " << m_destid <<"] CONFIGURE MWMR" << std::endl;
#endif
	switch (reg) {
	case MWMR_RESET :
	  m_reset_request = true;
	  break;
	case MWMR_CONFIG_FIFO_NO :
	  m_channel_index = pkt->buf[i];
	  break;
	case MWMR_CONFIG_FIFO_WAY :
	  m_channel_read = ( pkt->buf[i] != 0 );
	  break;
	case MWMR_CONFIG_STATUS_ADDR :
	  if(m_channel_read)
	    m_read_channel[m_channel_index].status_address = pkt->buf[i];
	  else
	    m_write_channel[m_channel_index].status_address = pkt->buf[i];
	  break;
	case MWMR_CONFIG_DEPTH :
	  if(m_channel_read)
	    m_read_channel[m_channel_index].depth = pkt->buf[i];
	  else
	    m_write_channel[m_channel_index].depth = pkt->buf[i];
	  break;
	case MWMR_CONFIG_BUFFER_ADDR :
	  if(m_channel_read)
	    m_read_channel[m_channel_index].base_address = pkt->buf[i];
	    else
	    m_write_channel[m_channel_index].base_address = pkt->buf[i];
	  break;
	case MWMR_CONFIG_RUNNING :
	  if(m_channel_read)
	    m_read_channel[m_channel_index].running = ( pkt->buf[i] != 0 );
	  else
	    m_write_channel[m_channel_index].running = ( pkt->buf[i] != 0 );
	  m_fifo_event.notify();
	  break;
	} // end switch cell
      }
    }

    //add the writing time (writing time equals to number of words)
    c0.add_time(pkt->nwords);

    //send anwser
    m_rsp.error  = false;
    m_rsp.nwords = pkt->nwords;
    m_rsp.srcid  = pkt->srcid;
    m_rsp.trdid  = pkt->trdid;
    m_rsp.pktid  = pkt->pktid;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Target " << m_destid <<"] Send answer packet " << pkt->pktid << " with time = " << c0.time() << std::endl;
#endif

    p_vci_target.send(&m_rsp, c0.time());
    m_return.set_time(c0.time());
    return m_return;
  }

  /////////////////////////////////////////////////////////////////////////////  
  // CALLBACK FUNCTION FOR THE READ REQUEST
  /////////////////////////////////////////////////////////////////////////////  
  tmpl(tlmt_core::tlmt_return&)::readRequestReceived(soclib::tlmt::fifo_cmd_packet<vci_param> *req,
						     const tlmt_core::tlmt_time &time,
						     void *private_data)
  {
    int index = (int)private_data;

    //Update the time local
    c0.update_time(time);

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid <<"] Receive read request fifo = " << index << " with time = " << c0.time() << " full = " << m_read_fifo[index].full << std::endl ;
#endif

    //add the processing time and the reading time (reading time equals to number of words)
    c0.add_time(5 + req->nwords);

    assert (req->nwords == m_read_fifo_depth);
    if ( m_read_fifo[index].full) {  // fifo pleine
      m_read_fifo[index].full = false;

      for (uint32_t j = 0; j < m_read_fifo_depth; j++) 
	req->buf[j] = m_read_fifo[index].data[j];

      m_read_fifo[index].time = c0.time();
      m_fifo_event.notify();

      //send awnser
      p_read_fifo[index]->send(index,c0.time());
    } 
    else{
      m_read_request[index].pending = true;
      m_read_request[index].data = req->buf;
      m_read_request[index].time = c0.time();
    } 
    return m_return;
  }

  /////////////////////////////////////////////////////////////////////////////  
  // CALLBACK FUNCTION FOR THE WRITE REQUEST
  /////////////////////////////////////////////////////////////////////////////  
  tmpl(tlmt_core::tlmt_return&)::writeRequestReceived(soclib::tlmt::fifo_cmd_packet<vci_param> *req,
						      const tlmt_core::tlmt_time &time,
						      void *private_data)
  {
    int index = (int)private_data;

    //Update the time local
    c0.update_time(time);

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid <<"] Receive write request fifo = " << index << " with time = " << c0.time() << " full = " << m_write_fifo[index].full << std::endl ;
#endif

    //add the processing time and the writing time (writing time equals to number of words)
    c0.add_time(5 + req->nwords);

    assert (req->nwords == m_write_fifo_depth);
    if (!m_write_fifo[index].full) {  // fifo vide
      m_write_fifo[index].full = true;

      for(uint32_t j=0;j< req->nwords; j++)
	m_write_fifo[index].data[j] = req->buf[j];

      m_write_fifo[index].time = c0.time(); 
      m_fifo_event.notify();

      //send awnser
      p_write_fifo[index]->send(index,c0.time());
    }
    else{
      m_write_request[index].pending = true;
      for(uint32_t j=0;j< req->nwords; j++)
	m_write_request[index].data[j] = req->buf[j];
      m_write_request[index].time = c0.time();
    }

    return m_return;
  }

  /////////////////////////////////////////////////////////////////////////////  
  // CALLBACK FUNCTION FOR THE STATE COPROCESSOR
  /////////////////////////////////////////////////////////////////////////////  
  tmpl(tlmt_core::tlmt_return&)::setStateCoprocessor(bool state,
						     const tlmt_core::tlmt_time &time,
						     void *private_data)
  {
    //Update the time local
    c0.update_time(time);

    m_active_coprocessor = state;

#if MWMR_CONTROLLER_DEBUG
    if(state)
      std::cout << "[MWMR Initiator " << m_srcid <<"] ACTIVED COPROCESSOR time " << c0.time() << std::endl ;
    else
      std::cout << "[MWMR Initiator " << m_srcid <<"] DESACTIVED COPROCESSOR time " << c0.time() << std::endl ;
#endif

    m_active_coprocessor_event.notify();
    return m_return;
  }

  /////////////////////////////////////////////////////////////////////////////  
  // RESET THE MWMR
  /////////////////////////////////////////////////////////////////////////////  
  tmpl(void)::reset() 
  {
#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Target " << m_destid <<"] RESET" << std::endl ;
#endif

    for ( uint32_t  i = 0 ; i < m_read_channels ; i++ ) {
      m_read_fifo[i].full        = false ;
      m_read_request[i].pending  = false ;
      m_read_channel[i].running  = false ;
    }
    for ( uint32_t  i = 0 ; i < m_write_channels ; i++ ) {
      m_write_fifo[i].full       = false ;
      m_write_request[i].pending = false ;
      m_write_channel[i].running = false ;
    }

   
    //send the anwser to all read fifo 
    for ( uint32_t i = 0; i < m_read_channels; i++)
      p_read_fifo[i]->send(i,c0.time());
    //send the anwser to all write fifo
    for ( uint32_t i = 0; i < m_write_channels; i++)
      p_write_fifo[i]->send(i,c0.time());

    m_reset_request = false;
  }

  /////////////////////////////////////////////////////////////////////////////  
  // GET LOCK  EXECUTE THE LOCKED_READ AND STORE CONDITIONAL OPERATIONS 
  /////////////////////////////////////////////////////////////////////////////  
  tmpl(void)::getLock(typename vci_param::addr_t status_address, uint32_t *status) 
  {
    do{
#if MWMR_CONTROLLER_DEBUG
      std::cout << "[MWMR Initiator " << m_srcid << "] LOCKED READ" << std::endl;
#endif
      
      m_cmd.cmd     = vci_param::CMD_LOCKED_READ;
      m_cmd.nwords  = 1;
      m_cmd.address = status_address + 12;
      m_cmd.buf     = &status[3]; 
      m_cmd.srcid   = m_srcid; 
      m_cmd.trdid   = 0;
      m_cmd.pktid   = m_pktid;
      m_cmd.be      = 0xF;
      m_cmd.contig  = true;
      
      do{
	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
#endif
	m_pktid++;
      }while(status[3]!=0);
      
#if MWMR_CONTROLLER_DEBUG
      std::cout << "[MWMR Initiator " << m_srcid << "] STORE CONDITIONEL" << std::endl;
#endif
      
      status[3]   = 1;
      m_cmd.cmd     = vci_param::CMD_STORE_COND;
      m_cmd.nwords  = 1;
      m_cmd.address = status_address + 12;
      m_cmd.buf     = &status[3]; 
      m_cmd.srcid   = m_srcid; 
      m_cmd.trdid   = 0;
      m_cmd.pktid   = m_pktid;
      m_cmd.be      = 0xF;
      m_cmd.contig  = true;
      
#if MWMR_CONTROLLER_DEBUG
      std::cout << "[MWMR Initiator " << m_srcid << "] Send store conditional packet with time = " << c0.time() << std::endl;
#endif

      p_vci_initiator.send(&m_cmd, c0.time());
      wait(m_vci_event);

#if MWMR_CONTROLLER_DEBUG
      std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser store conditional packet with time = " << c0.time() << std::endl;
#endif
      m_pktid++;
      
    }while(status[3]==0);
  }

  tmpl(void)::releaseLock(typename vci_param::addr_t status_address, uint32_t *status) 
  {
#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] RELEASE THE LOCK" << std::endl;
#endif
    status[3]   = 0; //release the lock
    m_cmd.cmd     = vci_param::CMD_WRITE;
    m_cmd.nwords  = 1;
    m_cmd.address = status_address + 12;
    m_cmd.buf     = &status[3]; 
    m_cmd.be      = 0xF;
    m_cmd.srcid   = m_srcid; 
    m_cmd.trdid   = 0;
    m_cmd.pktid   = m_pktid;
    m_cmd.contig  = true;

#if MWMR_CONTROLLER_DEBUG
    std::cout << std::dec << "[MWMR Initiator " << m_srcid << "] Send read packet with time = " << c0.time() << std::endl;
#endif
    p_vci_initiator.send(&m_cmd, c0.time());
    wait(m_vci_event);
    m_pktid++;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
#endif

  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // READ STATUS OF A DETERMINED CHANNEL
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::readStatus(typename vci_param::addr_t status_address, uint32_t *status) 
  {
    // STATUS[0] = index_read;
    // STATUS[1] = index_write;
    // STATUS[2] = content (capacity)
    // STATUS[3] = lock

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] READ STATUS" << std::endl;
#endif

    m_cmd.cmd     = vci_param::CMD_READ;
    m_cmd.nwords  = 3;
    m_cmd.address = status_address;
    m_cmd.buf     = status; 
    m_cmd.be      = 0xF;
    m_cmd.srcid   = m_srcid; 
    m_cmd.trdid   = 0;
    m_cmd.pktid   = m_pktid;
    m_cmd.contig  = true;
    
#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] Send read packet with time = " << c0.time() << std::endl;
#endif

    p_vci_initiator.send(&m_cmd, c0.time());
    wait(m_vci_event);
    m_pktid++;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
    std::cout << std::hex;
    for(unsigned int j=0; j<m_cmd.nwords; j++)
      std::cout << "[" << (m_cmd.address+(j*vci_param::nbytes)) << "] = " << status[j] << std::endl;
    std::cout << std::dec;
#endif
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // UPDATE STATUS OF A DETERMINED CHANNEL
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::updateStatus(typename vci_param::addr_t status_address, uint32_t *status) 
  {
#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] UPDATE STATUS" << std::endl;
#endif
    status[3]   = 0; // release the lock 
    m_cmd.cmd     = vci_param::CMD_WRITE;
    m_cmd.nwords  = 4;
    m_cmd.address = status_address;
    m_cmd.buf     = status; 
    m_cmd.be      = 0xF;
    m_cmd.srcid   = m_srcid; 
    m_cmd.trdid   = 0;
    m_cmd.pktid   = m_pktid;
    m_cmd.contig  = true;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] Send read packet with time = " << c0.time() << std::endl;
#endif

    p_vci_initiator.send(&m_cmd, c0.time());
    wait(m_vci_event);
    m_pktid++;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
#endif

  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // READ DATA FROM A DETERMINED CHANNEL TO A FIFO
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::readFromChannel(uint32_t fifo_index, uint32_t *status) 
  {
    typename vci_param::addr_t limit_address;
    uint32_t                   busy_positions;

    // STATUS[0] = index_read;
    // STATUS[1] = index_write;
    // STATUS[2] = content (number of elements in the channel)
    // STATUS[3] = lock

    busy_positions = status[2];
    limit_address  = m_read_channel[fifo_index].base_address + m_read_channel[fifo_index].depth;
#if MWMR_CONTROLLER_DEBUG
    std::cout << "[ BUSY_POSITIONS = " << busy_positions << std::hex << " BASE ADDRESS = " <<  m_read_channel[fifo_index].base_address  << " LIMIT ADDRESS = " << limit_address << " ]" << std::dec << std::endl;
#endif
    
    ///////// read transfer OK //////////
    if(busy_positions>= m_read_fifo_depth){  
      if(status[0]+(m_read_fifo_depth*vci_param::nbytes)<=limit_address){ // send only 1 message

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] READ FIFO " << fifo_index << " SEND MESSAGE 1 OF 1" << std::endl;
#endif

	m_cmd.cmd     = vci_param::CMD_READ;
	m_cmd.nwords  = m_read_fifo_depth;
	m_cmd.address = status[0];
	m_cmd.buf     = m_read_fifo[fifo_index].data; 
	m_cmd.srcid   = m_srcid; 
	m_cmd.trdid   = 0;
	m_cmd.pktid   = m_pktid;
	m_cmd.contig  = true; //address contiguous

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Send read packet with time = " << c0.time() << std::endl;
#endif

	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
	m_pktid++;

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
	std::cout << std::hex;
	for(unsigned int j=0; j<m_cmd.nwords; j++)
	  std::cout << "[" << (m_cmd.address+(j*vci_param::nbytes)) << "] = " << m_cmd.buf[j] << std::endl;
	std::cout << std::dec;
#endif
      }
      else{ // send 2 message
	typename vci_param::data_t data_1[m_read_fifo_depth], data_2[m_read_fifo_depth];
	typename vci_param::addr_t address;
	uint32_t nwords_1, nwords_2;
	uint32_t count = 0;
	for(nwords_1 = 0, address = status[0]; address < limit_address; nwords_1++, count++, address+=vci_param::nbytes );
	for(nwords_2 = 0; count <m_read_fifo_depth; nwords_2++,count++);

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] READ FIFO " << fifo_index << " SEND MESSAGE 1 OF 2" << std::endl;
#endif

	m_cmd.cmd     = vci_param::CMD_READ;
	m_cmd.nwords  = nwords_1;
	m_cmd.address = status[0];
	m_cmd.buf     = data_1; 
	m_cmd.be      = 0xF;
	m_cmd.srcid   = m_srcid; 
	m_cmd.trdid   = 0;
	m_cmd.pktid   = m_pktid;
	m_cmd.contig  = true; 

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Send read packet with time = " << c0.time() << std::endl;
#endif
	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
	m_pktid++;

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
	std::cout << std::hex;
	for(unsigned int j=0; j<m_cmd.nwords; j++)
	  std::cout << "[" << (m_cmd.address+(j*vci_param::nbytes)) << "] = " << m_cmd.buf[j] << std::endl;
	std::cout << std::dec;
#endif

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] READ FIFO " << fifo_index << " SEND MESSAGE 2 OF 2" << std::endl;
#endif
	m_cmd.cmd     = vci_param::CMD_READ;
	m_cmd.nwords  = nwords_2;
	m_cmd.address = m_read_channel[fifo_index].base_address;
	m_cmd.buf     = data_2; 
	m_cmd.srcid   = m_srcid; 
	m_cmd.trdid   = 0;
	m_cmd.pktid   = m_pktid;
	m_cmd.be      = 0xF;
	m_cmd.contig  = true; //address contiguous

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Send read packet with time = " << c0.time() << std::endl;
#endif
	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
	m_pktid++;

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser read packet with time = " << c0.time() << std::endl;
	std::cout << std::hex;
	for(unsigned int j=0; j<m_cmd.nwords; j++)
	  std::cout << "[" << (m_cmd.address+(j*vci_param::nbytes)) << "] = " << m_cmd.buf[j] << std::endl;
	std::cout << std::dec;
#endif

	count = 0;
	for(uint32_t j=0; count<nwords_1; count++, j++)
	  m_read_fifo[fifo_index].data[count] = data_1[j];
	for(uint32_t j=0; count<m_read_fifo_depth; count++, j++)
	  m_read_fifo[fifo_index].data[count] = data_2[j];
      }
      // update the read pointer
      status[0] = status[0] + (m_read_fifo_depth * vci_param::nbytes);
      if(status[0] >= limit_address)
	status[0] = status[0] - limit_address + m_read_channel[fifo_index].base_address;

      // update the number of elements in the channel
      status[2] -= m_read_fifo_depth;

      // update the status descriptor
      updateStatus(m_read_channel[fifo_index].status_address, status);

      // update the fifo state
      m_read_fifo[fifo_index].full = true;
      m_read_fifo[fifo_index].time = c0.time();
    }
    else{
#if MWMR_CONTROLLER_DEBUG
      std::cout << "[MWMR Initiator " << m_srcid << "] READ FIFO " << fifo_index << " NOT OK" << std::endl;
#endif
      ///////////// release lock ///////////////////////
      releaseLock(m_read_channel[fifo_index].status_address, status);

      ///////////// update the time ///////////////////
      m_read_fifo[fifo_index].time += m_waiting_time ;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // WRITE DATA FROM A FIFO TO A DETERMINED CHANNEL
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::writeToChannel(uint32_t fifo_index, uint32_t *status) 
  {
    typename vci_param::addr_t limit_address;
    uint32_t                   busy_positions;
    uint32_t                   free_positions;

    // STATUS[0] = index_read;
    // STATUS[1] = index_write;
    // STATUS[2] = content (number of elements in the channel)
    // STATUS[3] = lock
    
    busy_positions = status[2];

    //divide the channel depth by number of byte of a word
    free_positions = (m_write_channel[fifo_index].depth/4) - busy_positions;

    //free_positions = (m_write_channel[fifo_index].depth<<2) - busy_positions;
    limit_address = m_write_channel[fifo_index].base_address + m_write_channel[fifo_index].depth;

#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] BUSY_POSITIONS = " << busy_positions << " FREE_POSITIONS = " << free_positions << std::hex << " BASE ADDRESS = " <<  m_write_channel[fifo_index].base_address  << " LIMIT ADDRESS = " << limit_address << std::dec << std::endl;
#endif
    
    ///////// write transfer OK //////////
    if(free_positions >= m_write_fifo_depth){  
      if(status[1]+(m_write_fifo_depth*vci_param::nbytes)<=limit_address){ // send only 1 message

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] WRITE FIFO " << fifo_index << " SEND MESSAGE 1 OF 1" << std::endl;
#endif

	m_cmd.cmd     = vci_param::CMD_WRITE;
	m_cmd.nwords  = m_write_fifo_depth;
	m_cmd.address = status[1];
	m_cmd.buf     = m_write_fifo[fifo_index].data; 
	m_cmd.srcid   = m_srcid; 
	m_cmd.trdid   = 0;
	m_cmd.pktid   = m_pktid;
	m_cmd.be      = 0xF;
	m_cmd.contig  = true; //address contiguous
	
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Send write packet with time = " << c0.time() << std::endl;
#endif

	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
	m_pktid++;

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser write packet with time = " << c0.time() << std::endl;
#endif

      }
      else{ // send 2 message

	typename vci_param::addr_t address;
	typename vci_param::data_t data_1[m_write_fifo_depth], data_2[m_write_fifo_depth];
	uint32_t nwords_1, nwords_2;
	uint32_t count = 0;
	for(nwords_1 = 0, address = status[1]; address < limit_address; nwords_1++, count++, address+=vci_param::nbytes)
	  data_1[nwords_1] = m_write_fifo[fifo_index].data[count];
	for(nwords_2=0; count <m_write_fifo_depth; nwords_2++,count++)
	  data_2[nwords_2] = m_write_fifo[fifo_index].data[count];

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] WRITE FIFO " << fifo_index << " SEND MESSAGE 1 OF 2" << std::endl;
#endif

	m_cmd.cmd     = vci_param::CMD_WRITE;
	m_cmd.nwords  = nwords_1;
	m_cmd.address = status[1];
	m_cmd.buf     = data_1; 
	m_cmd.srcid   = m_srcid; 
	m_cmd.trdid   = 0;
	m_cmd.pktid   = m_pktid;
	m_cmd.be      = 0xF;
	m_cmd.contig  = true;
	
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Send write packet with time = " << c0.time() << std::endl;
#endif
	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
	m_pktid++;

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser write packet with time = " << c0.time() << std::endl;
#endif


#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] WRITE FIFO " << fifo_index << " SEND 2 MESSAGE OF 2" << std::endl;
#endif
	m_cmd.cmd     = vci_param::CMD_WRITE;
	m_cmd.nwords  = nwords_2;
	m_cmd.address = m_write_channel[fifo_index].base_address;
	m_cmd.buf     = data_2; 
	m_cmd.be      = 0xF;
	m_cmd.srcid   = m_srcid; 
	m_cmd.trdid   = 0;
	m_cmd.pktid   = m_pktid;
	m_cmd.contig  = true; //address contiguous
	
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Send write packet with time = " << c0.time() << std::endl;
#endif
	p_vci_initiator.send(&m_cmd, c0.time());
	wait(m_vci_event);
	m_pktid++;

#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] Receive awnser write packet with time = " << c0.time() << std::endl;
#endif
      }
      // update the write pointer
      status[1] = status[1] +  (m_write_fifo_depth * vci_param::nbytes);
      if(status[1] >= limit_address)
	status[1] = status[1] - limit_address + m_write_channel[fifo_index].base_address;

      // update the number of elements in the channel
      status[2] += m_write_fifo_depth;

      // update the status descriptor
      updateStatus(m_write_channel[fifo_index].status_address, status);

      // update the fifo state
      m_write_fifo[fifo_index].full = false;
      m_write_fifo[fifo_index].time = c0.time();
    }
    else{
#if MWMR_CONTROLLER_DEBUG
      std::cout << "[MWMR Initiator " << m_srcid << "] WRITE FIFO " << fifo_index << " NOT OK" << std::endl;
#endif
      ///////////// release lock ///////////////////////
      releaseLock(m_write_channel[fifo_index].status_address, status);

      ///////////// update the time ///////////////////
      m_write_fifo[fifo_index].time += m_waiting_time ;
    } 
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // RELEASE THE PENDING FIFOS
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::releasePendingFifos()
  {
#if MWMR_CONTROLLER_DEBUG
    std::cout << "[MWMR Initiator " << m_srcid << "] RELEASE PENDING FIFOS " << std::endl;
#endif
  
    // handling all pending coprocessor requests
    for ( uint32_t i = 0; i < m_read_channels; i++) {
      if ( m_read_request[i].pending && m_read_fifo[i].full){
	//copie 
	for ( uint32_t n = 0 ; n < m_read_fifo_depth ; n++ )
	  m_read_request[i].data[n] = m_read_fifo[i].data[n];

	m_read_request[i].pending = false ;
	m_read_fifo[i].full = false ;

	//send awnser
	p_read_fifo[i]->send(i,c0.time()); //DEVE RETORNAR O DATA
      } 
    } // end for read channels
    for ( uint32_t i = 0; i < m_write_channels; i++) {
      if ( m_write_request[i].pending && !m_write_fifo[i].full) {
	for ( uint32_t n = 0 ; n < m_write_fifo_depth ; n++ )
	  m_write_fifo[i].data[n] = m_write_request[i].data[n];

	m_write_request[i].pending = false;
	m_write_fifo[i].full = true ;

	//send awnser
	p_write_fifo[i]->send(i,c0.time());
      } 
    } // end for write channels
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // EXECUTE THE LOOP
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::execLoop() 
  {
    bool                       fifo_serviceable;
    tlmt_core::tlmt_time       fifo_time;
    uint32_t                   fifo_index;
    bool                       fifo_read;
    uint32_t                   status[4];
    typename vci_param::addr_t status_address;

    int count = 0;
    //while(count < 15) {
    while(true) {
      count++;

      if(!m_active_coprocessor){
	c0.disable();
	wait(m_active_coprocessor_event);
	c0.enable();
      }
      if(m_reset_request)
	reset();

      //// select the first serviceable FIFO
      //// taking the request time into account 
      //// write FIFOs have the highest priority
      fifo_serviceable = false ; 
      fifo_time = std::numeric_limits<uint32_t>::max();
    
      for (uint32_t  i = 0; i < m_read_channels; i++) {
	if ( !m_read_fifo[i].full && m_read_channel[i].running){
	  if (fifo_time >= m_read_fifo[i].time) {
	    fifo_serviceable = true;
	    fifo_time = m_read_fifo[i].time;
	    fifo_index = i;
	    fifo_read  = true;
	  } // end if date
	} // end if valid
      } // end for read fifo

      for (uint32_t  i = 0; i < m_write_channels; i++) {
	if ( m_write_fifo[i].full && m_write_channel[i].running) {
	  if (fifo_time >= m_write_fifo[i].time) {
	    fifo_serviceable = true;
	    fifo_time = m_write_fifo[i].time;
	    fifo_index = i;
	    fifo_read  = false;
	  } // end if date
	} // end if valid
      } // end for write fifo
               
      if ( !fifo_serviceable ){
#if MWMR_CONTROLLER_DEBUG
	std::cout << "[MWMR Initiator " << m_srcid << "] FIFO NO SERVICEABLE"<< std::endl;
#endif
	wait(m_fifo_event);
      }
      else{
	count++;
	//Update Time
	c0.update_time(fifo_time);

	// get the status address
	if (fifo_read) {
#if MWMR_CONTROLLER_DEBUG
	  std::cout << "[MWMR Initiator " << m_srcid << "] READ FIFO SELECTED " << fifo_index << std::endl;
#endif
	  status_address = m_read_channel[fifo_index].status_address;
	} 
	else {    /////////////////////////////////////////////////////
#if MWMR_CONTROLLER_DEBUG
	  std::cout << "[MWMR Initiator " << m_srcid << "] WRITE FIFO SELECTED " << fifo_index << std::endl;
#endif
	  status_address = m_write_channel[fifo_index].status_address;
	}

	//// get the lock ////
	getLock(status_address,status);
      
	//// read the status ////
	readStatus(status_address,status);
    
	if ( fifo_read){
	  ////// read from channel //////
	  readFromChannel(fifo_index,status);
	}
	else{
	  ////// write to channel //////
	  writeToChannel(fifo_index,status);
	}
	
	//// release pending fifos ////
	releasePendingFifos();
      }  
    }
  } // end loopExec        

  tmpl(/**/)::VciMwmrController(sc_core::sc_module_name name,
				uint32_t src,
				const soclib::common::IntTab &initiator_index,
				uint32_t dest,
				const soclib::common::IntTab &target_index,
				const soclib::common::MappingTable &mt,
				uint32_t read_fifo_depth,  //in words
				uint32_t write_fifo_depth, //in words
				uint32_t n_read_channels,
				uint32_t n_write_channels,
				uint32_t n_config,
				uint32_t n_status)
    : soclib::tlmt::BaseModule(name),
      m_initiator_index(initiator_index),
      m_target_index(target_index),
      m_mt(mt),
      m_read_fifo_depth(read_fifo_depth),
      m_write_fifo_depth(write_fifo_depth),
      m_read_channels(n_read_channels),
      m_write_channels(n_write_channels),
      m_config_registers(n_config),
      m_status_registers(n_status),
      p_state("state", new tlmt_core::tlmt_callback<VciMwmrController,bool>(this, &VciMwmrController<vci_param>::setStateCoprocessor)),
      p_vci_initiator("vci_initiator", new tlmt_core::tlmt_callback<VciMwmrController,soclib::tlmt::vci_rsp_packet<vci_param> *>(this, &VciMwmrController<vci_param>::vciRspReceived),&c0),
      p_vci_target("vci_target", new tlmt_core::tlmt_callback<VciMwmrController,soclib::tlmt::vci_cmd_packet<vci_param> *>(this, &VciMwmrController<vci_param>::vciCmdReceived))
  {
    m_initiator_segments=m_mt.getSegmentList(m_initiator_index);
    m_target_segments=m_mt.getSegmentList(m_target_index);

    m_waiting_time = 1000;
    m_channel_index = 0;
    m_channel_read = false;
    m_reset_request = false;
    m_active_coprocessor = false;
    m_srcid = src;
    m_destid = dest;
    m_pktid = 1;

    m_read_channel = new channel_struct<vci_param>[m_read_channels];
    m_read_request = new request_struct<vci_param>[m_read_channels];
    m_read_fifo    = new fifos_struct<vci_param>[m_read_channels];
    for(uint32_t i=0;i<m_read_channels;i++){
      //Channel
      m_read_channel[i].running  = false ;
      //Requests
      m_read_request[i].data     = new typename vci_param::data_t[m_read_fifo_depth];
      m_read_request[i].pending  = false ;
      //Fifos
      m_read_fifo[i].data        = new typename vci_param::data_t[m_read_fifo_depth];
      m_read_fifo[i].full        = false ;

      std::ostringstream tmpName;
      tmpName << "read_fifo" << i;
      p_read_fifo.push_back(new soclib::tlmt::FifoTarget<vci_param>(tmpName.str().c_str(), new tlmt_core::tlmt_callback<VciMwmrController,soclib::tlmt::fifo_cmd_packet<vci_param> *>(this,&VciMwmrController<vci_param>::readRequestReceived,(int*)i)));
    }

    m_write_channel = new channel_struct<vci_param>[m_write_channels];
    m_write_request = new request_struct<vci_param>[m_write_channels];
    m_write_fifo    = new fifos_struct<vci_param>[m_write_channels];
    for(uint32_t i=0;i<m_write_channels;i++){
      //Channel
      m_write_channel[i].running = false ;
      //Request
      m_write_request[i].data     = new typename vci_param::data_t[m_write_fifo_depth];
      m_write_request[i].pending = false ;
      //Fifos
      m_write_fifo[i].data        = new typename vci_param::data_t[m_write_fifo_depth];
      m_write_fifo[i].full       = false ;

      std::ostringstream tmpName;
      tmpName << "write_fifo" << i;
      p_write_fifo.push_back(new soclib::tlmt::FifoTarget<vci_param>(tmpName.str().c_str(), new tlmt_core::tlmt_callback<VciMwmrController,soclib::tlmt::fifo_cmd_packet<vci_param> *>(this,&VciMwmrController<vci_param>::writeRequestReceived,(int*)i)));
    }

    //CONFIG PORTS
    for(uint32_t i=0;i<m_config_registers;i++){
      std::ostringstream tmpName;
      tmpName << "config" << i;
      p_config.push_back(new tlmt_core::tlmt_out<typename vci_param::data_t>(tmpName.str().c_str(),NULL));
    }

    //STATUS PORTS
    for(uint32_t i=0;i<m_status_registers;i++){
      std::ostringstream tmpName;
      tmpName << "status" << i;
      p_status.push_back(new tlmt_core::tlmt_out<typename vci_param::data_t*>(tmpName.str().c_str(),NULL));
    }

    SC_THREAD(execLoop);

  }

}}

