/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2007
 *         Francois Pecheux <francois.pecheux@lip6.fr>, 2007
 *
 * 
 * Dec 2008 - Francois Charot
 * Adaptation of the iss_wrapper component to fit the requirements of the tms320c6x processor
 *
 * $Id$
 */

#ifndef SOCLIB_CABA_ISS_C6X_WRAPPER_H_
#define SOCLIB_CABA_ISS_C6X_WRAPPER_H_

#include <inttypes.h>
#include "caba_base_module.h"
#include "soclib_endian.h"
#include "xcache_processor_ports.h"

namespace soclib { namespace caba {

template<typename iss_t>
class IssC6xWrapper
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
	SC_HAS_PROCESS(IssC6xWrapper);

public:
	IssC6xWrapper( sc_module_name insname, int ident );
	~IssC6xWrapper();

    void setCacheInfo( const struct XCacheInfo &info );
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
