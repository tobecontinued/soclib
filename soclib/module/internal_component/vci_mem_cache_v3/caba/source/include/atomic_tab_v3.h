#ifndef ATOMIC_TAB_V3_H_
#define ATOMIC_TAB_V3_H_

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include "arithmetics.h"



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/*                  The atomic access tab                             */
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

class AtomicTab{
    typedef uint32_t size_t;
    typedef sc_dt::sc_uint<40> addr_t;

private:
    size_t size_tab;                            // The size of the tab
    std::vector<addr_t> addr_tab;               // Address entries
    std::vector<bool>   valid_tab;              // Valid entries

public:

    AtomicTab()
	: addr_tab(0),
	  valid_tab(0)
	{
    	size_tab=0;
    }

    AtomicTab(size_t size_tab_i)
	: addr_tab(size_tab_i),
	  valid_tab(size_tab_i)
	{
	    size_tab=size_tab_i;
    }


    /////////////////////////////////////////////////////////////////////
    /* The size() function returns the size of the tab */
    /////////////////////////////////////////////////////////////////////
    const size_t size(){
	    return size_tab;
    }


    /////////////////////////////////////////////////////////////////////
    /* The init() function initializes the transaction tab entries */
    /////////////////////////////////////////////////////////////////////
    void init(){
    	for ( size_t i=0; i<size_tab; i++) {
		    addr_tab[i]=0;
		    valid_tab[i]=false;
    	}
    }


    /////////////////////////////////////////////////////////////////////
    /* The set() function sets an entry 
       Arguments :
       - id : the id of the initiator (index of the entry)
       - addr : the address of the lock
     */
    /////////////////////////////////////////////////////////////////////
    void set(const size_t id, const addr_t addr){
	    assert( (id<size_tab) && "Atomic Tab Error : bad entry");
	    addr_tab[id]=addr;
	    valid_tab[id]=true;
	    return;
    }


    /////////////////////////////////////////////////////////////////////
    /* The isatomic() function tests if the SC request corresponds to an entry
       Arguments :
       - id : the id of the initiator (index of the entry)
       - addr : the address of the lock

       This function return true if the request corresponds
     */
    /////////////////////////////////////////////////////////////////////
    bool isatomic(const size_t id, const addr_t addr){
	    assert( (id<size_tab) && "Atomic Tab Error : bad entry");
	    bool test=valid_tab[id];
	    test=test && (addr == addr_tab[id]);
	    return test;
    }


    /////////////////////////////////////////////////////////////////////
    /* The reset() function resets all the entries for this lock
       Arguments :
       - addr : the address of the lock
     */
    /////////////////////////////////////////////////////////////////////
    void reset(const addr_t addr){
	    for(size_t i=0 ; i<size_tab ; i++){
		    if(addr == addr_tab[i]){
			    valid_tab[i]=false;
		    }
	    }
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

