#ifndef UPDATE_TAB_V2_H_
#define UPDATE_TAB_V2_H_

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include "arithmetics.h"

////////////////////////////////////////////////////////////////////////
//                  An update tab entry    
////////////////////////////////////////////////////////////////////////
class UpdateTabEntry {
  typedef uint32_t size_t;
  typedef sc_dt::sc_uint<40> addr_t;

  public:
  bool 	valid;                // It is a valid pending transaction
  bool	update;               // It is an update transaction
  bool  brdcast;              // It is a broadcast invalidate
  bool  rsp;                  // It needs a response to the initiator
  size_t 	srcid;        // The srcid of the initiator which wrote the data
  size_t 	trdid;        // The trdid of the initiator which wrote the data
  size_t 	pktid;        // The pktid of the initiator which wrote the data
  addr_t	nline;	      // The identifier of the cache line
  size_t 	count;        // The number of acknowledge responses to receive

  UpdateTabEntry(){
    valid	= false;
    update  = false;
    brdcast = false;
    rsp     = false;
    srcid	= 0;
    trdid	= 0;
    pktid	= 0;
    nline	= 0;
    count	= 0;
  }

  UpdateTabEntry(bool   i_valid, 
      bool   i_update,
      bool   i_brdcast,
      bool   i_rsp,
      size_t i_srcid, 
      size_t i_trdid, 
      size_t i_pktid, 
      addr_t i_nline,
      size_t i_count) 
  {
    valid		= i_valid;
    update	    = i_update;
    brdcast     = i_brdcast;
    rsp         = i_rsp;
    srcid		= i_srcid;
    trdid		= i_trdid;
    pktid		= i_pktid;
    nline		= i_nline;
    count		= i_count;
  }

  UpdateTabEntry(const UpdateTabEntry &source)
  {
    valid   = source.valid;
    update  = source.update;
    brdcast = source.brdcast;
    rsp     = source.rsp;
    srcid   = source.srcid;
    trdid   = source.trdid;
    pktid   = source.pktid;
    nline   = source.nline;
    count   = source.count;
  }

  ////////////////////////////////////////////////////
  // The init() function initializes the entry 
  ///////////////////////////////////////////////////
  void init()
  {
    valid  = false;
    update = false;
    brdcast= false;
    rsp    = false;
    srcid  = 0;
    trdid  = 0;
    pktid  = 0;
    nline  = 0;
    count  = 0;
  }

  ////////////////////////////////////////////////////////////////////
  // The copy() function copies an existing entry
  // Its arguments are :
  // - source : the update tab entry to copy
  ////////////////////////////////////////////////////////////////////
  void copy(const UpdateTabEntry &source)
  {
    valid  = source.valid;
    update = source.update;
    brdcast= source.brdcast;
    rsp    = source.rsp;
    srcid  = source.srcid;
    trdid  = source.trdid;
    pktid  = source.pktid;
    nline  = source.nline;
    count  = source.count;
  }

  ////////////////////////////////////////////////////////////////////
  // The print() function prints the entry  
  ////////////////////////////////////////////////////////////////////
  void print(){
    std::cout << std::dec << "valid  = " << valid  << std::endl;
    std::cout << "update = " << update << std::endl;
    std::cout << "brdcast= " << brdcast<< std::endl;
    std::cout << "rsp    = " << rsp    << std::endl;
    std::cout << "srcid  = " << srcid  << std::endl; 
    std::cout << "trdid  = " << trdid  << std::endl; 
    std::cout << "pktid  = " << pktid  << std::endl; 
    std::cout << std::hex << "nline  = " << nline  << std::endl;
    std::cout << std::dec << "count  = " << count  << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////
//                        The update tab             
////////////////////////////////////////////////////////////////////////
class UpdateTab{

  typedef uint32_t size_t;
  typedef sc_dt::sc_uint<40> addr_t;

  private:
  size_t size_tab;
  std::vector<UpdateTabEntry> tab;

  public:

  UpdateTab()
    : tab(0)
  {
    size_tab=0;
  }

  UpdateTab(size_t size_tab_i)
    : tab(size_tab_i)
  {
    size_tab=size_tab_i;
  }

  ////////////////////////////////////////////////////////////////////
  // The size() function returns the size of the tab  
  ////////////////////////////////////////////////////////////////////
  const size_t size(){
    return size_tab;
  }


  ////////////////////////////////////////////////////////////////////
  // The size() function returns the size of the tab  
  ////////////////////////////////////////////////////////////////////
  void print(){
    for(size_t i=0; i<size_tab; i++) {
      std::cout << "UPDATE TAB ENTRY " << std::dec << i << "--------" << std::endl;
      tab[i].print();
    }
    return;
  }


  /////////////////////////////////////////////////////////////////////
  // The init() function initializes the tab 
  /////////////////////////////////////////////////////////////////////
  void init(){
    for ( size_t i=0; i<size_tab; i++) {
      tab[i].init();
    }
  }


  /////////////////////////////////////////////////////////////////////
  // The reads() function reads an entry 
  // Arguments :
  // - entry : the entry to read
  // This function returns a copy of the entry.
  /////////////////////////////////////////////////////////////////////
  UpdateTabEntry read (size_t entry)
  {
    assert(entry<size_tab && "Bad Update Tab Entry");
    return UpdateTabEntry(tab[entry]);
  }

  ///////////////////////////////////////////////////////////////////////////
  // The set() function writes an entry in the Update Table
  // Arguments :
  // - update : transaction type (bool)
  // - srcid : srcid of the initiator
  // - trdid : trdid of the initiator
  // - pktid : pktid of the initiator
  // - count : number of expected responses
  // - index : (return argument) index of the selected entry
  // This function returns true if the write successed (an entry was empty).
  ///////////////////////////////////////////////////////////////////////////
  bool set(const bool	update,
      const bool   brdcast,
      const bool   rsp,
      const size_t srcid,
      const size_t trdid,
      const size_t pktid,
      const addr_t nline,
      const size_t count,
      size_t &index)
  {
    for ( size_t i=0 ; i<size_tab ; i++ ) {
      if( !tab[i].valid ) {
        tab[i].valid		= true;
        tab[i].update		= update;
        tab[i].brdcast      = brdcast;
        tab[i].rsp          = rsp;
        tab[i].srcid		= (size_t) srcid;
        tab[i].trdid		= (size_t) trdid;
        tab[i].pktid		= (size_t) pktid;
        tab[i].nline		= (addr_t) nline;
        tab[i].count		= (size_t) count;
        index			    = i;
        return true;
      }
    }
    return false;
  } // end set()

  /////////////////////////////////////////////////////////////////////
  // The decrement() function decrements the counter for a given entry.
  // Arguments :
  // - index   : the index of the entry
  // - counter : (return argument) value of the counter after decrement
  // This function returns true if the entry is valid.
  /////////////////////////////////////////////////////////////////////
  bool decrement( const size_t index,
      size_t &counter ) 
  {
    assert((index<size_tab) && "Bad Update Tab Entry");
    if ( tab[index].valid ) {
      tab[index].count--;
      counter = tab[index].count;
      return true;
    } else {
      return false;
    }
  }

  /////////////////////////////////////////////////////////////////////
  // The is_full() function returns true if the table is full
  /////////////////////////////////////////////////////////////////////
  bool is_full()
  {
    for(size_t i = 0 ; i < size_tab ; i++){
      if(!tab[i].valid){
        return false;
      }
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////
  // The need_rsp() function returns the need of a response
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  bool need_rsp(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].rsp;	
  }

  /////////////////////////////////////////////////////////////////////
  // The is_update() function returns the transaction type
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  bool is_brdcast(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].brdcast;	
  }

  /////////////////////////////////////////////////////////////////////
  // The is_update() function returns the transaction type
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  bool is_update(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].update;	
  }

  /////////////////////////////////////////////////////////////////////
  // The srcid() function returns the srcid value
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  size_t srcid(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].srcid;	
  }

  /////////////////////////////////////////////////////////////////////
  // The trdid() function returns the trdid value
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  size_t trdid(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].trdid;	
  }

  /////////////////////////////////////////////////////////////////////
  // The pktid() function returns the pktid value
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  size_t pktid(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].pktid;	
  }

  /////////////////////////////////////////////////////////////////////
  // The nline() function returns the nline value
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////
  addr_t nline(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    return tab[index].nline;
  }

  /////////////////////////////////////////////////////////////////////
  // The search_inval() function returns the index of the entry in UPT
  // Arguments :
  // - nline : the line number of the entry in the directory
  /////////////////////////////////////////////////////////////////////
  bool search_inval(const addr_t nline,size_t &index)
  {
    size_t i ;

    for (i = 0 ; i < size_tab ; i++){
      if((tab[i].nline == nline) && tab[i].valid){
        if(!tab[i].update){
          index = i ;
          return true;
        }
      }
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////
  // The read_nline() function returns the index of the entry in UPT
  // Arguments :
  // - nline : the line number of the entry in the directory
  /////////////////////////////////////////////////////////////////////
  bool read_nline(const addr_t nline,size_t &index) 
  {
    size_t i ;

    for (i = 0 ; i < size_tab ; i++){
      if((tab[i].nline == nline) && tab[i].valid){
        index = i ;
        return true;
      }
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////
  // The clear() function erases an entry of the tab
  // Arguments :
  // - index : the index of the entry
  /////////////////////////////////////////////////////////////////////       
  void clear(const size_t index)
  {
    assert(index<size_tab && "Bad Update Tab Entry");
    tab[index].valid=false;
    return;	
  }

};

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

