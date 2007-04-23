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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_FIFO_PORTS_H
#define SOCLIB_CABA_FIFO_PORTS_H

#include <systemc.h>
#include "caba/interface/fifo_signals.h"

namespace soclib { namespace caba {

template <typename word_t>
struct FifoInput {
	sc_in<word_t> data;
	sc_out<bool> r;
	sc_in<bool> rok;

	void operator() (FifoSignals<word_t> &sig)
	{
		data(sig.data);
		r(sig.r_wok);
		rok(sig.w_rok);
	}

	void operator() (FifoInput<word_t> &port)
	{
		data(port.data);
		r(port.r);
		rok(port.rok);
	}
};

template <typename word_t>
struct FifoOutput {
	sc_out<word_t> data;
	sc_in<bool> wok;
	sc_out<bool> w;

	void operator() (FifoSignals<word_t> &sig)
	{
		data(sig.data);
		wok(sig.r_wok);
		w(sig.w_rok);
	}

	void operator() (FifoOutput<word_t> &port)
	{
		data(port.data);
		wok(port.wok);
		w(port.w);
	}
};

}}

#endif /* SOCLIB_CABA_FIFO_PORTS_H */
