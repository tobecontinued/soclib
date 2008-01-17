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
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 */
#ifndef SOCLIB_CABA_ISS_WRAPPER_H_
#define SOCLIB_CABA_ISS_WRAPPER_H_

#include <inttypes.h>
#include "caba/util/base_module.h"
#include "common/endian.h"
#include "common/iss/iss.h"
#include "caba/interface/xcache_processor_ports.h"

namespace soclib { namespace caba {

template<typename iss_t>
class IssWrapper
	: public soclib::caba::BaseModule
{
public:
	soclib::caba::ICacheProcessorPort p_icache;
	soclib::caba::DCacheProcessorPort p_dcache;
	sc_in<bool> p_irq[iss_t::n_irq];
	sc_in<bool> p_resetn;
	sc_in<bool> p_clk;

private:
	iss_t m_iss;
	void transition();
	void genMoore();

protected:
	SC_HAS_PROCESS(IssWrapper);

public:
	IssWrapper( sc_module_name insname, int ident );
	~IssWrapper();
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
