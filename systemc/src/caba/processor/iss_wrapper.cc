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
#include "common/iss/iss.h"
#include "caba/processor/iss_wrapper.h"
#include "caba/interface/xcache_signals.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename iss_t> x IssWrapper<iss_t>

tmpl(/**/)::IssWrapper( sc_module_name insname, int ident )
	: BaseModule(insname),
      p_icache("icache"),
      p_dcache("dcache"),
      p_resetn("resetn"),
      p_clk("clk"),
	  m_iss(ident)
{
	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();
	
	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

tmpl(/**/)::~IssWrapper()
{
}

tmpl(void)::transition()
{
	if ( ! p_resetn.read() ) {
		m_iss.reset();
		return;
	}

	// Transmit asynchronous data bus error (write)
	if (p_dcache.berr)
		m_iss.setWriteBerr();

	// Execute one cycle:
	//
	// Transmit interrupts, Instruction and Iberr, Data and Dberr, Run
	if (!(p_icache.frz || p_dcache.frz)) {
		char it = 0;
		for ( size_t i=0; i<(size_t)iss_t::n_irq; i++ ) {
			if (p_irq[i])
				it |= 1 << i;
		}
		
		m_iss.setIrq(it);
		m_iss.setInstruction(p_icache.berr, p_icache.ins.read());
		m_iss.setRdata(p_dcache.berr, p_dcache.rdata.read());
		m_iss.step();
	}
}

tmpl(void)::genMoore()
{
	uint32_t i_adr		= 0;

	m_iss.getInstructionRequest( i_adr );
	p_icache.req = true;
	p_icache.adr = i_adr;

	uint32_t d_type 	= 0;
	uint32_t d_adr		= 0;
	uint32_t d_wdata	= 0;
	
	m_iss.getDataRequest( d_type, d_adr, d_wdata );

	switch( d_type ) {
	case soclib::common::Iss::MEM_LB:
	case soclib::common::Iss::MEM_LBU:
	case soclib::common::Iss::MEM_LH:
	case soclib::common::Iss::MEM_LHU:
	case soclib::common::Iss::MEM_LW:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::RW;
		p_dcache.adr = d_adr;
		p_dcache.wdata = 0;
		break;
	case soclib::common::Iss::MEM_SB:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::WB;
		p_dcache.adr = d_adr;
		p_dcache.wdata = d_wdata;
		break;
	case soclib::common::Iss::MEM_SH:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::WH;
		p_dcache.adr = d_adr;
		p_dcache.wdata = d_wdata;
		break;
	case soclib::common::Iss::MEM_SW:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::WW;
		p_dcache.adr = d_adr;
		p_dcache.wdata = d_wdata;
		break;
	case soclib::common::Iss::MEM_INVAL:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::RZ;
		p_dcache.adr = d_adr;
		p_dcache.wdata = 0;
		break;
	default:
		p_dcache.req = false;
		p_dcache.type = 0;
		p_dcache.adr = 0;
		p_dcache.wdata = 0;
		break;
	}
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
