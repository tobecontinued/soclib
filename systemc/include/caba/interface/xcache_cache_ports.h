// -*- c++ -*-
// File     : xcache_cache_ports.h
// Date     : 17/07/2007
// Copyright: UPMC/LIP6
/////////////////////////////////////////////////////////////////////////
// History
// - 17/07/2007
//   The DCACHE interface has been modified by A.Greiner :
//   The "unc" signal has been supressed,
//   and  replaced by a new code RU for the "type" signal.
////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_CABA_SIGNAL_XCACHE_PORTS_H_
#define SOCLIB_CABA_SIGNAL_XCACHE_PORTS_H_

#include <systemc.h>
#include "caba/interface/xcache_signals.h"

namespace soclib { namespace caba {

/*
 * DCACHE cache port
 */
class DCacheCachePort
{
public:
	sc_in<bool>          req;
	sc_in<sc_uint<4> >   type;
	sc_in<sc_uint<32> >  wdata;
	sc_in<sc_uint<32> >  adr;
	sc_out<bool>         frz;
	sc_out<sc_uint<32> > rdata;
	sc_out<bool>         berr; 

    DCacheCachePort(const std::string &name = sc_gen_unique_name("dcache_cache"))
		: req    ((name+"_req").c_str()),
		  type   ((name+"_type").c_str()),
		  wdata  ((name+"_wdata").c_str()),
		  adr    ((name+"_adr").c_str()),
		  frz    ((name+"_frz").c_str()),
		  rdata  ((name+"_rdata").c_str()),
		  berr   ((name+"_berr").c_str())
	{
	}
    
	void operator () (DCacheSignals &sig)
	{
		req    (sig.req);
		type   (sig.type);
		wdata  (sig.wdata);
		adr    (sig.adr);
		frz    (sig.frz);
		rdata  (sig.rdata);
		berr   (sig.berr);
	}
};

/*
 * ICACHE cache port
 */
class ICacheCachePort
{
public:
	sc_in<bool> 	        req;
	sc_in<sc_uint<32> >     adr; 
	sc_in<sc_uint<2> >      type; 
	sc_out<bool> 	        frz;
	sc_out<sc_uint<32> >    ins;
	sc_out<bool>            berr;
    
    ICacheCachePort(const std::string &name = sc_gen_unique_name("icache_cache"))
		: req    ((name+"_req").c_str()),
		  adr    ((name+"_adr").c_str()),
		  type   ((name+"_type").c_str()),
		  frz    ((name+"_frz").c_str()),
		  ins    ((name+"_ins").c_str()),
		  berr   ((name+"_berr").c_str())
	{
	}

	void operator () (ICacheSignals &sig) {
		req   (sig.req);
		adr   (sig.adr);
		type  (sig.type);
		frz  (sig.frz);
		ins   (sig.ins);
		berr  (sig.berr);
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
