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
    using soclib::common::Iss;

	if ( ! p_resetn.read() ) {
		m_iss.reset();
        m_ins_asked = false;
        m_mem_type = Iss::MEM_NONE;
        m_iss.getInstructionRequest( m_ins_asked, m_ins_addr );
		return;
	}

    bool frozen = false;

    if ( m_ins_asked ) {
        if ( p_icache.frz.read() )
            frozen = true;
        else
            m_iss.setInstruction(p_icache.berr, p_icache.ins.read());
    }

    if ( m_mem_type != Iss::MEM_NONE ) {
        if ((bool)p_dcache.berr.read()) {
            switch(m_mem_type) {
            case Iss::MEM_LB:
            case Iss::MEM_LBU:
            case Iss::MEM_LH:
            case Iss::MEM_LHBR:
            case Iss::MEM_LHU:
            case Iss::MEM_LWBR:
            case Iss::MEM_LW:
                m_iss.setRdata(true, 0);
                break;
            case Iss::MEM_SB:
            case Iss::MEM_SH:
            case Iss::MEM_SW:
                m_iss.setWriteBerr();
                break;
            case Iss::MEM_INVAL:
            case Iss::MEM_NONE:
                assert(0 && "Impossible");
            }
            m_mem_type = Iss::MEM_NONE;
        } else {
            if ((bool)p_dcache.frz.read())
                frozen = true;
            else {
                m_iss.setRdata(false, p_dcache.rdata.read());
                m_mem_type = Iss::MEM_NONE;
            }
        }
    }

	if ( frozen || m_iss.isBusy() )
        m_iss.nullStep();
    else {
        // Execute one cycle:
		uint32_t it = 0;
		for ( size_t i=0; i<(size_t)iss_t::n_irq; i++ )
			if (p_irq[i].read())
				it |= (1<<i);
		
		m_iss.setIrq(it);
		m_iss.step();
        m_iss.getDataRequest( m_mem_type, m_mem_addr, m_mem_wdata );
	}
    m_iss.getInstructionRequest( m_ins_asked, m_ins_addr );
}

tmpl(void)::genMoore()
{
    using soclib::common::Iss;

	p_icache.req = m_ins_asked;
	p_icache.adr = m_ins_addr;
 
 	switch( m_mem_type ) {
	case Iss::MEM_LB:
	case Iss::MEM_LBU:
	case Iss::MEM_LH:
	case Iss::MEM_LHU:
	case Iss::MEM_LW:
	case Iss::MEM_LHBR:
	case Iss::MEM_LWBR:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::RW;
		p_dcache.adr = m_mem_addr;
		p_dcache.wdata = 0;
		break;
	case Iss::MEM_SB:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::WB;
		p_dcache.adr = m_mem_addr;
		p_dcache.wdata = m_mem_wdata;
		break;
	case Iss::MEM_SH:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::WH;
		p_dcache.adr = m_mem_addr;
		p_dcache.wdata = m_mem_wdata;
		break;
	case Iss::MEM_SW:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::WW;
		p_dcache.adr = m_mem_addr;
		p_dcache.wdata = m_mem_wdata;
		break;
	case Iss::MEM_INVAL:
		p_dcache.req = true;
		p_dcache.type = DCacheSignals::RZ;
		p_dcache.adr = m_mem_addr;
		p_dcache.wdata = 0;
		break;
	case Iss::MEM_NONE:
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
