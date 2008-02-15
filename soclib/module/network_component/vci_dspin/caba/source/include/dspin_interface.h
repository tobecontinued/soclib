/* -*- c++ -*-
  * File : dspin_interface.h
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner, Abbas Sheibanyrad, Ivan Miro, Zhen Zhang
  *
  * SOCLIB_LGPL_HEADER_BEGIN
  * SoCLib is free software; you can redistribute it and/or modify it
  * under the terms of the GNU Lesser General Public License as published
  * by the Free Software Foundation; version 2.1 of the License.
  * SoCLib is distributed in the hope that it will be useful, but
  * WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * You should have received a copy of the GNU Lesser General Public
  * License along with SoCLib; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  * 02110-1301 USA
  * SOCLIB_LGPL_HEADER_END
  */

#ifndef SOCLIB_DSPIN_INTERFACE_H_
#define SOCLIB_DSPIN_INTERFACE_H_

#include <systemc>
#include "static_assert.h"

namespace soclib { namespace caba {

using namespace sc_core;
using namespace sc_dt;

enum{
	DSPIN_BOP 	= 0x1,
	DSPIN_EOP 	= 0x2,
	DSPIN_ERR	= 0x4,
	DSPIN_PAR	= 0x8
};

enum{
	DSPIN_CONS	= 0x04000000,
	DSPIN_CONTIG	= 0x08000000
};

/***  DSPIN SIGNALS  ***/
template<int dspin_data_size>
class DspinSignals {
    public:
	sc_signal<sc_uint<dspin_data_size> >  	data;    // data
	sc_signal<bool>                 	write;   // write command 
	sc_signal<bool>                 	read;    // read command

#define __ren(x) x((insname+"_" #x).c_str())
	DspinSignals(std::string insname = sc_gen_unique_name("dspin_signals"))
	    : __ren(data),
	    __ren(write),
	    __ren(read)
    {}
#undef __ren

};


/***  DSPIN OUT Ports ***/
template<int dspin_data_size>
struct DspinOutput {
    sc_out<sc_uint<dspin_data_size> >     	data;    // data
    sc_out<bool>                    	write;   // valid data
    sc_in<bool>                     	read;    // data accepted

    void operator () (DspinSignals<dspin_data_size> &sig) {
	data         (sig.data);
	write        (sig.write);
	read         (sig.read);
    };

    void operator () (DspinOutput<dspin_data_size> &port) {
	data         (port.data);
	write        (port.write);
	read         (port.read);
    };


#define __ren(x) x((name+"_" #x).c_str())
    DspinOutput(const std::string &name = sc_gen_unique_name("dspin_output"))
	: __ren(data),
	__ren(write),
	__ren(read)
    {}
#undef __ren

}; 


/*** DSPIN IN Ports ***/
template<int dspin_data_size>
struct DspinInput {
    sc_in<sc_uint<dspin_data_size> >     	data;     // data
    sc_in<bool>                    		write;    // valid data
    sc_out<bool>                   		read;     // data accepted

    void operator () (DspinSignals<dspin_data_size> &sig) {
	data         (sig.data);
	write        (sig.write);
	read         (sig.read);
    };

    void operator () (DspinInput<dspin_data_size> &port) {
	data         (port.data);
	write        (port.write);
	read         (port.read);
    };

#define __ren(x) x((name+"_" #x).c_str())
    DspinInput(const std::string &name = sc_gen_unique_name("dspin_input"))
	: __ren(data),
	__ren(write),
	__ren(read)
    {}
#undef __ren
};

}}

#endif

