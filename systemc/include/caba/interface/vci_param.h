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
#ifndef SOCLIB_CABA_SIGNAL_VCI_PARAM_H_
#define SOCLIB_CABA_SIGNAL_VCI_PARAM_H_

#include <systemc>
#include "common/static_assert.h"

namespace soclib { namespace caba {

using namespace sc_core;

namespace VCI_CMD {
enum {
    NOP,
    READ,
    WRITE,
    LOCKED_READ,
    STORE_COND = NOP,
};
}

/**
 * VCI parameters grouped in a single class
 */
template<
    int cell_size,
    int plen_size,
    int addr_size,
    int rerror_size,
    int clen_size,
    int rflag_size,
    int srcid_size,
    int pktid_size,
    int trdid_size,
	int wrplen_size
    >
class VciParams
{
	/* Obey standart */

	// This is a check for a pow of 2
    static_assert(!((cell_size)&(cell_size-1)));
	static_assert(plen_size <= 9);
    static_assert(addr_size <= 32);
	static_assert(rerror_size <= 3);
	static_assert(clen_size <= 8);
    // We need more than 5 bits for srcid, so we dont check
    //static_assert(srcid_size <= 5);
	static_assert(pktid_size <= 8);
	static_assert(wrplen_size <= 5);

public:
    /* Standart's constants, may be used by some modules */
    static const int B = cell_size;
    static const int K = plen_size;
    static const int N = addr_size;
    static const int E = rerror_size;
    static const int Q = clen_size;
    static const int F = rflag_size;
    static const int S = srcid_size;
    static const int P = pktid_size;
    static const int T = trdid_size;
    static const int W = wrplen_size;

    /* The basic signal types */
	/* Handshake */
	typedef bool ack_t;
	typedef bool val_t;
	/* Request content */
	typedef sc_dt::sc_uint<N> addr_t;
	typedef sc_dt::sc_uint<B> be_t;
	typedef bool cfixed_t;
	typedef sc_dt::sc_uint<Q> clen_t;
	typedef sc_dt::sc_uint<2> cmd_t;
	typedef bool contig_t;
	typedef sc_dt::sc_uint<B*8> data_t;
	typedef bool eop_t;
	typedef bool const_t;
	typedef sc_dt::sc_uint<K> plen_t;
	typedef bool wrap_t;
	/* Response content */
	typedef sc_dt::sc_uint<E> rerror_t;

	/* The advanced signal types */
	/* Request content */
	typedef bool defd_t;
	typedef sc_dt::sc_uint<W> wrplen_t;
	/* Response content */
	typedef sc_dt::sc_uint<F> rflag_t;
	/* Threading */
	typedef sc_dt::sc_uint<S> srcid_t;
	typedef sc_dt::sc_uint<T> trdid_t;
	typedef sc_dt::sc_uint<P> pktid_t;
};

}}

#endif /* SOCLIB_CABA_SIGNAL_VCI_PARAM_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

