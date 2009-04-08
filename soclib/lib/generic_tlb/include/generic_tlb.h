/*****************************************************************
 * File         : generic_tlb.h
 * Date         : 03/11/2007
 * Authors      : Alain Greiner, Yang GAO
 * Copyright    : UPMC/LIP6
 * 
 * This file is distributed under the LGPL licence agreement.
 ******************************************************************
 * This object is a generic TLB (Translation Lookaside Buffer).
 * It is implemented as a set-associative cache.
 * The replacement algorithm is pseudo-LRU.
 * Each TLB entry has the following format:
 * - int        ET          (00: unmapped; 01: unused or PTD)
 *                          (10: PTE new;  11: PTE old      )
 * - bool       uncachable  (uncached)
 * - bool       writable    (writable access bit)
 * - bool       executable  (executable access bit)
 * - bool       user        (access in user mode allowed)
 * - bool       global      (PTE not invalidated by a TLB flush)
 * - bool       dirty       (page has been modified)
 * - bool       lru         (recently used for replace in TLB)
 * - uint32_t   vpn         (virtual page number)
 * - uint32_t   ppn         (physical page number)
 *****************************************************************
 * This file has three constructor parameters:
 * - nways  : number of ways per associative set.
 * - nsets  : number of associative sets.
 * - nbits  : number of bits defining the page width [10,28]
 * Both nways & nsets must be power of 2 no larger than 64.
 *****************************************************************/

#ifndef SOCLIB_CABA_GENERIC_TLB_H
#define SOCLIB_CABA_GENERIC_TLB_H

#include <inttypes.h>
#include <systemc>
#include "arithmetics.h"

namespace soclib { 
namespace caba {

    // define ET state of PTE 
    enum {  
        UNMAPPED,
        PTD,
        PTE_NEW,
        PTE_OLD,
    };

    // PTE information struct
    typedef struct pte_info_s {
        size_t et;     
        bool c;
        bool w;
        bool x;
        bool u;
        bool g;
        bool d;
    }pte_info_t;

    enum {  
        PTE_ET_MASK   = 0xC0000000,
        PTE_C_MASK    = 0x00000020,
        PTE_W_MASK    = 0x00000010,
        PTE_X_MASK    = 0x00000008,
        PTE_U_MASK    = 0x00000004,
        PTE_G_MASK    = 0x00000002,
        PTE_D_MASK    = 0x00000001,
        //PTD_PTP_MASK  = 0x00FFFFFF,
        PTD_PTP_MASK  = 0x3FFFFFC0,
        PTD_ID2_MASK  = 0x003FF000,
        PPN_M_MASK    = 0x000FFFC0,
        OFFSET_K_MASK = 0x00000FFF,
        OFFSET_M_MASK = 0x003FFFFF,
    };

    enum {  
        PTE_ET_SHIFT = 30,
        PTD_SHIFT    = 6,
        PTE_C_SHIFT  = 5,
        PTE_W_SHIFT  = 4,
        PTE_X_SHIFT  = 3,
        PTE_U_SHIFT  = 2,
        PTE_G_SHIFT  = 1,
        PTE_D_SHIFT  = 0,
    };

    enum {  
        PAGE_M_NBITS = 22,
        PAGE_K_NBITS = 12,
    };

using soclib::common::uint32_log2;

template<typename addr_t>
class GenericTlb
{
public:
    size_t    m_nways;
    size_t    m_nsets;
    size_t    m_page_shift;
    size_t    m_page_mask;
    size_t    m_sets_shift;
    size_t    m_sets_mask;

    uint32_t  *m_ppn; 
    uint32_t  *m_vpn;
    size_t    *m_et;
    bool      *m_lru;
    bool      *m_cachable;
    bool      *m_writable;
    bool      *m_executable;
    bool      *m_user;
    bool      *m_global;
    bool      *m_dirty;

    // access methods 
    inline uint32_t &ppn(size_t way, size_t set)
    { 
        return m_ppn[(way*m_nsets)+set]; 
    }

    inline uint32_t &vpn(size_t way, size_t set)
    { 
        return m_vpn[(way*m_nsets)+set]; 
    }

    inline bool &lru(size_t way, size_t set)
    { 
        return m_lru[(way*m_nsets)+set]; 
    }

    inline size_t &et(size_t way, size_t set)
    { 
        return m_et[(way*m_nsets)+set]; 
    }

    inline bool &cachable(size_t way, size_t set)
    { 
        return m_cachable[(way*m_nsets)+set]; 
    }

    inline bool &writable(size_t way, size_t set)
    { 
        return m_writable[(way*m_nsets)+set]; 
    }

    inline bool &executable(size_t way, size_t set)
    { 
        return m_executable[(way*m_nsets)+set]; 
    }

    inline bool &user(size_t way, size_t set)
    { 
        return m_user[(way*m_nsets)+set]; 
    }

    inline bool &global(size_t way, size_t set)
    { 
        return m_global[(way*m_nsets)+set]; 
    }

    inline bool &dirty(size_t way, size_t set)
    { 
        return m_dirty[(way*m_nsets)+set]; 
    }

public:
    //////////////////////////////////////////////////////////////
    // constructor checks parameters, allocates the memory
    // and computes m_page_mask, m_sets_mask and m_sets_shift
    //////////////////////////////////////////////////////////////
    GenericTlb(size_t nways, size_t nsets, size_t nbits)
    {
        m_nways = nways;
        m_nsets = nsets;
 
        m_sets_shift = uint32_log2(nsets);
        m_sets_mask = (1<<(int)uint32_log2(nsets))-1;

        assert(IS_POW_OF_2(nsets));
        assert(IS_POW_OF_2(nways));
        assert(nsets <= 64);
        assert(nways <= 64);

        if((nbits < 10) || (nbits > 28) )
        {
            printf("Error in the genericTlb component\n");
            printf("The nbits parameter must be in the range [10,28]\n");
            exit(1);
        } 
        else 
        {
            m_page_mask  = 0xFFFFFFFF >> (32 - nbits);
            m_page_shift = nbits;    // nomber of page offset bits 
        }

        m_ppn        = new uint32_t[nways * nsets];
        m_vpn        = new uint32_t[nways * nsets];
        m_lru        = new bool[nways * nsets];
        m_et         = new size_t[nways * nsets];
        m_cachable   = new bool[nways * nsets];
        m_writable   = new bool[nways * nsets];
        m_executable = new bool[nways * nsets];
        m_user       = new bool[nways * nsets];
        m_global     = new bool[nways * nsets];
        m_dirty      = new bool[nways * nsets];

    } // end constructor

    ~GenericTlb()
    {
        delete [] m_ppn;
        delete [] m_vpn;
        delete [] m_lru;
        delete [] m_et;
        delete [] m_cachable;
        delete [] m_writable;
        delete [] m_executable;
        delete [] m_user;
        delete [] m_global;
        delete [] m_dirty;
    }

    /////////////////////////////////////////////////////////////
    //  This method resets all the TLB entry.
    /////////////////////////////////////////////////////////////
    inline void reset() 
    {
        memset(m_et, UNMAPPED, sizeof(*m_et)*m_nways*m_nsets);
    } 

    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical address, 
    //  the pte informations, way and set are returned. 
    /////////////////////////////////////////////////////////
    inline bool translate(  uint32_t vaddress,      // virtual address
                            addr_t *paddress,       // return physique address
                            pte_info_t *pte_info,   // return pte information
                            size_t *tw,             // return way  
                            size_t *ts )            // return set   
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask; 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift)))) // TLB hit
            {  
                pte_info->et = et(way,set);
                pte_info->c = cachable(way,set);
                pte_info->w = writable(way,set);
                pte_info->x = executable(way,set);
                pte_info->u = user(way,set);
                pte_info->g = global(way,set);
                pte_info->d = dirty(way,set);
                *tw = way;
                *ts = set;
                *paddress = (addr_t)((addr_t)ppn(way,set) << m_page_shift) | (addr_t)(vaddress & m_page_mask);
                return true;   
            } 
        } 
        return false;
    } // end translate()

    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical page number is returned. 
    /////////////////////////////////////////////////////////
    inline bool translate(uint32_t vaddress, addr_t *paddress) 
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask; 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift)))) // TLB hit
            {  
                *paddress = (addr_t)((addr_t)ppn(way,set) << m_page_shift) | (addr_t)(vaddress & m_page_mask);
                return true;   
            } 
        } 
        return false;
    } // end translate()

    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical address, 
    //  the cached bit, the writable bit, the executable bit, 
    //  the protecte bit, and the dirty bit are returned.
    //  The dirty bit is updated if necessary.
    /////////////////////////////////////////////////////////
    inline bool translate(uint32_t vaddress) 
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask; 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift))))  // TLB hit
            { 
                 return true;   
            } 
        } 
        return false;
    } // end translate()

    /////////////////////////////////////////////////////////////
    //  This method resets all VALID bits in one cycle,
    //  when the the argument is true.
    //  Locked descriptors are preserved when it is false.
    /////////////////////////////////////////////////////////////
    inline void flush(bool all) 
    {
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            for(size_t set = 0; set < m_nsets; set++) 
            {
                if(global(way,set)) 
                {
                    if(all) et(way,set) = UNMAPPED;  // forced reset, the locked page invalid too
                } 
                else 
                {
                    et(way,set) = UNMAPPED; // not forced reset, the locked page conserve  
                }
            } 
        } 
    } // end flush

    //////////////////////////////////////////////////////////////
    //  This method modifies all LRU bits of a given set :
    //  The LRU bit of the accessed descriptor is set to true,
    //  all other LRU bits in the set are reset to false.
    /////////////////////////////////////////////////////////////
    inline void setlru(size_t way,size_t set)   
    {
        lru(way,set) = true;  // set bit lru for recently used
    } // end setlru()

    //////////////////////////////////////////////////////////////
    //  This method modifies all LRU bits of a given set :
    //  The LRU bit of the accessed descriptor is set to true,
    //  all other LRU bits in the set are reset to false.
    /////////////////////////////////////////////////////////////
    inline uint32_t getpte(size_t way,size_t set)   
    {
        uint32_t pte = (uint32_t)(((~0)>>(m_page_shift-4)) & ppn(way,set)) << 6 | (PTE_ET_MASK & (et(way,set) << PTE_ET_SHIFT)); 

        if ( cachable(way,set) )
            pte = pte | PTE_C_MASK;
        if ( writable(way,set) )
            pte = pte | PTE_W_MASK;
        if ( executable(way,set) )
            pte = pte | PTE_X_MASK;
        if ( user(way,set) )
            pte = pte | PTE_U_MASK;
        if ( global(way,set) )
            pte = pte | PTE_G_MASK;
        if ( dirty(way,set) )
            pte = pte | PTE_D_MASK;

        return pte;
    } // end getpte()

    /////////////////////////////////////////////////////////////
    //  This method return the index of the least recently
    //  used descriptor in the associative set.
    /////////////////////////////////////////////////////////////
    inline size_t getlru(size_t set)
    {
        size_t defaul = 0;
        
        // check val bit firstly, replace the invalid PTE
        for(size_t way = 0; way < m_nways; way++) 
        {
            if( et(way,set) == UNMAPPED ) 
            {
                return way;
            }
        } 

        // then we check bit lock ... 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if( !global(way,set) && !lru(way,set) ) 
            {
                return way;
            } 
        }

        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if( !global(way,set) && lru(way,set) ) 
            {
                return way;
            } 
        }

        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if( global(way,set) && !lru(way,set) ) 
            {
                return way;
            } 
        }

        // reset lru bits of four ways
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            lru(way,set) = false; 
        }
 
        return defaul;
    } // end getlru()

    /////////////////////////////////////////////////////////////
    //  This method writes a new entry in the TLB.
    /////////////////////////////////////////////////////////////
    inline void update(size_t pte, size_t vaddress) 
    {
        
        size_t set = (vaddress >> m_page_shift) & m_sets_mask;
        size_t way = getlru(set);

        for(size_t i = 0; i < m_nways; i++) 
        {
            if (vpn(i,set) == (vaddress >> (m_page_shift + m_sets_shift))) 
            {
                way = i;
                break;
            }
        }
        
        vpn(way,set) = vaddress >> (m_page_shift + m_sets_shift);
        ppn(way,set) = ( pte << (m_page_shift - 10) ) >> (m_page_shift - 4);  

        cachable(way,set)   = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        executable(way,set) = (((pte & PTE_X_MASK)  >> PTE_X_SHIFT) == 1) ? true : false;
        user(way,set)       = (((pte & PTE_U_MASK)  >> PTE_U_SHIFT) == 1) ? true : false;
        global(way,set)     = (((pte & PTE_G_MASK)  >> PTE_G_SHIFT) == 1) ? true : false;
        writable(way,set)   = (((pte & PTE_W_MASK)  >> PTE_W_SHIFT) == 1) ? true : false;       
        dirty(way,set)      = (((pte & PTE_D_MASK)  >> PTE_D_SHIFT) == 1) ? true : false; 
        et(way,set)         = PTE_OLD; 

    } // end update()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    inline void inval(uint32_t vaddress)
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask;
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if(vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift))) 
            {
                if ( !global(way,set) ) et(way,set) = UNMAPPED;
            } 
        } 
    } // end inval()

    /////////////////////////////////////////////////////////////
    //  This method writes a new entry in the TLB.
    /////////////////////////////////////////////////////////////
    inline void setdirty(size_t way, size_t set)
    {
        dirty(way,set) = true;
    } // end setdirty()

}; // GenericTlb

template<typename addr_t>
class GenericCcTlb : public GenericTlb<addr_t>
{
public:
    size_t  *m_nline; 

    // access methods 
    inline size_t &nline(size_t way, size_t set)
    { 
        return m_nline[(way*this->m_nsets)+set]; 
    }

public:
    //////////////////////////////////////////////////////////////
    // constructor checks parameters, allocates the memory
    // and computes m_page_mask, m_sets_mask and m_sets_shift
    //////////////////////////////////////////////////////////////
    GenericCcTlb(size_t nways, size_t nsets, size_t nbits):GenericTlb<addr_t>::GenericTlb(nways, nsets, nbits)
    {
        m_nline = new size_t[this->m_nways * this->m_nsets];
    } // end constructor

    ~GenericCcTlb()
    {
        delete [] m_nline;
    }

    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical address, 
    //  the pte informations, way and set are returned. 
    /////////////////////////////////////////////////////////
    inline bool cctranslate(  uint32_t vaddress,    // virtual address
                            addr_t *paddress,       // return physique address
                            pte_info_t *pte_info,   // return pte information
                            size_t *victim_index,   // return nline
                            size_t *tw,             // return way  
                            size_t *ts )            // return set   
    {
        size_t set = (vaddress >> this->m_page_shift) & this->m_sets_mask; 
        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            if((this->et(way,set) > 1) &&     // PTE
               (this->vpn(way,set) == (vaddress >> (this->m_page_shift + this->m_sets_shift)))) // TLB hit
            {  
                pte_info->et = this->et(way,set);
                pte_info->c = this->cachable(way,set);
                pte_info->w = this->writable(way,set);
                pte_info->x = this->executable(way,set);
                pte_info->u = this->user(way,set);
                pte_info->g = this->global(way,set);
                pte_info->d = this->dirty(way,set);
                *victim_index = nline(way,set);
                *tw = way;
                *ts = set;
                *paddress = (addr_t)((addr_t)this->ppn(way,set) << this->m_page_shift) | (addr_t)(vaddress & this->m_page_mask);
                return true;   
            } 
        } 
        return false;
    } // end translate()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    inline bool cccheck( addr_t n_line, 
                         size_t start_way, size_t start_set, 
                         size_t* n_way, size_t* n_set,
                         bool* end )
    {
        for( size_t way = start_way; way < this->m_nways; way++ ) 
        {
            for( size_t set = start_set; set < this->m_nsets; set++ ) 
            {
                if (nline(way,set) == n_line) 
                {
                    *n_way = way;
                    *n_set = set;
                    if ( way == (this->m_nways-1) && set == (this->m_nsets-1) )
                    {
                        *end = true;
                    }
                    else
                    {
                        *end = false;
                    }
                    return true;
                }
            } 
        } 
        return false;
    } // end cccheck()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    inline void findpost(size_t way, size_t set, size_t* start_way, size_t* start_set)
    {
        if ( set == (this->m_nsets-1) )
        {
            *start_set = 0;
            *start_way = way + 1;
        }
        else
        {
            *start_set = set + 1;
        }
    } // end postcheck()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    inline bool inval(uint32_t vaddress, size_t* victim)
    {
        size_t set = (vaddress >> this->m_page_shift) & this->m_sets_mask;
        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            if(this->vpn(way,set) == (vaddress >> (this->m_page_shift + this->m_sets_shift))) 
            {
                *victim = nline(way,set);
                if ( !this->global(way,set) ) this->et(way,set) = UNMAPPED;
            } 
        } 

        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            for( size_t set = 0; set < this->m_nsets; set++ ) 
            {
                if (nline(way,set) == *victim) 
                {
                    return false;
                }
            } 
        } 
        return true;
    } // end inval()

    //////////////////////////////////////////////////////////////
    //  This method coherence invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    inline void ccinval(size_t invway, size_t invset)
    {
        this->et(invway,invset) = UNMAPPED;
    } // end ccinval()

    //////////////////////////////////////////////////////////////
    //  This method coherence invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    inline size_t getnline(size_t way, size_t set)
    {
        return nline(way,set);
    } // end getnline()

    /////////////////////////////////////////////////////////////
    //  This method is used for context switch. In this case, all 
    //  TLB entries should be invalidated except global entries.
    //  All the entries that are in the same data cache line have
    //  the same NLINE. Therefore only all the entries of one data 
    //  cache line are not global, it send a cleanup request to 
    //  data cache. The return value indicates whether need a cleanup
    //  and cleanup_nline contains the NLINE that is for cleanup.   
    /////////////////////////////////////////////////////////////
    inline bool checkcleanup(size_t nway, size_t nset, size_t* cleanup_nline)
    {
        bool cleanup = false;
        bool isglobal = false;
        if ( this->et(nway,nset) != UNMAPPED )
        {
            size_t inval_line = nline(nway,nset);
            for ( size_t way = nway; way < this->m_nways; way++ )
            {            
                for ( size_t set = nset; set < this->m_nsets; set++ )            
                {
                    if ( (this->et(way,set) != UNMAPPED) && (inval_line == nline(way,set)) )
                    {
                        if (!this->global(way,set))
                        {
                            this->et(way,set) = UNMAPPED;
                        }
                        else
                        {
                            isglobal = true;
                        }
                    }
                }
            }
            cleanup = !isglobal;
            if(cleanup)
                *cleanup_nline = inval_line;
        }
        return cleanup;
    } // end checkcleanup

    /////////////////////////////////////////////////////////////
    //  This method writes a new entry in the TLB.
    /////////////////////////////////////////////////////////////
    inline bool update(size_t pte, size_t vaddress,size_t line, size_t* victim ) 
    {
        bool cleanup = false;
        bool found = false;
        size_t set = (vaddress >> this->m_page_shift) & this->m_sets_mask;
        size_t selway = 0;
        bool hit = false;

        for(size_t way = 0; way < this->m_nways; way++) 
        {
            if (this->vpn(way,set) == (vaddress >> (this->m_page_shift + this->m_sets_shift))) 
            {
                selway = way;
                hit = true;
                break;
            }
        }

        if ( !hit )
        {
            // check val bit firstly, replace the invalid PTE
            for(size_t way = 0; way < this->m_nways && !found; way++) 
            {
                if( this->et(way,set) == UNMAPPED ) 
                {
                    found = true;
                    cleanup = false;
                    selway = way;
                }
            } 
            if ( !found ) // if no invalid way, we check bit lock ...
            {
                for( size_t way = 0; way < this->m_nways && !found; way++ ) 
                {
                    if( !this->global(way,set) && !this->lru(way, set) ) 
                    {
                        found = true;
                        cleanup = true;
                        selway = way;
                    } 
                }
                for( size_t way = 0; way < this->m_nways && !found; way++ ) 
                {
                    if( !this->global(way,set) && this->lru(way, set) ) 
                    {
                        found = true;
                        cleanup = true;
                        selway = way;
                    } 
                }
                for( size_t way = 0; way < this->m_nways && !found; way++ ) 
                {
                    if( this->global(way,set) && !this->lru(way, set) ) 
                    {
                        found = true;
                        cleanup = true;
                        selway = way;
                    } 
                }
                if ( !found )
                {
                    for( size_t way = 0; way < this->m_nways; way++ ) 
                    {
                        this->lru(way,set) = false; 
                    }
                    cleanup = true;
                    selway = 0;
                }
                if ( cleanup )
                {
                    for( size_t way = 0; way < this->m_nways; way++ ) 
                    {
                        for( size_t set = 0; set < this->m_nsets; set++ ) 
                        {
                            if (nline(way,set) == nline(selway,set)) 
                            {
                                cleanup = false;
                                break;
                            }
                        } 
                    } 
                }
            }
        }

        this->vpn(selway,set) = vaddress >> (this->m_page_shift + this->m_sets_shift);
        this->ppn(selway,set) = ( pte << (this->m_page_shift - 10) ) >> (this->m_page_shift - 4);  

        this->cachable(selway,set)   = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        this->executable(selway,set) = (((pte & PTE_X_MASK)  >> PTE_X_SHIFT) == 1) ? true : false;
        this->user(selway,set)       = (((pte & PTE_U_MASK)  >> PTE_U_SHIFT) == 1) ? true : false;
        this->global(selway,set)     = (((pte & PTE_G_MASK)  >> PTE_G_SHIFT) == 1) ? true : false;
        this->writable(selway,set)   = (((pte & PTE_W_MASK)  >> PTE_W_SHIFT) == 1) ? true : false;       
        this->dirty(selway,set)      = (((pte & PTE_D_MASK)  >> PTE_D_SHIFT) == 1) ? true : false; 
        this->et(selway,set)         = PTE_OLD; 
        
        *victim = nline(selway,set);
        nline(selway,set) = line;  

        return cleanup;
    } // end update()

    //////////////////////////////////////////////////////////////
    //  This method verify whether all 16 words of a data cache line 
    //  that are as PTE don't exist in TLB. If is true, a cleanup 
    //  request is actived.
    //////////////////////////////////////////////////////////////
    inline bool cleanupcheck( size_t n_line ) 
    {
        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            for( size_t set = 0; set < this->m_nsets; set++ ) 
            {
                if (nline(way,set) == n_line) 
                {
                    return true;
                }
            } 
        } 
        return false;
    } // end cccheck()
}; // GenericCcTlb

}}

#endif /* SOCLIB_CABA_GENERIC_TLB_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

