/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_SIGNAL_XCACHE_PORTS_H_
#define SOCLIB_CABA_SIGNAL_XCACHE_PORTS_H_

#include <systemc.h>
#include "caba/interface/xcache_signals.h"

namespace soclib { namespace caba {

/**
 * DCACHE cache port
 */
class DCacheCachePort
{
public:
	sc_in<bool>          req;
	sc_in<sc_uint<4> >   type;
	sc_in<bool>          unc;
	sc_in<sc_uint<32> >  wdata;
	sc_in<sc_uint<32> >  adr;
	sc_out<bool>         frz;
	sc_out<sc_uint<32> > rdata;
	sc_out<bool>         berr; 

    
	void operator () (DCacheSignals &sig)
	{
		req    (sig.req);
		type   (sig.type);
		unc    (sig.unc);
		wdata  (sig.wdata);
		adr    (sig.adr);
		frz   (sig.frz);
		rdata  (sig.rdata);
		berr   (sig.berr);
	}
};

/**
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

#endif /* SOCLIB_CABA_SIGNAL_XCACHE_PORTS_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
