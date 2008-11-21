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
//                    A directory entry                               
////////////////////////////////////////////////////////////////////////
class DirectoryEntry {

    typedef uint32_t tag_t;
    typedef uint32_t size_t;
    typedef uint32_t copy_t;

public:

    bool	 	valid;                  // entry valid
    bool	 	dirty;                  // entry dirty
    bool	 	lock;                   // entry locked
    tag_t	 	tag;                    // tag of the entry
    copy_t 		copies;              	// vector of copies 

    DirectoryEntry()
    {
	valid   = false;
	dirty   = false;
	lock    = false;
	tag     = 0;
	copies	= 0;
    }
    
    DirectoryEntry(const DirectoryEntry &source)
    {
	valid	= source.valid;
	dirty	= source.dirty;
	tag	= source.tag;
	lock	= source.lock;
	copies	= source.copies;
    }
    
    /////////////////////////////////////////////////////////////////////
    // The init() function initializes the entry 
    /////////////////////////////////////////////////////////////////////
    void init()
    {
	valid = false;
	dirty = false;
	lock  = false;
    }
    
    /////////////////////////////////////////////////////////////////////
    // The copy() function copies an existing source entry to a target 
    /////////////////////////////////////////////////////////////////////
    void copy(const DirectoryEntry &source)
    {
	valid	= source.valid;
	dirty	= source.dirty;
	tag	= source.tag;
	lock	= source.lock;
	copies	= source.copies;
    }

    ////////////////////////////////////////////////////////////////////
    // The print() function prints the entry 
    ////////////////////////////////////////////////////////////////////
    void print()
    {
	std::cout << "Valid = " << valid << " ; Dirty = " << dirty << " ; Lock = " 
  	   << lock << " ; Tag = " << std::hex << tag << " ; copies = " << copies << std::endl;
    }

}; // end class DirectoryEntry

////////////////////////////////////////////////////////////////////////
//                       The directory  
////////////////////////////////////////////////////////////////////////
class CacheDirectory {

    typedef uint32_t addr_t;
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
	m_ways 	= ways; 
	m_sets 	= sets;
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
	for(size_t i=0; i<m_ways; i++){
		if( (m_lru_tab[set][i].recent) && (m_dir_tab[set][i].lock)){
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
 
}} // end namespaces

#endif
