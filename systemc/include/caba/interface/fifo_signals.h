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
#ifndef SOCLIB_CABA_FIFO_SIGNALS_H
#define SOCLIB_CABA_FIFO_SIGNALS_H

#include <systemc>

namespace soclib { namespace caba {

	using namespace sc_core;

template <typename word_t>
class FifoSignals
{
public:
    sc_signal<word_t> data;
    sc_signal<bool> r_wok;
    sc_signal<bool> w_rok;

#define __ren(x) x((insname+"_" #x).c_str())
	FifoSignals(std::string insname = sc_gen_unique_name("fifo_signals"))
		: __ren(data),
		  __ren(r_wok),
		  __ren(w_rok)
	{
	}
#undef __ren
};

}}

#endif /* SOCLIB_CABA_FIFO_SIGNALS_H */
