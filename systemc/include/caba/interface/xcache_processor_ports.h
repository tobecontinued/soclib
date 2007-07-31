//////////////////////////////////////////////////////////////////////////
// File     : xcache_processor_ports.h
// Date     : 17/07/2007
// Copyright: UPMC/LIP6
/////////////////////////////////////////////////////////////////////////
// History
// - 17/07/2007
//   The DCACHE interface has been modified by A.Greiner :
//   The "unc" signal has been supressed,
//   and  replaced by a new code RU for the "type" signal.
////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_CABA_XCACHE_PROCESSOR_PORTS_H_
#define SOCLIB_CABA_XCACHE_PROCESSOR_PORTS_H_

#include <systemc.h>
#include "caba/interface/xcache_signals.h"

namespace soclib { namespace caba {

/*
 * DCACHE cpu port
 */
class DCacheProcessorPort
{
public:
	sc_out<bool>         req;
	sc_out<sc_uint<4> >  type;
	sc_out<sc_uint<32> > wdata;
	sc_out<sc_uint<32> > adr;
	sc_in<bool>          frz;
	sc_in<sc_uint<32> >  rdata;
	sc_in<bool>          berr; 

    
	void operator () (DCacheSignals &sig)
	{
		req    (sig.req);
		type   (sig.type);
		wdata  (sig.wdata);
		adr    (sig.adr);
		frz   (sig.frz);
		rdata  (sig.rdata);
		berr   (sig.berr);
	}
};

/*
 * ICACHE cpu port
 */
class ICacheProcessorPort
{
public:
	sc_out<bool> 	        req;
	sc_out<sc_uint<32> >   adr; 
	sc_out<sc_uint<2> >    type; 
	sc_in<bool> 	       frz;
	sc_in<sc_uint<32> >    ins;
	sc_in<bool>            berr;
    
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
