/* -*- c++ -*-
  * File : dspin_router.h
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

#ifndef DSPIN_ROUTER_H_
#define DSPIN_ROUTER_H_

#include <systemc>
#include "caba_base_module.h"
#include "generic_fifo.h"
#include "dspin_interface.h"

namespace soclib { namespace caba {

    using namespace sc_core;

    template<typename vci_param, int dspin_data_size, int dspin_fifo_size>
	class DspinRouter
	: public soclib::caba::BaseModule
	{

	    // FSM of request
	    enum{
		NORTH	= 0,
		SOUTH	= 1,
		EAST	= 2,
		WEST	= 3,
		LOCAL	= 4
	    };

	    protected:
	    SC_HAS_PROCESS(DspinRouter);

	    public:
	    // ports
	    sc_in<bool>                             	p_clk;
	    sc_in<bool>                             	p_resetn;

	    // fifo req ant rsp
	    DspinOutput<dspin_data_size>		p_out[5];
	    DspinInput<dspin_data_size>			p_in[5];

	    // constructor / destructor
	    DspinRouter(sc_module_name    insname, int indent);

	    private:
	    // internal registers
	    sc_signal<int>				r_alloc_out[5];
	    sc_signal<int>				r_alloc_in[5];
	    sc_signal<int>                  		r_index_out[5];
	    sc_signal<int>                  		r_index_in[5];

	    // deux fifos req and rsp
	    soclib::caba::GenericFifo<sc_uint<dspin_data_size> > *fifo_in;
	    soclib::caba::GenericFifo<sc_uint<dspin_data_size> > *fifo_out;

	    // Index of router
	    int		XLOCAL;
	    int		YLOCAL;

	    // methods systemc
	    void transition();
	    void genMoore();

	    // checker
	    static_assert(dspin_fifo_size <= 256 && dspin_fifo_size >= 1);
	};

}} // end namespace
               
#endif // VCI_DSPIN_INITIATOR_WRAPPER_H_
