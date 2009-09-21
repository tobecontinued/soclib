#ifndef XRAM_TRANSACTION_H_
#define XRAM_TRANSACTION_H_
 
#include <inttypes.h>
#include <systemc>
#include <cassert>
#include "arithmetics.h"

#define DEBUG_XRAM_TRANSACTION 0

////////////////////////////////////////////////////////////////////////
//                  A transaction tab entry         
////////////////////////////////////////////////////////////////////////

class TransactionTabEntry {
  typedef uint32_t size_t;
  typedef uint32_t data_t;
  typedef uint32_t be_t;

 public:
  bool 	valid;     		// entry valid 
  bool 	xram_read; 		// read request to XRAM
  data_t  	nline;    		// index (zy) of the requested line
  size_t 	srcid;     		// processor requesting the transaction
  size_t 	trdid;     		// processor requesting the transaction
  size_t 	pktid;     		// processor requesting the transaction
  bool 	proc_read;	 	// read request from processor
  bool 	single_word;   		// single word in case of processor read 
  size_t 	word_index;    		// word index in case of single word read
  std::vector<data_t> wdata;        	// write buffer (one cache line)
  std::vector<be_t> wdata_be;    	// be for each data in the write buffer

  /////////////////////////////////////////////////////////////////////
  // The init() function initializes the entry 
  /////////////////////////////////////////////////////////////////////
  void init()
  {
    valid		= false;
  }

  /////////////////////////////////////////////////////////////////////
  // The alloc() function initializes the vectors of an entry
  // Its arguments are :
  // - n_words : number of words per line in the cache
  /////////////////////////////////////////////////////////////////////
  void alloc(size_t n_words)
  {
    wdata_be.reserve( (int)n_words );
    wdata.reserve( (int)n_words );
    for(size_t i=0; i<n_words; i++){
      wdata_be.push_back(false);
      wdata.push_back(0);
    }
  }

  ////////////////////////////////////////////////////////////////////
  // The copy() function copies an existing entry
  // Its arguments are :
  // - source : the transaction tab entry to copy
  ////////////////////////////////////////////////////////////////////
  void copy(const TransactionTabEntry &source)
  {
    valid		= source.valid;
    xram_read 	= source.xram_read;
    nline		= source.nline;
    srcid		= source.srcid;
    trdid		= source.trdid;
    pktid		= source.pktid;
    proc_read 	= source.proc_read;
    single_word 	= source.single_word;
    word_index	= source.word_index;
    wdata_be.assign(source.wdata_be.begin(),source.wdata_be.end());
    wdata.assign(source.wdata.begin(),source.wdata.end());	
  }

  ////////////////////////////////////////////////////////////////////
  // The print() function prints the entry 
  ////////////////////////////////////////////////////////////////////
  void print(){
    std::cout << "valid       = " << valid        << std::endl;
    std::cout << "xram_read   = " << xram_read    << std::endl;
    std::cout << "nline       = " << nline        << std::endl;
    std::cout << "srcid       = " << srcid        << std::endl;
    std::cout << "trdid       = " << trdid        << std::endl;
    std::cout << "pktid       = " << pktid        << std::endl;
    std::cout << "proc_read   = " << proc_read    << std::endl;
    std::cout << "single_word = " << single_word  << std::endl;
    std::cout << "word_index  = " << word_index   << std::endl; 
    for(size_t i=0; i<wdata_be.size() ; i++){
      std::cout << "wdata_be [" << i <<"] = " << wdata_be[i] << std::endl;
    }
    for(size_t i=0; i<wdata.size() ; i++){
      std::cout << "wdata [" << i <<"] = " << wdata[i] << std::endl;
    }
    std::cout << std::endl;
  }

  /////////////////////////////////////////////////////////////////////
  // 		Constructors
  /////////////////////////////////////////////////////////////////////

  TransactionTabEntry()
    {
      wdata_be.clear();
      wdata.clear();
      valid=false;
    }

  TransactionTabEntry(const TransactionTabEntry &source){
    valid		= source.valid;
    xram_read	= source.xram_read;
    nline		= source.nline;
    srcid		= source.srcid;
    trdid		= source.trdid;
    pktid		= source.pktid;
    proc_read	= source.proc_read;
    single_word 	= source.single_word;
    word_index	= source.word_index;
    wdata_be.assign(source.wdata_be.begin(),source.wdata_be.end());
    wdata.assign(source.wdata.begin(),source.wdata.end());	
  }

}; // end class TransactionTabEntry

////////////////////////////////////////////////////////////////////////
//                  The transaction tab                              
////////////////////////////////////////////////////////////////////////
class TransactionTab{
  typedef uint32_t size_t;
  typedef uint32_t data_t;
  typedef uint32_t be_t;

 private:
  size_t size_tab;                // The size of the tab

  data_t be_to_mask(be_t be)
  {
    data_t ret = 0;
    if ( be&0x1 ) {
      ret = ret | 0x000000FF;
    }
    if ( be&0x2 ) {
      ret = ret | 0x0000FF00;
    }
    if ( be&0x4 ) {
      ret = ret | 0x00FF0000;
    }
    if ( be&0x8 ) {
      ret = ret | 0xFF000000;
    }
    return ret;
  }

 public:
  TransactionTabEntry *tab;       // The transaction tab

  ////////////////////////////////////////////////////////////////////
  //		Constructors
  ////////////////////////////////////////////////////////////////////
  TransactionTab()
    {
      size_tab=0;
      tab=NULL;
    }

  TransactionTab(size_t n_entries, size_t n_words)
    {
      size_tab = n_entries;
      tab = new TransactionTabEntry[size_tab];
      for ( size_t i=0; i<size_tab; i++) {
	tab[i].alloc(n_words);
      }
    }

  ~TransactionTab()
    {
      delete [] tab;
    }

  /////////////////////////////////////////////////////////////////////
  // The size() function returns the size of the tab
  /////////////////////////////////////////////////////////////////////
  size_t size()
  {
    return size_tab;
  }

  /////////////////////////////////////////////////////////////////////
  // The init() function initializes the transaction tab entries
  /////////////////////////////////////////////////////////////////////
  void init()
  {
    for ( size_t i=0; i<size_tab; i++) {
      tab[i].init();
    }
  }

  /////////////////////////////////////////////////////////////////////
  // The print() function prints a transaction tab entry
  // Arguments :
  // - index : the index of the entry to print
  /////////////////////////////////////////////////////////////////////
  void print(const size_t index)
  {
    assert( (index < size_tab) 
	    && "Invalid Transaction Tab Entry");
    tab[index].print();
    return;
  }

  /////////////////////////////////////////////////////////////////////
  // The read() function returns a transaction tab entry.
  // Arguments :
  // - index : the index of the entry to read
  /////////////////////////////////////////////////////////////////////
  TransactionTabEntry read(const size_t index)
  {
    assert( (index < size_tab) 
	    && "Invalid Transaction Tab Entry");
    return tab[index];
  }

  /////////////////////////////////////////////////////////////////////
  // The full() function returns the state of the transaction tab
  // Arguments :
  // - index : (return argument) the index of an empty entry 
  // The function returns true if the transaction tab is full
  /////////////////////////////////////////////////////////////////////
  bool full(size_t &index)
  {
    for(size_t i=0; i<size_tab; i++){
      if(!tab[i].valid){
	index=i;
	return false;	
      }
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////
  // The hit_read() function checks if a read XRAM transaction exists 
  // for a given cache line.
  // Arguments :
  // - index : (return argument) the index of the hit entry, if there is 
  // - nline : the index (zy) of the requested line
  // The function returns true if a read request has already been sent
  //////////////////////////////////////////////////////////////////////
  bool hit_read(const data_t nline,size_t &index)
  {
    for(size_t i=0; i<size_tab; i++){
      if(tab[i].valid && (nline==tab[i].nline) && (tab[i].xram_read)) {
        index=i;
        return true;	
      }
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////
  // The hit_write() function checks if a write XRAM transaction exists 
  // for a given cache line.
  // Arguments :
  // - index : (return argument) the index of the hit entry, if there is 
  // - nline : the index (zy) of the requested line
  // The function returns true if a write request has already been sent
  //////////////////////////////////////////////////////////////////////
  bool hit_write(const data_t nline)
  {
    for(size_t i=0; i<size_tab; i++){
      if(tab[i].valid && (nline==tab[i].nline) && !(tab[i].xram_read)) {
        return true;	
      }
    }
    return false;
  }


  /////////////////////////////////////////////////////////////////////
  // The write_data_mask() function writes a vector of data (a line).
  // The data is written only if the corresponding bits are set
  // in the be vector. 
  // Arguments :
  // - index : the index of the request in the transaction tab
  // - be   : vector of be 
  // - data : vector of data
  /////////////////////////////////////////////////////////////////////
  void write_data_mask(const size_t index, 
		       const std::vector<be_t> &be, 
		       const std::vector<data_t> &data) 
  {
    assert( (index < size_tab) 
	    && "Invalid Transaction Tab Entry");
    assert(be.size()==tab[index].wdata_be.size() 
	   && "Bad data mask in write_data_mask in TransactionTab");
    assert(data.size()==tab[index].wdata.size() 
	   && "Bad data in write_data_mask in TransactionTab");

    for(size_t i=0; i<tab[index].wdata_be.size() ; i++) {
      tab[index].wdata_be[i] = tab[index].wdata_be[i] | be[i];
      data_t mask = be_to_mask(be[i]);
      tab[index].wdata[i] = (tab[index].wdata[i] & ~mask) | (data[i] & mask);
    }
  }

  /////////////////////////////////////////////////////////////////////
  // The set() function registers a transaction (read or write)
  // to the XRAM in the transaction tab.
  // Arguments :
  // - index : index in the transaction tab
  // - xram_read : transaction type (read or write a cache line)
  // - nline : the index (zy) of the cache line
  // - srcid : srcid of the initiator that caused the transaction
  // - trdid : trdid of the initiator that caused the transaction
  // - pktid : pktid of the initiator that caused the transaction
  // - proc_read : does the initiator want a copy
  // - single_word : single word read (in case of processor read)
  // - word_index : index in the line (in case of single word read)
  // - data : the data to write (in case of write)
  // - data_be : the mask of the data to write (in case of write)
  /////////////////////////////////////////////////////////////////////
  void set(const size_t index,
	   const bool xram_read,
	   const data_t nline,
	   const size_t srcid,
	   const size_t trdid,
	   const size_t pktid,
	   const bool proc_read,
	   const bool single_word,
	   const size_t word_index,
	   const std::vector<be_t> &data_be,
	   const std::vector<data_t> &data) 
  {
    assert( (index < size_tab) 
	    && "The selected entry is out of range in set() Transaction Tab");
    assert(data_be.size()==tab[index].wdata_be.size() 
	   && "Bad data_be argument in set() TransactionTab");
    assert(data.size()==tab[index].wdata.size() 
	   && "Bad data argument in set() TransactionTab");

    tab[index].valid	= true;
    tab[index].xram_read	= xram_read;
    tab[index].nline	= nline;
    tab[index].srcid	= srcid;
    tab[index].trdid	= trdid;
    tab[index].pktid	= pktid;
    tab[index].proc_read	= proc_read;
    tab[index].single_word	= single_word;
    tab[index].word_index	= word_index;
    for(size_t i=0; i<tab[index].wdata.size(); i++) {
      tab[index].wdata_be[i] = data_be[i];
      tab[index].wdata[i]    = data[i];
    }
  }

  /////////////////////////////////////////////////////////////////////
  // The write_rsp() function writes a word of the response to an 
  // XRAM read transaction.
  // The data is only written when the corresponding BE field is Ox0.
  // Arguments :
  // - index : the index of the transaction in the transaction tab
  // - word_index : the index of the data in the line
  // - data : the data to write
  /////////////////////////////////////////////////////////////////////
  void write_rsp(const size_t index,
		 const size_t word,
		 const data_t data)
  {
    assert( (index < size_tab) 
	    && "Selected entry  out of range in write_rsp() Transaction Tab");
    assert( (word <= tab[index].wdata_be.size()) 
	    && "Bad word_index in write_rsp() in TransactionTab");
    assert( tab[index].valid 
	    && "Transaction Tab Entry invalid in write_rsp()");
    assert( tab[index].xram_read 
	    && "Selected entry is not an XRAM read transaction in write_rsp()");

    data_t mask = be_to_mask(tab[index].wdata_be[word]);
    tab[index].wdata[word] = (tab[index].wdata[word] & mask) | (data & ~mask);
  }

  /////////////////////////////////////////////////////////////////////
  // The erase() function erases an entry in the transaction tab.
  // Arguments :
  // - index : the index of the request in the transaction tab
  /////////////////////////////////////////////////////////////////////
  void erase(const size_t index)
  {
    assert( (index < size_tab) 
	    && "The selected entry is out of range in erase() Transaction Tab");
    tab[index].valid	= false;
  }
}; // end class TransactionTab

#endif
