//////////////////////////////////////////////////////////////////////////
// File     : xcache_signals.h
// Date     : 17/07/2007
// Copyright: UPMC/LIP6
/////////////////////////////////////////////////////////////////////////
// History
// - 17/07/2007
//   The DCACHE interface has been modified by A.Greiner :
//   The "unc" signal has been supressed,
//   and  replaced by a new code RU for the "type" signal.
////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_CABA_SIGNAL_XCACHE_SIGNALS_H_
#define SOCLIB_CABA_SIGNAL_XCACHE_SIGNALS_H_

#include <systemc.h>

namespace soclib { namespace caba {

/*
 * DCACHE signals
 */
class DCacheSignals
{
public:
	enum req_type_e {
		WW = 0x8,       // Write Word
		WH = 0x9,       // Write Half
		WB = 0xA,       // Write Byte
		RW = 0x0,       // Read  
		RU = 0x2,       // Read Uncached 
		RZ = 0x4,       // Line Invalidate
	};
	
	sc_signal<bool>            req;   // valid request
	sc_signal<sc_uint<4> >     type;  // request type
	sc_signal<sc_uint<32> >    wdata; // data from processor
	sc_signal<sc_uint<32> >    adr;   // address
	sc_signal<bool>            frz;   // request not accepted
	sc_signal<sc_uint<32> >    rdata; // data from cache
	sc_signal<bool>            berr;  // bus or memory error 

	DCacheSignals(std::string name_ = (std::string)sc_gen_unique_name("dcache"))
		: req     (((std::string) (name_ + "_req"   )).c_str()),
		  type    (((std::string) (name_ + "_type"  )).c_str()),
		  wdata   (((std::string) (name_ + "_wdata" )).c_str()),
		  adr     (((std::string) (name_ + "_adr"   )).c_str()),
		  frz     (((std::string) (name_ + "_frz"   )).c_str()),
		  rdata   (((std::string) (name_ + "_rdata" )).c_str()),
		  berr    (((std::string) (name_ + "_berr"  )).c_str())
	{}
};

/*
 * ICACHE signals
 */
class ICacheSignals
{
public:
	enum req_type_e {
		RI = 0x0,       // Read 
		RZ = 0x1,       // Line Invalidate
		RU = 0x2,       // Read Uncached
	};
	
	sc_signal<bool>             req;  // valid read request
	sc_signal<sc_uint<32> >     adr;  // instruction address
	sc_signal<sc_uint<2> >      type; // request type
	sc_signal<bool>             frz;  // instruction not valid
	sc_signal<sc_uint<32> >     ins;  // instruction
	sc_signal<bool>             berr; // bus or memory error
	
	ICacheSignals (std::string name_ = (std::string) sc_gen_unique_name ("icache"))
		: req    (((std::string) (name_ + "_req" )).c_str()),
		  adr    (((std::string) (name_ + "_adr" )).c_str()),
		  type   (((std::string) (name_ + "_type")).c_str()),
		  frz    (((std::string) (name_ + "_frz" )).c_str()),
		  ins    (((std::string) (name_ + "_ins" )).c_str()),
		  berr   (((std::string) (name_ + "_berr")).c_str())
	{
	}
};

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
