#include "vci_param.h"
#include "../include/vci_ram.h"

#ifndef RAM_DEBUG
#define RAM_DEBUG 0
#endif

namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciRam<vci_param>

  tmpl(tlmt_core::tlmt_return&)::callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					  const tlmt_core::tlmt_time &time,
					  void *private_data)
  {
    // First, find the right segment using the first address of the packet
    std::list<soclib::common::Segment>::iterator seg;	
    size_t segIndex;

    for (segIndex=0,seg = m_segments.begin(); seg != m_segments.end(); ++segIndex, ++seg ) {
      soclib::common::Segment &s = *seg;
      if (!s.contains(pkt->address))
	continue;
      
      switch(pkt->cmd){
      case vci_param::CMD_READ :
	return callback_read(segIndex,s,pkt,time,private_data);
	break;
      case vci_param::CMD_WRITE :
	return callback_write(segIndex,s,pkt,time,private_data);
	break;
      case vci_param::CMD_LOCKED_READ :
	return callback_locked_read(segIndex,s,pkt,time,private_data);
	break;
      case vci_param::CMD_STORE_COND :
	return callback_store_cond(segIndex,s,pkt,time,private_data);
	break;
      default:
	break;
      }
      return m_return;
    }
    //send error message
    tlmt_core::tlmt_time delay = 5; 

    rsp.error  = true;
    rsp.nwords = pkt->nwords;
    rsp.srcid  = pkt->srcid;
    rsp.pktid  = pkt->pktid;
    rsp.trdid  = pkt->trdid;

#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Address " << pkt->address << " does not match any segment " << std::endl;
    std::cout << "[RAM " << m_id << "] Send to source "<< pkt->srcid << " a error packet with time = "  << time + delay << std::endl;
#endif
    p_vci.send(&rsp, time + delay);
    m_return.set_time(time + delay);

    return m_return;
  }

  tmpl(tlmt_core::tlmt_return&)::callback_read(size_t segIndex,
					       soclib::common::Segment &s,
					       soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					       const tlmt_core::tlmt_time &time,
					       void *private_data)
  {
#if RAM_DEBUG
    std::cout << "[RAM " << m_id <<"] Receive from source " << pkt->srcid <<" a Read packet " << pkt->pktid << " with time = "  << time << std::endl;
#endif

    if (pkt->contig) {
      for (size_t i=0;i<pkt->nwords;i++){
	pkt->buf[i]= m_contents[segIndex][((pkt->address+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes];
      }
    }
    else{
      for (size_t i=0;i<pkt->nwords;i++){
	pkt->buf[i]= m_contents[segIndex][(pkt->address - s.baseAddress()) / vci_param::nbytes]; // always write in the same address
      }
    }

    tlmt_core::tlmt_time delay = tlmt_core::tlmt_time(pkt->nwords + 5); 
    rsp.error  = false;
    rsp.nwords = pkt->nwords;
    rsp.srcid  = pkt->srcid;
    rsp.pktid  = pkt->pktid;
    rsp.trdid  = pkt->trdid;

#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Send to source "<< pkt->srcid << " a anwser packet " << pkt->pktid << " with time = "  << time + delay << std::endl;
#endif

    p_vci.send(&rsp, time + delay);
    m_return.set_time(time+ delay);
    return m_return;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // CALLBACK FUNCTION TO LOCKED READ COMMAND
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(tlmt_core::tlmt_return&)::callback_locked_read(size_t segIndex,
						      soclib::common::Segment &s,
						      soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
						      const tlmt_core::tlmt_time &time,
						      void *private_data)
  {
#if RAM_DEBUG
    std::cout << "[RAM " << m_id <<"] Receive from source " << pkt->srcid <<" a Locked Read packet " << pkt->pktid << " with time = "  << time << std::endl;
#endif

    if (pkt->contig) {
      for (size_t i=0; i<pkt->nwords; i++){
	pkt->buf[i]= m_contents[segIndex][((pkt->address+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes];
	//m_atomic.doLoadLinked(pkt->address + (i*vci_param::nbytes), pkt->srcid);
	insertLineTable(pkt->address + (i*vci_param::nbytes), pkt->srcid);
#if RAM_DEBUG
	showTable();
#endif
      }
    }
    else{
      for (size_t i=0; i<pkt->nwords; i++){
	pkt->buf[i]= m_contents[segIndex][(pkt->address - s.baseAddress()) / vci_param::nbytes]; // always write in the same address
	//m_atomic.doLoadLinked(pkt->address, pkt->srcid);
	insertLineTable(pkt->address, pkt->srcid);
#if RAM_DEBUG
	showTable();
#endif
      }
    }

    tlmt_core::tlmt_time delay = tlmt_core::tlmt_time(pkt->nwords + 5); 
    rsp.error  = false;
    rsp.nwords = pkt->nwords;
    rsp.srcid  = pkt->srcid;
    rsp.pktid  = pkt->pktid;
    rsp.trdid  = pkt->trdid;

#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Send to source "<< pkt->srcid << " a anwser packet " << pkt->pktid << " with time = "  << time + delay << std::endl;
#endif

    p_vci.send(&rsp, time + delay);
    m_return.set_time(time+ delay);
    return m_return;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // CALLBACK FUNCTION TO WRITE COMMAND
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(tlmt_core::tlmt_return&)::callback_write(size_t segIndex,
						soclib::common::Segment &s,
						soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
						const tlmt_core::tlmt_time &time,
						void *private_data)
  {
#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Receive from source " << pkt->srcid <<" a Write packet "<< pkt->pktid << " with time = "  << time << std::endl;
    for(uint32_t j=0;j< pkt->nwords; j++){
      if(pkt->contig)
	std::cout << std::hex << "[" << (pkt->address + (j*vci_param::nbytes)) << "] = " << pkt->buf[j] << std::dec << std::endl;
      else
	std::cout << std::hex << "[" << pkt->address << "] = " << pkt->buf[j] << std::dec << std::endl;
    }     
#endif

    uint32_t index;
    for (size_t i=0; i<pkt->nwords; i++){
      if(pkt->contig)
	index = ((pkt->address+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes;
      else
	index = (pkt->address - s.baseAddress()) / vci_param::nbytes;

      ram_t *tab = m_contents[segIndex];
      unsigned int cur = tab[index];
      uint32_t mask = 0;
      unsigned int be=pkt->be;

      if ( be & 1 )
	mask |= 0x000000ff;
      if ( be & 2 )
	mask |= 0x0000ff00;
      if ( be & 4 )
	mask |= 0x00ff0000;
      if ( be & 8 )
	mask |= 0xff000000;
      
      tab[index] = (cur & ~mask) | (pkt->buf[i] & mask);
    }

    tlmt_core::tlmt_time delay = tlmt_core::tlmt_time(pkt->nwords + 5); 
    rsp.error  = false;
    rsp.nwords = pkt->nwords;
    rsp.srcid  = pkt->srcid;
    rsp.pktid  = pkt->pktid;
    rsp.trdid  = pkt->trdid;
    
#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Send to source "<< pkt->srcid << " a anwser packet " << pkt->pktid << " with time = "  << time + delay  << std::endl;
#endif

    p_vci.send(&rsp,  time + delay);
    return m_return;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // CALLBACK FUNCTION TO STORE CONDITIONNEL COMMAND
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(tlmt_core::tlmt_return&)::callback_store_cond(size_t segIndex,
						soclib::common::Segment &s,
						soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
						const tlmt_core::tlmt_time &time,
						void *private_data)
  {
    typename vci_param::addr_t address;
    
    tlmt_core::tlmt_time delay = tlmt_core::tlmt_time(pkt->nwords + 5); 

#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Receive from source " << pkt->srcid <<" a Store Conditionnel packet "<< pkt->pktid << " with time = "  << time << std::endl;
    for(uint32_t j=0;j< pkt->nwords; j++){
      if(pkt->contig)
	std::cout << std::hex << "[" << (pkt->address + (j*vci_param::nbytes)) << "] = " << pkt->buf[j] << std::dec << std::endl;
      else
	std::cout << std::hex << "[" << pkt->address << "] = " << pkt->buf[j] << std::dec << std::endl;
    }         
#endif

    for (size_t i=0; i<pkt->nwords; i++){
      if(pkt->contig)
	address = (pkt->address + (i*vci_param::nbytes));
      else
	address = pkt->address;
	
      //if(m_atomic.isAtomic(address,pkt->srcid)){
      //m_atomic.accessDone(address);
      
      if(isOwner(address,pkt->srcid)){
	removeLineTable(address);
#if RAM_DEBUG
	std::cout << "[RAM " << m_id << "] STORE CONDITIONNEL OK" << std::endl;
	showTable();
#endif
	
	uint32_t index = (address - s.baseAddress()) / vci_param::nbytes;
	ram_t *tab = m_contents[segIndex];
	unsigned int cur = tab[index];
	uint32_t mask = 0;
	unsigned int be=pkt->be;
	
	if ( be & 1 )
	  mask |= 0x000000ff;
	if ( be & 2 )
	  mask |= 0x0000ff00;
	if ( be & 4 )
	  mask |= 0x00ff0000;
	if ( be & 8 )
	  mask |= 0xff000000;
      
	tab[index] = (cur & ~mask) | (pkt->buf[i] & mask);
	pkt->buf[i] = 1;
      }
      else{
	pkt->buf[i] = 0;

#if RAM_DEBUG
	std::cout << "[RAM " << m_id << "] STORE CONDITIONNEL NOT OK" << std::endl;
#endif
      }
    }

    rsp.error  = false;
    rsp.nwords = pkt->nwords;
    rsp.srcid  = pkt->srcid;
    rsp.pktid  = pkt->pktid;
    rsp.trdid  = pkt->trdid;
    
#if RAM_DEBUG
    std::cout << "[RAM " << m_id << "] Send to source "<< pkt->srcid << " a anwser packet " << pkt->pktid << " with time = "  << time + delay  << std::endl;
#endif

    p_vci.send(&rsp,  time + delay);
    return m_return;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // IF ADDRESS DOES NOT EXIST IN THE TABLE THEN INSERT ONE LINE FOR ADDRESS
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::insertLineTable(typename vci_param::addr_t address, uint32_t srcid){

    if(!existsAddress(address)){
#if RAM_DEBUG
     std::cout << std::hex << "INSERT IN THE TABLE THE ADDRESS " << address << std::dec << std::endl;
#endif
      //create a new line
      line_table<vci_param>* line;
      line = (line_table<vci_param>*) malloc(sizeof(line_table<vci_param>));

      line->address = address;
      line->srcid = srcid;
      line->previous = NULL;
      line->next = NULL;
      
      if(tableLL_begin==NULL){ // empty table
	tableLL_begin = line;
	tableLL_end = line;
      }
      else{ //insert in the end of table
	tableLL_end->next = line;
	line->previous = tableLL_end;
	tableLL_end = line;
      }
    }
    else{
#if RAM_DEBUG
      std::cout << std::hex << "THE ADDRESS " << address << " ALREADY EXISTS " <<std::dec << std::endl;
#endif
    }
  }


  //////////////////////////////////////////////////////////////////////////////////////////
  // IF ADDRESS EXIST IN THE TABLE THEN REMOVE 
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::removeLineTable(typename vci_param::addr_t address){

    line_table<vci_param> *line = tableLL_begin;
    bool found = false;
    while(line!=NULL && !found){
      if(line->address == address){
	if(tableLL_begin==line && tableLL_end==line){   // only line in the table
	  tableLL_begin = NULL;
	  tableLL_end = NULL;
	}
	else if(tableLL_begin==line){ // first line in the table
	  tableLL_begin = line->next;
	  tableLL_begin->previous = NULL;
	}
	else if(tableLL_end==line){    // last line in the table
	  tableLL_end = line->previous;
	  tableLL_end->next = NULL;
	}
	else{ // intermediary line
	  line->previous->next = line->next;
	  line->next->previous = line->previous;
	}
	free(line);
	found = true;
      }
      line = line->next;
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // IF THE ADDRESS EXISTS IN THE TABLE RETURN TRUE OTHERWISE RETURN FALSE
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(bool)::existsAddress(typename vci_param::addr_t address){
     line_table<vci_param> *line = tableLL_begin;
     while(line!=NULL){
       if(line->address == address)
	 return true;
       line = line->next;
     }
     return false;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // IF INFORMED ID IS EQUALS TO THE ID IN THE TABLE THEN RETURN TRUE OTHERWISE RETURN FALSE
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(bool)::isOwner(typename vci_param::addr_t address, uint32_t srcid){
     line_table<vci_param> *line = tableLL_begin;
     while(line!=NULL){
       if(line->address == address){
	 if(line->srcid == srcid)
	   return true;
	 else
	   return false;
       }
       line = line->next;
     }
     return false;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // SHOW ALL LINES OF TABLE LL
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(void)::showTable(){
     if(tableLL_begin!= NULL){
       std::cout << "SHOW TABLE" << std::endl;
       line_table<vci_param> *line = tableLL_begin;
       while(line!=NULL){
	 std::cout << std::hex << "ADDRESS = " << line->address << std::dec << " SCRID = " << line->srcid << std::endl;
	 line = line->next;
       }
     }
     else
       std::cout <<  "EMPTY TABLE" << std::endl; 
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // CONSTRUCTOR
  //////////////////////////////////////////////////////////////////////////////////////////
  tmpl(/**/)::VciRam(
		     sc_core::sc_module_name name,
		     uint32_t id,
		     const soclib::common::IntTab &index,
		     const soclib::common::MappingTable &mt,
		     common::ElfLoader &loader)
    : soclib::tlmt::BaseModule(name),
      m_index(index),
      m_mt(mt),
      m_loader(new common::ElfLoader(loader)),
      //m_atomic(8),// 8 equals to maximal number of initiator
      p_vci("vci", new tlmt_core::tlmt_callback<VciRam,soclib::tlmt::vci_cmd_packet<vci_param> *>(this, &VciRam<vci_param>::callback))
  {
    m_id = id;
    m_segments=m_mt.getSegmentList(m_index);
    m_contents = new ram_t*[m_segments.size()];
    size_t word_size = sizeof(typename vci_param::data_t);

    std::list<soclib::common::Segment>::iterator seg;
    size_t i;
    for (i=0, seg = m_segments.begin(); seg != m_segments.end(); ++i, ++seg ) {
      soclib::common::Segment &s = *seg;
      m_contents[i] = new ram_t[(s.size()+word_size-1)/word_size];
    }
    
    if ( m_loader ){
      for (i=0, seg = m_segments.begin(); seg != m_segments.end(); ++i, ++seg ) {
	soclib::common::Segment &s = *seg;
	m_loader->load(&m_contents[i][0], s.baseAddress(), s.size());
	for (size_t addr = 0; addr < s.size()/word_size; ++addr )
	  m_contents[i][addr] = le_to_machine(m_contents[i][addr]);
      }
    }

    //initialize the LL table 
    tableLL_begin = NULL;
    tableLL_end   = NULL;
    //m_atomic.clearAll();
  }

  tmpl(/**/)::~VciRam(){}
 
}}

