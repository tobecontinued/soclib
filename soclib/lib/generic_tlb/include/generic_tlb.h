/*****************************************************************
 * File         : generic_tlb_40bits.h
 * Date         : 02/06/2009
 * Authors      : Alain Greiner, Yang GAO
 * Copyright    : UPMC/LIP6
 * 
 * This file is distributed under the LGPL licence agreement.
 ******************************************************************
 * This object is a generic TLB (Translation Lookaside Buffer) to
 * translate a 40 bits physical address for Hypertransport.
 * It is implemented as a set-associative cache.
 * The replacement algorithm is pseudo-LRU.
 * Each TLB entry has the following format:
 * - bool       valid            (0: unmapped; 1: mapped)
 * - bool       locally accessed 
 * - bool       remotely accessed 
 * - bool       cacheable   (cached)
 * - bool       writable    (writable access bit)
 * - bool       executable  (executable access bit)
 * - bool       user        (access in user mode allowed)
 * - bool       global      (PTE not invalidated by a TLB flush)
 * - bool       dirty       (page has been modified)
 * Additional flag bits for every tlb entry: 
 * - bool       pagesize    (0: 4K page size; 1: 2M page size)
 * - bool       lru         (recently used for replace in TLB)
 * - uint32_t   vpn         (virtual page number)
 * - uint32_t   ppn         (physical page number)
 *
 * valid, lru and global are stored in registers, others are in ram.
 *
 *****************************************************************
 * This file has three constructor parameters:
 * - nways  : number of ways per associative set.
 * - nsets  : number of associative sets.
 * Both nways & nsets must be power of 2 no larger than 64.
 *****************************************************************/

#ifndef SOCLIB_CABA_GENERIC_TLB_H
#define SOCLIB_CABA_GENERIC_TLB_H

#include <inttypes.h>
#include <systemc>
#include <assert.h>
#include "static_assert.h"
#include "arithmetics.h"

namespace soclib { 
namespace caba {

using namespace sc_core;

    // PTE information struct
    typedef struct pte_info_s {
        bool v;    // valid             
        bool t;    // type              
        bool l;    // locally accessed     
        bool r;    // remotely accessed 
        bool c;    // cacheable    
        bool w;    // writable    
        bool x;    // executable  
        bool u;    // user        
        bool g;    // global      
        bool d;    // dirty       
    }pte_info_t;

    enum {  
        PTD_ID2_MASK  = 0x001FF000,
        PAGE_K_MASK   = 0x00000FFF,
        PAGE_M_MASK   = 0x001FFFFF,
    };

    enum {
        PTE_V_MASK = 0x80000000,
        PTE_T_MASK = 0x40000000,
        PTE_L_MASK = 0x20000000,
        PTE_R_MASK = 0x10000000,
        PTE_C_MASK = 0x08000000,
        PTE_W_MASK = 0x04000000,
        PTE_X_MASK = 0x02000000,
        PTE_U_MASK = 0x01000000,
        PTE_G_MASK = 0x00800000,
        PTE_D_MASK = 0x00400000,
    };

    enum {  
        PTE_V_SHIFT = 31,
        PTE_T_SHIFT = 30,
        PTE_L_SHIFT = 29,
        PTE_R_SHIFT = 28,
        PTE_C_SHIFT = 27,
        PTE_W_SHIFT = 26,
        PTE_X_SHIFT = 25,
        PTE_U_SHIFT = 24,
        PTE_G_SHIFT = 23,
        PTE_D_SHIFT = 22,
    };

    enum {  
        PAGE_M_NBITS = 21,
        PAGE_K_NBITS = 12,
        INDEX1_NBITS = 11,
    };

using soclib::common::uint32_log2;

// registers describing a TLB entry state
typedef struct tlb_entry_state {
	sc_signal<bool> v; //valid
	sc_signal<bool> g; //global
	sc_signal<bool> l; //LRU
} tlb_entry_state_t;

// registers describing a tlb set state
typedef class tlb_set_state {
private:
	tlb_entry_state_t *entries;
	size_t nways;
public:
	tlb_set_state()
	{
	}

	~tlb_set_state()
	{
		delete [] entries;
	}

	void
	init(size_t m_nways)
	{
		nways = m_nways;
		entries = new tlb_entry_state_t[nways];
	}

	bool get_valid(size_t way)
	{
		return entries[way].v;
	}
	void set_valid(size_t way)
	{
		assert(entries[way].l == false &&
		    "invalid entry should not have LRU");
		entries[way].v = true;
	}
	void clear_valid(size_t way)
	{
		entries[way].v = false;
		entries[way].l = false;
	}

	bool get_global(size_t way)
	{
		return entries[way].g;
	}
	void set_global(size_t way)
	{
		entries[way].g = true;
	}
	void clear_global(size_t way)
	{
		entries[way].g = false;
	}

	bool get_lru(size_t way)
	{
		return entries[way].l;
	}
	void set_lru(size_t way)
	{
		entries[way].l = true;
		/*
		 * if all LRU bits for the set are true, clear them
		 */
		for (size_t i = 0; i < nways; i++) {
			if (i == way)
				continue;
			if (entries[i].l == false)
				return;
		}
		// all LRU bits are set
		for (size_t i = 0; i < nways; i++) {
			entries[i].l = false;
		}
	}
	void clear_lru(size_t way)
	{
		entries[way].l = false;
	}

	void reset()
	{
		for (size_t i = 0; i < nways; i++) {
			entries[i].l = false;
			entries[i].g = false;
			entries[i].v = false;
		}
	}
} tlb_set_state_t;

template<typename paddr_t>
class GenericTlb
{
protected:
    typedef uint32_t vaddr_t;
    typedef uint32_t data_t;

    const size_t  m_nways;
    const size_t  m_nsets;
    const size_t  m_paddr_nbits;
    const size_t  m_sets_shift;
    const size_t  m_sets_mask;

    data_t  *m_ppn; 
    data_t  *m_vpn;
    bool    *m_pagesize;  
    bool    *m_locacc;
    bool    *m_remacc;
    bool    *m_cacheable;
    bool    *m_writable;
    bool    *m_executable;
    bool    *m_user;
    bool    *m_dirty;

    tlb_set_state_t *m_setsstate;

public:
    // access methods 
    void setlru(size_t way, size_t set)
    { 
        m_setsstate[set].set_lru(way); 
    }

    void clearlru(size_t way, size_t set)
    { 
        m_setsstate[set].clear_lru(way); 
    }

    bool lru(size_t way, size_t set)
    { 
        return m_setsstate[set].get_lru(way); 
    }

    void setvalid(size_t way, size_t set)
    { 
        m_setsstate[set].set_valid(way); 
    }

    void clearvalid(size_t way, size_t set)
    { 
        m_setsstate[set].clear_valid(way); 
    }

    bool valid(size_t way, size_t set)
    { 
        return m_setsstate[set].get_valid(way); 
    }

    void setglobal(size_t way, size_t set)
    { 
        m_setsstate[set].set_global(way); 
    }

    void clearglobal(size_t way, size_t set)
    { 
        m_setsstate[set].clear_global(way); 
    }

    bool global(size_t way, size_t set)
    { 
        return m_setsstate[set].get_global(way); 
    }

    data_t &ppn(size_t way, size_t set)
    { 
        return m_ppn[(way*m_nsets)+set]; 
    }

    data_t &vpn(size_t way, size_t set)
    { 
        return m_vpn[(way*m_nsets)+set]; 
    }

    bool &pagesize(size_t way, size_t set)
    { 
        return m_pagesize[(way*m_nsets)+set]; 
    }

    bool &locacc(size_t way, size_t set)
    { 
        return m_locacc[(way*m_nsets)+set]; 
    }

    bool &remacc(size_t way, size_t set)
    { 
        return m_remacc[(way*m_nsets)+set]; 
    }

    bool &cacheable(size_t way, size_t set)
    { 
        return m_cacheable[(way*m_nsets)+set]; 
    }

    bool &writable(size_t way, size_t set)
    { 
        return m_writable[(way*m_nsets)+set]; 
    }

    bool &executable(size_t way, size_t set)
    { 
        return m_executable[(way*m_nsets)+set]; 
    }

    bool &user(size_t way, size_t set)
    { 
        return m_user[(way*m_nsets)+set]; 
    }

    bool &dirty(size_t way, size_t set)
    { 
        return m_dirty[(way*m_nsets)+set]; 
    }

    //////////////////////////////////////////////////////////////
    // constructor checks parameters, allocates the memory
    // and computes m_page_mask, m_sets_mask and m_sets_shift
    //////////////////////////////////////////////////////////////
    GenericTlb(size_t nways, size_t nsets, size_t paddr_nbits):
    m_nways(nways),
    m_nsets(nsets),
    m_paddr_nbits(paddr_nbits),
    m_sets_shift(uint32_log2(nsets)),
    m_sets_mask((1<<(int)uint32_log2(nsets))-1)
    {
        assert(IS_POW_OF_2(nsets));
        assert(IS_POW_OF_2(nways));
        assert(nsets <= 64);
        assert(nways <= 64);

        if((m_paddr_nbits < 32) || (m_paddr_nbits > 42))
        {
            printf("Error in the genericTlb component\n");
            printf("The physical address parameter must be in the range [32,42]\n");
            exit(1);
        } 

        m_ppn        = new data_t[nways * nsets];
        m_vpn        = new data_t[nways * nsets];
        m_pagesize   = new bool[nways * nsets];
        m_locacc     = new bool[nways * nsets];
        m_remacc     = new bool[nways * nsets];
        m_cacheable   = new bool[nways * nsets];
        m_writable   = new bool[nways * nsets];
        m_executable = new bool[nways * nsets];
        m_user       = new bool[nways * nsets];
        m_dirty      = new bool[nways * nsets];
	m_setsstate = new tlb_set_state_t[nsets];
	for (size_t i = 0; i < nsets; i++)
		m_setsstate[i].init(nways);

    } // end constructor

    ~GenericTlb()
    {
        delete [] m_ppn;
        delete [] m_vpn;
        delete [] m_pagesize;
        delete [] m_locacc;
        delete [] m_remacc;
        delete [] m_cacheable;
        delete [] m_writable;
        delete [] m_executable;
        delete [] m_user;
        delete [] m_dirty;
	delete [] m_setsstate;
    }

    /////////////////////////////////////////////////////////////
    //  This method resets all the TLB entry.
    /////////////////////////////////////////////////////////////
    void reset() 
    {
	for (size_t i = 0; i < m_nsets; i++)
		m_setsstate[i].reset();
    } 

    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical address, 
    //  the pte informations, way and set are returned. 
    /////////////////////////////////////////////////////////
    bool translate(  vaddr_t vaddress,      // virtual address
                            paddr_t *paddress,     // return physique address
                            pte_info_t *pte_info,  // return pte information
                            size_t *tw,            // return way  
                            size_t *ts )           // return set   
    {
        size_t m_set = (vaddress >> PAGE_M_NBITS) & m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & m_sets_mask; 

        for( size_t way = 0; way < m_nways; way++ ) 
        {
            // TLB hit test for 2M page size
            if( valid(way,m_set) && pagesize(way,m_set) &&
               (vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + m_sets_shift))) ) 
            {
                pte_info->l = locacc(way,m_set);
                pte_info->r = remacc(way,m_set);
                pte_info->c = cacheable(way,m_set);
                pte_info->w = writable(way,m_set);
                pte_info->x = executable(way,m_set);
                pte_info->u = user(way,m_set);
                pte_info->g = global(way,m_set);
                pte_info->d = dirty(way,m_set);
                *tw = way;
                *ts = m_set;
                *paddress = (paddr_t)((paddr_t)ppn(way,m_set) << PAGE_M_NBITS) | (paddr_t)(vaddress & PAGE_M_MASK);
		setlru(way, m_set);
                return true;
            }

            // TLB hit test for 4K page size
            if( valid(way,k_set) && !pagesize(way,k_set) &&
               (vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + m_sets_shift))) ) 
            {  
                pte_info->l = locacc(way,k_set);
                pte_info->r = remacc(way,k_set);
                pte_info->c = cacheable(way,k_set);
                pte_info->w = writable(way,k_set);
                pte_info->x = executable(way,k_set);
                pte_info->u = user(way,k_set);
                pte_info->g = global(way,k_set);
                pte_info->d = dirty(way,k_set);
                *tw = way;
                *ts = k_set;
                *paddress = (paddr_t)((paddr_t)ppn(way,k_set) << PAGE_K_NBITS) | (paddr_t)(vaddress & PAGE_K_MASK);
		setlru(way, k_set);
                return true;   
            } 
        } 
        return false;
    } // end translate()

    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical page number is returned. 
    /////////////////////////////////////////////////////////
    bool translate(vaddr_t vaddress, paddr_t *paddress) 
    {
        size_t m_set = (vaddress >> PAGE_M_NBITS) & m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & m_sets_mask; 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            // TLB hit test for 2M page size
            if( valid(way,m_set) && pagesize(way,m_set) &&
               (vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + m_sets_shift))) ) 
            {
                *paddress = (paddr_t)((paddr_t)ppn(way,m_set) << PAGE_M_NBITS) | (paddr_t)(vaddress & PAGE_M_MASK);
		setlru(way, m_set);
                return true;
            }

            // TLB hit test for 4K page size
            if( valid(way,k_set) && !pagesize(way,k_set) &&
               (vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + m_sets_shift))) ) 
            {  
                *paddress = (paddr_t)((paddr_t)ppn(way,k_set) << PAGE_K_NBITS) | (paddr_t)(vaddress & PAGE_K_MASK);
		setlru(way, k_set);
                return true;   
            } 
        }
        return false;
    } // end translate()
/*
    /////////////////////////////////////////////////////////
    //  This method returns "false" in case of MISS
    //  In case of HIT, the physical page number is returned. 
    /////////////////////////////////////////////////////////
    bool translate(vaddr_t vaddress, paddr_t *paddress, bool *cached) 
    {
        size_t m_set = (vaddress >> PAGE_M_NBITS) & m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & m_sets_mask; 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            // TLB hit test for 2M page size
            if( valid(way,m_set) && pagesize(way,m_set) &&
               (vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + m_sets_shift))) ) 
            {
                *paddress = (paddr_t)((paddr_t)ppn(way,m_set) << PAGE_M_NBITS) | (paddr_t)(vaddress & PAGE_M_MASK);
                *cached = cacheable(way,m_set);
		setlru(way, m_set);
                return true;
            }

            // TLB hit test for 4K page size
            if( valid(way,k_set) && !pagesize(way,k_set) &&
               (vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + m_sets_shift))) ) 
            {  
                *paddress = (paddr_t)((paddr_t)ppn(way,k_set) << PAGE_K_NBITS) | (paddr_t)(vaddress & PAGE_K_MASK);
                *cached = cacheable(way,k_set);
		setlru(way, k_set);
                return true;   
            } 
        }
        return false;
    } // end translate()
*/
    /////////////////////////////////////////////////////////////
    //  This method resets all VALID bits in one cycle,
    //  when the the argument is true.
    //  Locked descriptors are preserved when it is false.
    /////////////////////////////////////////////////////////////
    void flush(bool all) 
    {
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            for(size_t set = 0; set < m_nsets; set++) 
            {
		if (!valid (way,set))
			continue;
                if(global(way,set)) 
                {
                    if(all) {
			clearvalid(way,set); // forced reset, the locked page invalid too
		    }
                } 
                else 
                {
                    clearvalid(way,set); // not forced reset, the locked page conserve  
                }
            } 
        } 
    } // end flush

    uint32_t getpte(size_t way,size_t set)   
    {
        data_t pte = 0; 
        if ( pagesize(way,set) )    // 2M page size
        {
           pte = (data_t)ppn(way,set) | (PTE_V_MASK & (valid(way,set) << PTE_V_SHIFT));
    
           if ( locacc(way,set) )
               pte = pte | PTE_L_MASK;
           if ( remacc(way,set) )
               pte = pte | PTE_R_MASK;
           if ( cacheable(way,set) )
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
        }
        else    // 4K page size
        {
           pte = (data_t)(PTE_V_MASK & (valid(way,set) << PTE_V_SHIFT));
    
           if ( locacc(way,set) )
               pte = pte | PTE_L_MASK;
           if ( remacc(way,set) )
               pte = pte | PTE_R_MASK;
           if ( cacheable(way,set) )
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
        }
        return pte;
    } // end getpte()

    /////////////////////////////////////////////////////////////
    //  This method return the index of the least recently
    //  used descriptor in the associative set.
    /////////////////////////////////////////////////////////////
    size_t getlru(size_t set)
    {
        // check val bit firstly, replace the invalid PTE
        for(size_t way = 0; way < m_nways; way++) 
        {
            if( !valid(way,set) ) 
            {
                return way;
            }
        } 

        // then we check bit lock, remplace and old way which is not global
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if( !global(way,set) && !lru(way,set) ) 
            {
                return way;
            } 
        }
	
	// finally remplace the first old way
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            if( !lru(way,set) ) 
            {
                return way;
            } 
        }

        assert(0 && "all TLB ways can't be new at the same time");
	return 0; /* avoid gcc warning */
    } // end getlru()

    /////////////////////////////////////////////////////////////
    //  This method writes a new 2M page size entry in the TLB.
    /////////////////////////////////////////////////////////////
    void update(data_t pte, vaddr_t vaddress) 
    {
        size_t set = (vaddress >> PAGE_M_NBITS) & m_sets_mask; 
        size_t way = getlru(set);
      
        vpn(way,set) = vaddress >> (PAGE_M_NBITS + m_sets_shift);
        ppn(way,set) = pte & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1);

        setvalid(way,set);
        pagesize(way,set)   = true;
        setlru(way,set);
        locacc(way,set)     = (((pte & PTE_L_MASK) >> PTE_L_SHIFT) == 1) ? true : false;
        remacc(way,set)     = (((pte & PTE_R_MASK) >> PTE_R_SHIFT) == 1) ? true : false;
        cacheable(way,set)  = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        writable(way,set)   = (((pte & PTE_W_MASK) >> PTE_W_SHIFT) == 1) ? true : false;       
        executable(way,set) = (((pte & PTE_X_MASK) >> PTE_X_SHIFT) == 1) ? true : false;
        user(way,set)       = (((pte & PTE_U_MASK) >> PTE_U_SHIFT) == 1) ? true : false;
	if (((pte & PTE_G_MASK) >> PTE_G_SHIFT) == 1)
		setglobal(way,set);
	else
		clearglobal(way,set);

        dirty(way,set)      = (((pte & PTE_D_MASK) >> PTE_D_SHIFT) == 1) ? true : false; 
    } // end update()

    /////////////////////////////////////////////////////////////
    //  This method writes a new 4K page size entry in the TLB.
    /////////////////////////////////////////////////////////////
    void update(data_t pte, data_t ppn2 , vaddr_t vaddress) 
    {
        size_t set = (vaddress >> PAGE_K_NBITS) & m_sets_mask; 
        size_t way = getlru(set);
      
        vpn(way,set) = vaddress >> (PAGE_K_NBITS + m_sets_shift);
        ppn(way,set) = ppn2 & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1);  

        setvalid(way,set);
        pagesize(way,set)   = false;
        setlru(way,set);
        locacc(way,set)     = (((pte & PTE_L_MASK) >> PTE_L_SHIFT) == 1) ? true : false;
        remacc(way,set)     = (((pte & PTE_R_MASK) >> PTE_R_SHIFT) == 1) ? true : false;
        cacheable(way,set)  = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        writable(way,set)   = (((pte & PTE_W_MASK) >> PTE_W_SHIFT) == 1) ? true : false;       
        executable(way,set) = (((pte & PTE_X_MASK) >> PTE_X_SHIFT) == 1) ? true : false;
        user(way,set)       = (((pte & PTE_U_MASK) >> PTE_U_SHIFT) == 1) ? true : false;
	if (((pte & PTE_G_MASK) >> PTE_G_SHIFT) == 1)
		setglobal(way,set);
	else
		clearglobal(way,set);
        dirty(way,set)      = (((pte & PTE_D_MASK) >> PTE_D_SHIFT) == 1) ? true : false; 
    } // end update()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    bool inval(vaddr_t vaddress) 
    {
        size_t m_set = (vaddress >> PAGE_M_NBITS) & m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & m_sets_mask; 
        for( size_t way = 0; way < m_nways; way++ ) 
        {
            // TLB hit test for 2M page size
            if( valid(way,m_set) && pagesize(way,m_set) &&
               (vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + m_sets_shift))) ) 
            {
                clearvalid(way,m_set);
                return true;
            }

            // TLB hit test for 4K page size
            if( valid(way,k_set) && !pagesize(way,k_set) &&
               (vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + m_sets_shift))) ) 
            {  
                clearvalid(way,k_set);
                return true;   
            } 
        } 
        return false;
    } // end translate()

    /////////////////////////////////////////////////////////////
    //  This method writes a new entry in the TLB.
    /////////////////////////////////////////////////////////////
    void setdirty(size_t way, size_t set)
    {
        dirty(way,set) = true;
    } // end setdirty()

    /////////////////////////////////////////////////////////////
    //  This method return the page size. 
    /////////////////////////////////////////////////////////////
    bool getpagesize(size_t way, size_t set)
    {
        return pagesize(way,set);
    }

}; // GenericTlb

template<typename paddr_t>
class GenericCcTlb : public GenericTlb<paddr_t>
{
public:
    typedef uint32_t vaddr_t;
    typedef uint32_t data_t;

    paddr_t  *m_nline; 

    // access methods 
    paddr_t &nline(size_t way, size_t set)
    { 
        return m_nline[(way*this->m_nsets)+set]; 
    }

public:
    //////////////////////////////////////////////////////////////
    // constructor checks parameters, allocates the memory
    // and computes m_page_mask, m_sets_mask and m_sets_shift
    //////////////////////////////////////////////////////////////
    GenericCcTlb(size_t nways, size_t nsets, size_t paddr_nbits):GenericTlb<paddr_t>::GenericTlb(nways, nsets, paddr_nbits)
    {
        m_nline = new paddr_t[this->m_nways * this->m_nsets];
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
    bool cctranslate( vaddr_t vaddress,      // virtual address
                             paddr_t *paddress,     // return physique address
                             pte_info_t *pte_info,  // return pte information
                             paddr_t *victim_index, // return nline
                             size_t *tw,            // return way  
                             size_t *ts )           // return set   
    {
        size_t m_set = (vaddress >> PAGE_M_NBITS) & this->m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & this->m_sets_mask; 

        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            // TLB hit test for 2M page size
            if( this->valid(way,m_set) && this->pagesize(way,m_set) &&
               (this->vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + this->m_sets_shift))) ) 
            {
                pte_info->l = this->locacc(way,m_set);
                pte_info->r = this->remacc(way,m_set);
                pte_info->c = this->cacheable(way,m_set);
                pte_info->w = this->writable(way,m_set);
                pte_info->x = this->executable(way,m_set);
                pte_info->u = this->user(way,m_set);
                pte_info->g = this->global(way,m_set);
                pte_info->d = this->dirty(way,m_set);
                *victim_index = nline(way,m_set);
                *tw = way;
                *ts = m_set;
                *paddress = (paddr_t)((paddr_t)this->ppn(way,m_set) << PAGE_M_NBITS) | (paddr_t)(vaddress & PAGE_M_MASK);
		this->setlru(way, m_set);
                return true;
            }

            // TLB hit test for 4K page size
            if( this->valid(way,k_set) && !(this->pagesize(way,k_set)) &&
               (this->vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + this->m_sets_shift))) ) 
            {  
                pte_info->l = this->locacc(way,k_set);
                pte_info->r = this->remacc(way,k_set);
                pte_info->c = this->cacheable(way,k_set);
                pte_info->w = this->writable(way,k_set);
                pte_info->x = this->executable(way,k_set);
                pte_info->u = this->user(way,k_set);
                pte_info->g = this->global(way,k_set);
                pte_info->d = this->dirty(way,k_set);
                *victim_index = nline(way,k_set);
                *tw = way;
                *ts = k_set;
                *paddress = (paddr_t)((paddr_t)this->ppn(way,k_set) << PAGE_K_NBITS) | (paddr_t)(vaddress & PAGE_K_MASK);
		this->setlru(way, k_set);
                return true;   
            } 
        } 
        return false;
    } // end translate()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry for cc_vcache2
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    bool inval(vaddr_t vaddress, paddr_t* victim)
    {
        paddr_t vic_nline = 0;
        size_t m_set = (vaddress >> PAGE_M_NBITS) & this->m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & this->m_sets_mask; 

        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            if( this->valid(way,m_set) && this->pagesize(way,m_set) &&
                this->vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + this->m_sets_shift))) 
            {
                vic_nline = nline(way,m_set);
                this->clearvalid(way,m_set);
                break;
            } 

            if( this->valid(way,k_set) && !(this->pagesize(way,k_set)) &&
                this->vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + this->m_sets_shift))) 
            {
                vic_nline = nline(way,k_set);
                this->clearvalid(way,k_set);
                break;
            }
            if ( way == (this->m_nways-1)) return false; 
        } 

        *victim = vic_nline;
        // verify whether need a cleanup
        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            for( size_t set = 0; set < this->m_nsets; set++ ) 
            {
                if( (nline(way,set) == vic_nline) && (this->valid(way,set)) ) 
                {
                    return false;
                }
            } 
        } 
        return true;
    } // end inval()

    //////////////////////////////////////////////////////////////
    //  This method invalidates a TLB entry for cc_vcache
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    bool inval1(vaddr_t vaddress, paddr_t* victim)
    {
        paddr_t vic_nline = 0;
        size_t m_set = (vaddress >> PAGE_M_NBITS) & this->m_sets_mask; 
        size_t k_set = (vaddress >> PAGE_K_NBITS) & this->m_sets_mask; 

        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            if( this->valid(way,m_set) && this->pagesize(way,m_set) &&
                this->vpn(way,m_set) == (vaddress >> (PAGE_M_NBITS + this->m_sets_shift))) 
            {
                vic_nline = nline(way,m_set);
                this->clearvalid(way,m_set);
                break;
            } 

            if( this->valid(way,k_set) && !(this->pagesize(way,k_set)) &&
                this->vpn(way,k_set) == (vaddress >> (PAGE_K_NBITS + this->m_sets_shift))) 
            {
                vic_nline = nline(way,k_set);
                this->clearvalid(way,k_set);
                break;
            }
            if ( way == (this->m_nways-1)) return false; 
        } 

        *victim = vic_nline;
        return true;
    } // end inval1()

    //////////////////////////////////////////////////////////////
    //  This method coherence invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    void ccinval(size_t invway, size_t invset)
    {
        this->clearvalid(invway,invset);
    } // end ccinval()

    //////////////////////////////////////////////////////////////
    //  This method coherence invalidates a TLB entry
    //  identified by the virtual page number.
    //////////////////////////////////////////////////////////////
    size_t getnline(size_t way, size_t set)
    {
        return nline(way,set);
    } // end getnline()

    /////////////////////////////////////////////////////////////
    //  This method for cc_vcache2 is used for context switch.  
    //  In this case, allTLB entries should be invalidated except 
    //  global entries.All the entries that are in the same data 
    //  cache line have the same NLINE. Therefore only all the  
    //  entries of one datacache line are not global, it send a  
    //  cleanup request todata cache. The return value indicates 
    //  whether need a cleanupand cleanup_nline contains the NLINE 
    //  that is for cleanup.   
    /////////////////////////////////////////////////////////////
    bool checkcleanup(size_t nway, size_t nset, paddr_t* cleanup_nline)
    {
        bool cleanup = false;
        bool isglobal = false;
        if ( this->valid(nway,nset) )
        {
            size_t inval_line = nline(nway,nset);
            for (size_t start = (nway*this->m_nsets+nset); start < this->m_nways*this->m_nsets; start++)
            {
                if ( this->valid(start/this->m_nsets,start%this->m_nsets) && (inval_line == nline(start/this->m_nsets,start%this->m_nsets)) )
                {
                    if (!this->global(start/this->m_nsets,start%this->m_nsets))
                    {
                        this->clearvalid(start/this->m_nsets,start%this->m_nsets);
                    }
                    else
                    {
                        isglobal = true;
                    }
                }
            }

/*
            for ( size_t way = nway; way < this->m_nways; way++ )
            {            
                for ( size_t set = nset; set < this->m_nsets; set++ )            
                {
                    if ( this->valid(way,set) && (inval_line == nline(way,set)) )
                    {
                        if (!this->global(way,set))
                        {
                            this->clearvalid(way,set);
                        }
                        else
                        {
                            isglobal = true;
                        }
                    }
                }
            }
*/
            cleanup = !isglobal;
            if(cleanup)
                *cleanup_nline = inval_line;
        }
        return cleanup;
    } // end checkcleanup

    /////////////////////////////////////////////////////////////
    //  This method for cc_vcache is used for context switch.  
    //  In this case, allTLB entries should be invalidated except 
    //  global entries.All the entries that are in the same data 
    //  cache line have the same NLINE. Therefore only all the  
    //  entries of one datacache line are not global, it send a  
    //  cleanup request todata cache. The return value indicates 
    //  whether need a cleanupand cleanup_nline contains the NLINE 
    //  that is for cleanup.   
    /////////////////////////////////////////////////////////////
    bool checkcleanup1(size_t nway, size_t nset, paddr_t* cleanup_nline)
    {
        bool cleanup = false;
        bool isglobal = false;
        if ( this->valid(nway,nset) )
        {
            size_t inval_line = nline(nway,nset);
            if (!this->global(nway,nset))
            {
                this->clearvalid(nway,nset);
            }
            else
            {
                isglobal = true;
            }

            cleanup = !isglobal;
            if(cleanup)
                *cleanup_nline = inval_line;
        }
        return cleanup;
    } // end checkcleanup1

    //////////////////////////////////////////////////////////////////////////
    //  This method writes a new 2M page size entry in the TLB for cc_vcache2.
    //////////////////////////////////////////////////////////////////////////
    bool update(data_t pte, vaddr_t vaddress, paddr_t line, paddr_t* victim ) 
    {
        size_t set = (vaddress >> PAGE_M_NBITS) & this->m_sets_mask;
        size_t selway = this->getlru(set);
        bool cleanup = this->valid(selway,set);

	if (cleanup) {
            for( size_t way = 0; way < this->m_nways; way++ ) 
            {
                for( size_t set = 0; set < this->m_nsets; set++ ) 
                {
                    if ((nline(way,set) == nline(selway,set)) && this->valid(way,set)) 
                    {
                        cleanup = false;
                        break;
                    }
                } 
            } 
        }

        this->vpn(selway,set) = vaddress >> (PAGE_M_NBITS + this->m_sets_shift);
        this->ppn(selway,set) = pte & ((1<<(this->m_paddr_nbits - PAGE_M_NBITS))-1); 
 
        this->setvalid(selway,set);
        this->pagesize(selway,set)   = true;
        this->setlru(selway,set);

        this->locacc(selway,set)     = (((pte & PTE_L_MASK) >> PTE_L_SHIFT) == 1) ? true : false;
        this->remacc(selway,set)     = (((pte & PTE_R_MASK) >> PTE_R_SHIFT) == 1) ? true : false;
        this->cacheable(selway,set)  = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        this->writable(selway,set)   = (((pte & PTE_W_MASK) >> PTE_W_SHIFT) == 1) ? true : false;       
        this->executable(selway,set) = (((pte & PTE_X_MASK) >> PTE_X_SHIFT) == 1) ? true : false;
        this->user(selway,set)       = (((pte & PTE_U_MASK) >> PTE_U_SHIFT) == 1) ? true : false;
	if (((pte & PTE_G_MASK) >> PTE_G_SHIFT) == 1)
		this->setglobal(selway,set);
	else
		this->clearglobal(selway,set);
        this->dirty(selway,set)      = (((pte & PTE_D_MASK) >> PTE_D_SHIFT) == 1) ? true : false; 
        
        *victim = nline(selway,set);
        nline(selway,set) = line;  
        return cleanup;
    } // end update()

    //////////////////////////////////////////////////////////////////////////
    //  This method writes a new 2M page size entry in the TLB for cc_vcache.
    //////////////////////////////////////////////////////////////////////////
    bool update1(data_t pte, vaddr_t vaddress, paddr_t line, paddr_t* victim ) 
    {
        size_t set = (vaddress >> PAGE_M_NBITS) & this->m_sets_mask;
        size_t selway = this->getlru(set);
        bool cleanup = this->valid(selway,set);

        this->vpn(selway,set) = vaddress >> (PAGE_M_NBITS + this->m_sets_shift);
        this->ppn(selway,set) = pte & ((1<<(this->m_paddr_nbits - PAGE_M_NBITS))-1); 
 
        this->setvalid(selway,set);
        this->pagesize(selway,set)   = true;
        this->setlru(selway,set);

        this->locacc(selway,set)     = (((pte & PTE_L_MASK) >> PTE_L_SHIFT) == 1) ? true : false;
        this->remacc(selway,set)     = (((pte & PTE_R_MASK) >> PTE_R_SHIFT) == 1) ? true : false;
        this->cacheable(selway,set)  = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        this->writable(selway,set)   = (((pte & PTE_W_MASK) >> PTE_W_SHIFT) == 1) ? true : false;       
        this->executable(selway,set) = (((pte & PTE_X_MASK) >> PTE_X_SHIFT) == 1) ? true : false;
        this->user(selway,set)       = (((pte & PTE_U_MASK) >> PTE_U_SHIFT) == 1) ? true : false;
	if (((pte & PTE_G_MASK) >> PTE_G_SHIFT) == 1)
		this->setglobal(selway,set);
	else
		this->clearglobal(selway,set);
        this->dirty(selway,set)      = (((pte & PTE_D_MASK) >> PTE_D_SHIFT) == 1) ? true : false; 
        
        *victim = nline(selway,set);
        nline(selway,set) = line;  
        return cleanup;
    } // end update1()

    //////////////////////////////////////////////////////////////////////////
    //  This method writes a new 4K page size entry in the TLB for cc_vcache2.
    //////////////////////////////////////////////////////////////////////////
    bool update(data_t pte, data_t ppn2, vaddr_t vaddress, paddr_t line, paddr_t* victim ) 
    {
        size_t set = (vaddress >> PAGE_K_NBITS) & this->m_sets_mask;
        size_t selway = this->getlru(set);
        bool cleanup = this->valid(selway,set);

        // check if we really need to do a cleanup
	if (cleanup) {
            for( size_t way = 0; way < this->m_nways; way++ ) 
            {
                for( size_t set = 0; set < this->m_nsets; set++ ) 
                {
                    if ((nline(way,set) == nline(selway,set)) && this->valid(way,set)) 
                    {
                        cleanup = false;
                        break;
                    }
                } 
            } 
	}

        this->vpn(selway,set) = vaddress >> (PAGE_K_NBITS + this->m_sets_shift);
        this->ppn(selway,set) = ppn2 & ((1<<(this->m_paddr_nbits - PAGE_K_NBITS))-1);  
        this->setvalid(selway,set);
        this->pagesize(selway,set)   = false;
        this->setlru(selway,set);

        this->locacc(selway,set)     = (((pte & PTE_L_MASK) >> PTE_L_SHIFT) == 1) ? true : false;
        this->remacc(selway,set)     = (((pte & PTE_R_MASK) >> PTE_R_SHIFT) == 1) ? true : false;
        this->cacheable(selway,set)  = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        this->writable(selway,set)   = (((pte & PTE_W_MASK) >> PTE_W_SHIFT) == 1) ? true : false;       
        this->executable(selway,set) = (((pte & PTE_X_MASK) >> PTE_X_SHIFT) == 1) ? true : false;
        this->user(selway,set)       = (((pte & PTE_U_MASK) >> PTE_U_SHIFT) == 1) ? true : false;
	if (((pte & PTE_G_MASK) >> PTE_G_SHIFT) == 1)
		this->setglobal(selway,set);
	else
		this->clearglobal(selway,set);
		
        this->dirty(selway,set)      = (((pte & PTE_D_MASK) >> PTE_D_SHIFT) == 1) ? true : false; 
        
        *victim = nline(selway,set);
        nline(selway,set) = line; 

        return cleanup;
    } // end update()

    //////////////////////////////////////////////////////////////////////////
    //  This method writes a new 4K page size entry in the TLB for cc_vcache.
    //////////////////////////////////////////////////////////////////////////
    bool update1(data_t pte, data_t ppn2, vaddr_t vaddress, paddr_t line, paddr_t* victim ) 
    {
        size_t set = (vaddress >> PAGE_K_NBITS) & this->m_sets_mask;
        size_t selway = this->getlru(set);
        bool cleanup = this->valid(selway,set);

        this->vpn(selway,set) = vaddress >> (PAGE_K_NBITS + this->m_sets_shift);
        this->ppn(selway,set) = ppn2 & ((1<<(this->m_paddr_nbits - PAGE_K_NBITS))-1);  
        this->setvalid(selway,set);
        this->pagesize(selway,set)   = false;
        this->setlru(selway,set);

        this->locacc(selway,set)     = (((pte & PTE_L_MASK) >> PTE_L_SHIFT) == 1) ? true : false;
        this->remacc(selway,set)     = (((pte & PTE_R_MASK) >> PTE_R_SHIFT) == 1) ? true : false;
        this->cacheable(selway,set)  = (((pte & PTE_C_MASK) >> PTE_C_SHIFT) == 1) ? true : false;
        this->writable(selway,set)   = (((pte & PTE_W_MASK) >> PTE_W_SHIFT) == 1) ? true : false;       
        this->executable(selway,set) = (((pte & PTE_X_MASK) >> PTE_X_SHIFT) == 1) ? true : false;
        this->user(selway,set)       = (((pte & PTE_U_MASK) >> PTE_U_SHIFT) == 1) ? true : false;
	if (((pte & PTE_G_MASK) >> PTE_G_SHIFT) == 1)
		this->setglobal(selway,set);
	else
		this->clearglobal(selway,set);
        this->dirty(selway,set)      = (((pte & PTE_D_MASK) >> PTE_D_SHIFT) == 1) ? true : false; 
        
        *victim = nline(selway,set);
        nline(selway,set) = line; 

        return cleanup;
    } // end update1()

    //////////////////////////////////////////////////////////////
    //  This method verify whether all 16 words of a data cache line 
    //  that are as PTE don't exist in TLB. If is true, a cleanup 
    //  request is actived.
    //////////////////////////////////////////////////////////////
    bool cleanupcheck( paddr_t n_line ) 
    {
        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            for( size_t set = 0; set < this->m_nsets; set++ ) 
            {
                if ( (nline(way,set) == n_line) && this->valid(way,set) ) 
                {
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
    bool cccheck( paddr_t n_line, 
                         size_t start_way, size_t start_set, 
                         size_t* n_way, size_t* n_set,
                         bool* end )
    {
        for ( size_t start = start_way*(this->m_nsets)+start_set; start < this->m_nways*this->m_nsets; start++ )
        {
            if (( nline((start/this->m_nsets),(start%this->m_nsets)) == n_line ) && this->valid((start/this->m_nsets),(start%this->m_nsets))) 
            {
                *n_way = start/this->m_nsets;
                *n_set = start%this->m_nsets;
                if ( ((start/this->m_nsets) == (this->m_nways-1)) && ((start%this->m_nsets) == (this->m_nsets-1)) )
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
        *end = true;
        return false;
/*
        for( size_t way = 0; way < this->m_nways; way++ ) 
        {
            for( size_t set = 0; set < this->m_nsets; set++ ) 
            {
                if (( nline(way,set) == n_line ) && this->valid(way,set)) 
                {
                    *n_way = way;
                    *n_set = set;
                    if ( (way == (this->m_nways-1)) && (set == (this->m_nsets-1)) )
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
        *end = true;
        return false;
*/
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



