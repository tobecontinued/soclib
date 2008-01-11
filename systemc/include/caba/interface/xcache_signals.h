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

#include <systemc>

namespace soclib { namespace caba {

    using namespace sc_core;

/*
 * DCACHE signals
 */
class DCacheSignals
{
public:
	enum req_type_e {
		READ_WORD,
        READ_HALF,
        READ_BYTE,
		LINE_INVAL,
		WRITE_WORD,
		WRITE_HALF,
		WRITE_BYTE,
		STORE_COND,
		READ_LINKED,
	};

	sc_signal<bool>            req;   // valid request
	sc_signal<sc_dt::sc_uint<4> >     type;  // request type
	sc_signal<sc_dt::sc_uint<32> >    wdata; // data from processor
	sc_signal<sc_dt::sc_uint<32> >    adr;   // address
	sc_signal<bool>            frz;   // request not accepted
	sc_signal<sc_dt::sc_uint<32> >    rdata; // data from cache
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
	sc_signal<sc_dt::sc_uint<32> >     adr;  // instruction address
	sc_signal<sc_dt::sc_uint<2> >      type; // request type
	sc_signal<bool>             frz;  // instruction not valid
	sc_signal<sc_dt::sc_uint<32> >     ins;  // instruction
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
