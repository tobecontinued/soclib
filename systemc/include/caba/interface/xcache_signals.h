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
#ifndef SOCLIB_CABA_SIGNAL_XCACHE_SIGNALS_H_
#define SOCLIB_CABA_SIGNAL_XCACHE_SIGNALS_H_

#include <systemc.h>

namespace soclib { namespace caba {

/**
 * DCACHE signals
 */
class DCacheSignals
{
public:
	enum req_type_e {
		WW = 0x8, // Write Word
		WH = 0x9, // Write Half
		WB = 0xA, // Write Byte
		RW = 0x0, // Read Word Cached
		RZ = 0x4, // Line Invalidate
	};
	
	sc_signal<bool>         req;   // valid request
	sc_signal<sc_uint<4> >  type;  // request type
	sc_signal<bool>         unc;   // uncached request
	sc_signal<sc_uint<32> > wdata; // data from processor
	sc_signal<sc_uint<32> > adr;   // address
	sc_signal<bool>         frz;   // request not accepted
	sc_signal<sc_uint<32> > rdata; // data from cache
	sc_signal<bool>         berr;  // bus or memory error 

	DCacheSignals(std::string name_ = (std::string)sc_gen_unique_name("dcache"))
		: req     (((std::string) (name_ + "_req"   )).c_str()),
		  type    (((std::string) (name_ + "_type"  )).c_str()),
		  unc     (((std::string) (name_ + "_unc"   )).c_str()),
		  wdata   (((std::string) (name_ + "_wdata" )).c_str()),
		  adr     (((std::string) (name_ + "_adr"   )).c_str()),
		  frz     (((std::string) (name_ + "_frz"   )).c_str()),
		  rdata   (((std::string) (name_ + "_rdata" )).c_str()),
		  berr    (((std::string) (name_ + "_berr"  )).c_str())
	{}
};

/**
 * ICACHE signals
 */
class ICacheSignals
{
public:
	enum req_type_e {
		RW = 0x0, // Read Word Cached
		RZ = 0x1, // Line Invalidate
		RU = 0x2, // Read Word Uncached
	};
	
	sc_signal<bool>          req;  // valid read request
	sc_signal<sc_uint<32> >  adr;  // instruction address
	sc_signal<sc_uint<2> >   type; // instruction address
	sc_signal<bool>          frz; // instruction not valid
	sc_signal<sc_uint<32> >  ins;  // 32 bits instruction
	sc_signal<bool>          berr; // bus or memory error
	
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

#endif /* SOCLIB_CABA_SIGNAL_XCACHE_SIGNALS_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
