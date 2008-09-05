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
        PTD_PTP_MASK  = 0x00FFFFFF,
        PTD_ID2_MASK  = 0x003FF000,
        PPN_M_MASK    = 0x000FFFC0,
        OFFSET_K_MASK = 0x00000FFF,
        OFFSET_M_MASK = 0x003FFFFF,
    };

    enum {  
        PTE_ET_SHIFT = 30,
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
    inline uint32_t &ppn(size_t i, size_t j)
        { return m_ppn[(i*m_nsets)+j]; }

    inline uint32_t &vpn(size_t i, size_t j)
        { return m_vpn[(i*m_nsets)+j]; }

    inline bool &lru(size_t i, size_t j)
        { return m_lru[(i*m_nsets)+j]; }

    inline size_t &et(size_t i, size_t j)
        { return m_et[(i*m_nsets)+j]; }

    inline bool &cachable(size_t i, size_t j)
        { return m_cachable[(i*m_nsets)+j]; }

    inline bool &writable(size_t i, size_t j)
        { return m_writable[(i*m_nsets)+j]; }

    inline bool &executable(size_t i, size_t j)
        { return m_executable[(i*m_nsets)+j]; }

    inline bool &user(size_t i, size_t j)
        { return m_user[(i*m_nsets)+j]; }

    inline bool &global(size_t i, size_t j)
        { return m_global[(i*m_nsets)+j]; }

    inline bool &dirty(size_t i, size_t j)
        { return m_dirty[(i*m_nsets)+j]; }

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
        //m_sets_mask = ((1<<(int)uint32_log2(nsets))-1) << m_sets_shift; // this for debug TLB
        m_sets_mask = (1<<(int)uint32_log2(nsets))-1;

        assert(IS_POW_OF_2(nsets));
        assert(IS_POW_OF_2(nways));
        assert(nsets <= 64);
        assert(nways <= 64);

        if((nbits < 10) || (nbits > 28) ){
            printf("Error in the genericTlb component\n");
            printf("The nbits parameter must be in the range [10,28]\n");
            exit(1);
        } else {
            m_page_mask     = 0xFFFFFFFF >> (32 - nbits);
            m_page_shift    = nbits;    // nomber of page offset bits 
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

/////////////////////////////////////////////////////////////
//  This method resets all the TLB entry.
/////////////////////////////////////////////////////////////
    inline void reset() 
    {
        memset(m_et, UNMAPPED, sizeof(*m_et)*m_nways*m_nsets);
    } // end reset

/////////////////////////////////////////////////////////
//  This method returns "false" in case of MISS
//  In case of HIT, the physical address, 
//  the pte informations, way and set are returned. 
/////////////////////////////////////////////////////////
    inline bool translate(  uint32_t vaddress,      // virtual address
                            addr_t *paddress,     // return physique address
                            pte_info_t *pte_info,   // return pte information
                            size_t *tw,             // return way  
                            size_t *ts )            // return set   
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask; 
        for(size_t way = 0; way < m_nways; way++) {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift)))) { // TLB hit 
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
            } // end if
        } // end for
        return false;
    } // end translate()

/////////////////////////////////////////////////////////
//  This method returns "false" in case of MISS
//  In case of HIT, the physical page number is returned. 
/////////////////////////////////////////////////////////
    inline bool translate(uint32_t vaddress, addr_t *paddress) 
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask; 
        for(size_t way = 0; way < m_nways; way++) {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift)))) { // TLB hit 
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
        for(size_t way = 0; way < m_nways; way++) {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift)))) { // TLB hit 
                 return true;   
            } // end if
        } // end for
        return false;
    } // end translate()

/////////////////////////////////////////////////////////////
//  This method resets all VALID bits in one cycle,
//  when the the argument is true.
//  Locked descriptors are preserved when it is false.
/////////////////////////////////////////////////////////////
    inline void flush(bool all) 
    {
        for(size_t way = 0; way < m_nways; way++) {
            for(size_t set = 0; set < m_nsets; set++) {
                if(global(way,set)) {
                    if(all) m_et[way*m_nsets+set] = UNMAPPED;  // forced reset, the locked page invalid too
                } else {
                    m_et[way*m_nsets+set] = UNMAPPED;  // not forced reset, the locked page conserve
                }
            } // end for way
        } // end for set
    } // end flush

//////////////////////////////////////////////////////////////
//  This method modifies all LRU bits of a given set :
//  The LRU bit of the accessed descriptor is set to true,
//  all other LRU bits in the set are reset to false.
/////////////////////////////////////////////////////////////
    inline void setlru(size_t way,size_t set)   
    {
        m_lru[way*m_nsets+set] = true;  // set bit lru for recently used
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
        for(size_t way = 0; way < m_nways; way++) {
            if(et(way,set) == UNMAPPED) {
                return way;
            }
        } 

        // then we check bit lock ... 
        for(size_t way = 0; way < m_nways; way++) {
            if(!global(way,set)) {
                defaul = way;
                if(!lru(way,set)) return way;
            } // end if
        } // end for

        return defaul;
    } // end getlru()

/////////////////////////////////////////////////////////////
//  This method writes a new entry in the TLB.
/////////////////////////////////////////////////////////////
    inline void update(size_t pte, size_t vaddress) 
    {
        
        size_t set = (vaddress >> m_page_shift) & m_sets_mask;
        size_t way = getlru(set);

        for(size_t i = 0; i < m_nways; i++) {
            if (vpn(i,set) == (vaddress >> (m_page_shift + m_sets_shift))) {
                way = i;
                break;
            }
        }
        
        m_vpn[way*m_nsets+set] = vaddress >> (m_page_shift + m_sets_shift);
        //m_ppn[way*m_nsets+set] = (addr_t)((addr_t)pte << m_page_shift) >> m_page_shift;  
        m_ppn[way*m_nsets+set] = ( pte << (m_page_shift - 10) ) >> (m_page_shift - 4);  

        m_cachable[way*m_nsets+set] = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        m_executable[way*m_nsets+set] = (((pte & PTE_X_MASK)  >> PTE_X_SHIFT) == 1) ? true : false;
        m_user[way*m_nsets+set]       = (((pte & PTE_U_MASK)  >> PTE_U_SHIFT) == 1) ? true : false;
        m_global[way*m_nsets+set]     = (((pte & PTE_G_MASK)  >> PTE_G_SHIFT) == 1) ? true : false;
        m_writable[way*m_nsets+set]   = (((pte & PTE_W_MASK)  >> PTE_W_SHIFT) == 1) ? true : false;       
        m_dirty[way*m_nsets+set]      = (((pte & PTE_D_MASK)  >> PTE_D_SHIFT) == 1) ? true : false; 
        m_et[way*m_nsets+set]         = PTE_OLD; 

    } // end update()

//////////////////////////////////////////////////////////////
//  This method invalidates a TLB entry
//  identified by the virtual page number.
//////////////////////////////////////////////////////////////
    inline void inval(uint32_t vaddress)
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask;
        for(size_t way = 0; way < m_nways; way++) {
            if(vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift))) {
                if ( !global(way,set) ) m_et[way*m_nsets+set] = UNMAPPED;
            } // end if
        } // end for way
    } // end inval()

/////////////////////////////////////////////////////////////
//  This method writes a new entry in the TLB.
/////////////////////////////////////////////////////////////
    inline void setdirty(size_t way, size_t set)
    {
        m_dirty[way*m_nsets+set] = true;

    } // end setdirty()

/////////////////////////////////////////////////////////////
//  This method get physique page number in the TLB.
/////////////////////////////////////////////////////////////
    inline uint32_t getppn(uint32_t vaddress)
    {
        size_t set = (vaddress >> m_page_shift) & m_sets_mask; 
        for(size_t way = 0; way < m_nways; way++) {
            if((et(way,set) > 1) &&     // PTE
               (vpn(way,set) == (vaddress >> (m_page_shift + m_sets_shift)))) { // TLB hit 
                return m_ppn[way*m_nsets+set];   
            } // end if
        } // end for
        return 0; 

    } // end getppn()

}; // GenericTlb

}}

#endif /* SOCLIB_CABA_GENERIC_TLB_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

