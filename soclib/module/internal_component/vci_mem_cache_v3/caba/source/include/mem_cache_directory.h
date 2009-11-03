#ifndef SOCLIB_CABA_MEM_CACHE_DIRECTORY_H
#define SOCLIB_CABA_MEM_CACHE_DIRECTORY_H 

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include "arithmetics.h"

namespace soclib { namespace caba {

  using namespace sc_core;

  ////////////////////////////////////////////////////////////////////////
  //                    A LRU entry 
  ////////////////////////////////////////////////////////////////////////
  class LruEntry {

    public:

      bool recent;            

      void init()
      {
        recent=false;
      }

  }; // end class LruEntry

  ////////////////////////////////////////////////////////////////////////
  //                    An Owner
  ////////////////////////////////////////////////////////////////////////
  class Owner{
    typedef uint32_t size_t;
    
    public:
    // Fields
      bool      inst;       // Is the owner an ICache ?
      size_t    srcid;      // The SRCID of the owner

    ////////////////////////
    // Constructors
    ////////////////////////
      Owner(bool i_inst,size_t i_srcid){
        inst    = i_inst;
        srcid   = i_srcid;
      }

      Owner(const Owner &a){
        inst    = a.inst;
        srcid   = a.srcid;
      }

      Owner(){
        inst    = false;
        srcid   = 0;
      }
      // end constructors

  }; // end class Owner


  ////////////////////////////////////////////////////////////////////////
  //                    A directory entry                               
  ////////////////////////////////////////////////////////////////////////
  class DirectoryEntry {

    typedef uint32_t tag_t;
    typedef uint32_t size_t;

    public:

    bool    valid;                  // entry valid
    bool    is_cnt;                 // directory entry is in counter mode
    bool    dirty;                  // entry dirty
    bool    lock;                   // entry locked
    bool    inst;                   // at least one ICache owner
    tag_t   tag;                    // tag of the entry
    size_t  count;                  // number of copies
    Owner   owner;                  // an owner of the line 
    size_t  ptr;                    // pointer to the next owner

    DirectoryEntry()
    {
      valid         = false;
      is_cnt        = false;
      dirty         = false;
      lock          = false;
      inst          = false;
      tag           = 0;
      count         = 0;
      owner.inst    = 0;
      owner.srcid   = 0;
      ptr           = 0;
    }

    DirectoryEntry(const DirectoryEntry &source)
    {
      valid         = source.valid;
      is_cnt        = source.is_cnt;
      dirty         = source.dirty;
      lock          = source.lock;
      inst          = source.inst;
      tag           = source.tag;
      count         = source.count;
      owner         = source.owner;
      ptr           = source.ptr;
    }          

    /////////////////////////////////////////////////////////////////////
    // The init() function initializes the entry 
    /////////////////////////////////////////////////////////////////////
    void init()
    {
      valid     = false;
      is_cnt    = false;
      dirty     = false;
      lock      = false;
      inst      = false;
      count     = 0;
    }

    /////////////////////////////////////////////////////////////////////
    // The copy() function copies an existing source entry to a target 
    /////////////////////////////////////////////////////////////////////
    void copy(const DirectoryEntry &source)
    {
      valid	    = source.valid;
      is_cnt    = source.is_cnt;
      dirty	    = source.dirty;
      lock	    = source.lock;
      inst      = source.inst;
      tag	    = source.tag;
      count     = source.count;
      owner     = source.owner;
      ptr       = source.ptr;
    }

    ////////////////////////////////////////////////////////////////////
    // The print() function prints the entry 
    ////////////////////////////////////////////////////////////////////
    void print()
    {
      std::cout << "Valid = " << valid << " ; IS COUNT = " << is_cnt << " ; Dirty = " << dirty << " ; Lock = " 
        << lock << " ; Inst = " << inst << " ; Tag = " << std::hex << tag << std::dec << " ; Count = " << count << " ; Owner = " << owner.srcid << " " << owner.inst << " ; Pointer = " << ptr << std::endl;
    }

  }; // end class DirectoryEntry

  ////////////////////////////////////////////////////////////////////////
  //                       The directory  
  ////////////////////////////////////////////////////////////////////////
  class CacheDirectory {

    typedef sc_dt::sc_uint<40> addr_t;
    typedef uint32_t data_t;
    typedef uint32_t tag_t;
    typedef uint32_t size_t;

    private:

    // Directory constants
    size_t					m_ways;
    size_t					m_sets;
    size_t					m_words;
    size_t					m_width;

    // the directory & lru tables
    DirectoryEntry 				**m_dir_tab;
    LruEntry	 				**m_lru_tab;

    public:

    ////////////////////////
    // Constructor
    ////////////////////////
    CacheDirectory( size_t ways, size_t sets, size_t words, size_t address_width)	 
    {
      m_ways  = ways; 
      m_sets  = sets;
      m_words = words;
      m_width = address_width;

      m_dir_tab = new DirectoryEntry*[sets];
      for ( size_t i=0; i<sets; i++ ) {
        m_dir_tab[i] = new DirectoryEntry[ways];
        for ( size_t j=0 ; j<ways ; j++) m_dir_tab[i][j].init();
      }
      m_lru_tab = new LruEntry*[sets];
      for ( size_t i=0; i<sets; i++ ) {
        m_lru_tab[i] = new LruEntry[ways];
        for ( size_t j=0 ; j<ways ; j++) m_lru_tab[i][j].init();
      }
    } // end constructor

    /////////////////
    // Destructor
    /////////////////
    ~CacheDirectory()
    {
      for(size_t i=0 ; i<m_sets ; i++){
        delete [] m_dir_tab[i];
        delete [] m_lru_tab[i];
      }
      delete [] m_dir_tab;
      delete [] m_lru_tab;
    } // end destructor

    /////////////////////////////////////////////////////////////////////
    // The read() function reads a directory entry. In case of hit, the
    // LRU is updated.
    // Arguments :
    // - address : the address of the entry 
    // - way : (return argument) the way of the entry in case of hit
    // The function returns a copy of a (valid or invalid) entry  
    /////////////////////////////////////////////////////////////////////
    DirectoryEntry read(const addr_t &address,size_t &way)
    {

#define L2 soclib::common::uint32_log2
      const size_t set = (size_t)(address >> (L2(m_words) + 2)) & (m_sets - 1);
      const tag_t  tag = (tag_t)(address >> (L2(m_sets) + L2(m_words) + 2));
#undef L2

      bool hit       = false;
      for ( size_t i=0 ; i<m_ways ; i++ ) {
        bool equal = ( m_dir_tab[set][i].tag == tag );
        bool valid = m_dir_tab[set][i].valid;
        hit = equal && valid;
        if ( hit ) {			
          way = i;
          break;
        } 
      }
      if ( hit ) {
        m_lru_tab[set][way].recent = true;
        return DirectoryEntry(m_dir_tab[set][way]);
      } else {
        return DirectoryEntry();
      }
    } // end read()

    /////////////////////////////////////////////////////////////////////
    // The write function writes a new entry, 
    // and updates the LRU bits if necessary.
    // Arguments :
    // - set : the set of the entry
    // - way : the way of the entry
    // - entry : the entry value
    /////////////////////////////////////////////////////////////////////
    void write(const size_t &set, const size_t &way, const DirectoryEntry &entry)
    {
      assert( (set<m_sets) 
          && "Cache Directory write : The set index is invalid");
      assert( (way<m_ways) 
          && "Cache Directory write : The way index is invalid");

      // update Directory
      m_dir_tab[set][way].copy(entry);

      // update LRU bits
      bool all_recent = true;
      for ( size_t i=0 ; i<m_ways ; i++ ) {
        if ( i != way ) all_recent = m_lru_tab[set][i].recent && all_recent;
      }
      if ( all_recent ) {
        for( size_t i=0 ; i<m_ways ; i++ ) m_lru_tab[set][i].recent = false;
      } else {
        m_lru_tab[set][way].recent = true;
      }
    } // end write()

    /////////////////////////////////////////////////////////////////////
    // The print() function prints a selected directory entry
    // Arguments :
    // - set : the set of the entry to print
    // - way : the way of the entry to print
    /////////////////////////////////////////////////////////////////////
    void print(const size_t &set, const size_t &way)
    {
      std::cout << std::dec << " set : " << set << " ; way : " << way << " ; " ;
      m_dir_tab[set][way].print();
    } // end print()

    /////////////////////////////////////////////////////////////////////
    // The select() function selects a directory entry to evince.
    // Arguments :
    // - set   : (input argument) the set to modify
    // - way   : (return argument) the way to evince
    /////////////////////////////////////////////////////////////////////
    DirectoryEntry select(const size_t &set, size_t &way)
    {
      assert( (set < m_sets) 
          && "Cache Directory : (select) The set index is invalid");

      for(size_t i=0; i<m_ways; i++){
        if(!m_dir_tab[set][way].valid){
          way=i;
          return DirectoryEntry(m_dir_tab[set][way]);
        }
      }
      for(size_t i=0; i<m_ways; i++){
        if(!(m_lru_tab[set][i].recent) && !(m_dir_tab[set][i].lock)){
          way=i;
          return DirectoryEntry(m_dir_tab[set][way]);
        }
      }
      for(size_t i=0; i<m_ways; i++){
        if( !(m_lru_tab[set][i].recent) && (m_dir_tab[set][i].lock)){
          way=i;
          return DirectoryEntry(m_dir_tab[set][way]);
        }
      }
      for(size_t i=0; i<m_ways; i++){
        if( (m_lru_tab[set][i].recent) && !(m_dir_tab[set][i].lock)){
          way=i;
          return DirectoryEntry(m_dir_tab[set][way]);
        }
      }
      way = 0;
      return DirectoryEntry(m_dir_tab[set][0]);
    } // end select()

    /////////////////////////////////////////////////////////////////////
    // 		Global initialisation function
    /////////////////////////////////////////////////////////////////////
    void init()
    {
      for ( size_t set=0 ; set<m_sets ; set++ ) {
        for ( size_t way=0 ; way<m_ways ; way++ ) {
          m_dir_tab[set][way].init();
          m_lru_tab[set][way].init();
        }
      }
    } // end init()

  }; // end class CacheDirectory

  ///////////////////////////////////////////////////////////////////////
  //                    A Heap Entry
  ///////////////////////////////////////////////////////////////////////
  class HeapEntry{
    typedef uint32_t size_t;

    public:
    // Fields of the entry
      Owner     owner;
      size_t    next;

    ////////////////////////
    // Constructor
    ////////////////////////
      HeapEntry()
      :owner(false,0)
      {
        next = 0;
      } // end constructor

    ////////////////////////
    // Constructor
    ////////////////////////
      HeapEntry(const HeapEntry &entry){
        owner.srcid = entry.owner.srcid;
        owner.inst  = entry.owner.inst;
        next        = entry.next;
      } // end constructor

    /////////////////////////////////////////////////////////////////////
    // The copy() function copies an existing source entry to a target 
    /////////////////////////////////////////////////////////////////////
      void copy(const HeapEntry &entry){
        owner.srcid = entry.owner.srcid;
        owner.inst  = entry.owner.inst;
        next        = entry.next;
      } // end copy()

    ////////////////////////////////////////////////////////////////////
    // The print() function prints the entry 
    ////////////////////////////////////////////////////////////////////
      void print(){
        std::cout 
        << " -- owner.srcid : " << std::dec << owner.srcid << std::endl
        << " -- owner.inst  : " << std::dec << owner.inst << std::endl
        << " -- next        : " << std::dec << next << std::endl;

      } // end print()

  }; // end class HeapEntry

  ////////////////////////////////////////////////////////////////////////
  //                        The Heap 
  ////////////////////////////////////////////////////////////////////////
  class HeapDirectory{
    typedef uint32_t size_t;
    
    private:
    // Registers and the heap
      size_t    ptr_free;
      bool      full;
      HeapEntry *m_heap_tab;

    // Constants for debugging purpose
      size_t    tab_size;

    public:
    ////////////////////////
    // Constructor
    ////////////////////////
      HeapDirectory(uint32_t size){
        assert(size>0 && "Memory Cache, HeapDirectory constructor : invalid size");
        ptr_free    = 0;
        full        = false;
        m_heap_tab  = new HeapEntry[size];
        tab_size    = size;
      } // end constructor

    /////////////////
    // Destructor
    /////////////////
      ~HeapDirectory(){
        delete [] m_heap_tab;
      } // end destructor

    /////////////////////////////////////////////////////////////////////
    // 		Global initialisation function
    /////////////////////////////////////////////////////////////////////
      void init(){
        ptr_free=0;
        full=false;
        for(size_t i=0; i< tab_size-1;i++){
          m_heap_tab[i].next = i+1;
        }
        m_heap_tab[tab_size-1].next = tab_size-1;
        return;
      }

    /////////////////////////////////////////////////////////////////////
    // The print() function prints a selected directory entry
    // Arguments :
    // - ptr : the pointer to the entry to print
    /////////////////////////////////////////////////////////////////////
      void print(const size_t &ptr){
        std::cout << "Heap, printing the entry : " << std::dec << ptr << std::endl;
        m_heap_tab[ptr].print();
      } // end print()

    /////////////////////////////////////////////////////////////////////
    // The is_full() function return true if the heap is full.
    /////////////////////////////////////////////////////////////////////
      bool is_full(){
        return full;
      } // end is_full()

    /////////////////////////////////////////////////////////////////////
    // The next_free_ptr() function returns the pointer 
    // to the next free entry.
    /////////////////////////////////////////////////////////////////////
      size_t next_free_ptr(){
        return ptr_free;
      } // end next_free_ptr()

    /////////////////////////////////////////////////////////////////////
    // The next_free_entry() function returns 
    // a copy of the next free entry.
    /////////////////////////////////////////////////////////////////////
      HeapEntry next_free_entry(){
        return HeapEntry(m_heap_tab[ptr_free]);
      } // end next_free_entry()
   
    /////////////////////////////////////////////////////////////////////
    // The write_free_entry() function modify the next free entry.
    // Arguments :
    // - entry : the entry to write
    /////////////////////////////////////////////////////////////////////
      void write_free_entry(const HeapEntry &entry){
        m_heap_tab[ptr_free].copy(entry);
      } // end write_free_entry()

    /////////////////////////////////////////////////////////////////////
    // The write_free_ptr() function writes the pointer
    // to the next free entry
    /////////////////////////////////////////////////////////////////////
      void write_free_ptr(const size_t &ptr){
        assert( (ptr<tab_size) && "HeapDirectory error : try to write a wrong free pointer");
        ptr_free = ptr;
      } // end write_free_ptr()

    /////////////////////////////////////////////////////////////////////
    // The set_full() function sets the full bit (to true).
    /////////////////////////////////////////////////////////////////////
      void set_full(){
        full = true;
      } // end set_full()

    /////////////////////////////////////////////////////////////////////
    // The unset_full() function unsets the full bit (to false).
    /////////////////////////////////////////////////////////////////////
      void unset_full(){
        full = false;
      } // end unset_full()

    /////////////////////////////////////////////////////////////////////
    // The read() function returns a copy of
    // the entry pointed by the argument
    // Arguments :
    //  - ptr : the pointer to the entry to read
    /////////////////////////////////////////////////////////////////////
      HeapEntry read(const size_t &ptr){
        assert( (ptr<tab_size) && "HeapDirectory error : try to write a wrong free pointer");
        return HeapEntry(m_heap_tab[ptr]);
      } // end read()

    /////////////////////////////////////////////////////////////////////
    // The write() function writes an entry in the heap
    // Arguments :
    //  - ptr : the pointer to the entry to replace
    //  - entry : the entry to write
    /////////////////////////////////////////////////////////////////////
      void write(const size_t &ptr, const HeapEntry &entry){
        assert( (ptr<tab_size) && "HeapDirectory error : try to write a wrong free pointer");
        m_heap_tab[ptr].copy(entry);
      } // end write()

  }; // end class HeapDirectory


}} // end namespaces

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

