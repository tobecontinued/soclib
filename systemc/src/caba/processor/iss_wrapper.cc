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
#include "common/static_assert.h"

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

    /*
     * Ensure both the Xcache and the ISS have the same opcods
     * values...
     */
    static_assert((int)DCacheSignals::READ_WORD   == (int)iss_t::READ_WORD  );
    static_assert((int)DCacheSignals::READ_HALF   == (int)iss_t::READ_HALF  );
    static_assert((int)DCacheSignals::READ_BYTE   == (int)iss_t::READ_BYTE  );
    static_assert((int)DCacheSignals::LINE_INVAL  == (int)iss_t::LINE_INVAL );
    static_assert((int)DCacheSignals::WRITE_WORD  == (int)iss_t::WRITE_WORD );
    static_assert((int)DCacheSignals::WRITE_HALF  == (int)iss_t::WRITE_HALF );
    static_assert((int)DCacheSignals::WRITE_BYTE  == (int)iss_t::WRITE_BYTE );
    static_assert((int)DCacheSignals::STORE_COND  == (int)iss_t::STORE_COND );
    static_assert((int)DCacheSignals::READ_LINKED == (int)iss_t::READ_LINKED);
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

    bool dfrz = p_dcache.frz.read();
    bool dreq = p_dcache.req.read();
    bool dberr = p_dcache.berr.read();
    bool frozen = m_iss.isBusy() || p_icache.frz.read() || dfrz;

    if ( p_icache.req.read() )
        m_iss.setInstruction(p_icache.berr, p_icache.ins.read());

    if ( dreq && !dfrz )
        m_iss.setDataResponse(dberr, p_dcache.rdata.read());
    else if ( dberr ) {
        std::cout << name() << "Write berr" << std::endl;
        m_iss.setWriteBerr();
    }

	if ( frozen )
        m_iss.nullStep();
    else {
        // Execute one cycle:
		uint32_t it = 0;
		for ( size_t i=0; i<(size_t)iss_t::n_irq; i++ )
			if (p_irq[i].read())
				it |= (1<<i);
		
		m_iss.setIrq(it);
		m_iss.step();
	}
}

tmpl(void)::genMoore()
{
    {
        bool ins_asked;
        uint32_t ins_addr;
        m_iss.getInstructionRequest( ins_asked, ins_addr );
        p_icache.req = ins_asked;
        p_icache.adr = ins_addr;
    }
    {
        bool mem_asked;
        enum soclib::common::Iss::DataAccessType mem_type;
        uint32_t mem_addr;
        uint32_t mem_wdata;
        m_iss.getDataRequest( mem_asked, mem_type, mem_addr, mem_wdata );
        p_dcache.req = mem_asked;
        p_dcache.type = (DCacheSignals::req_type_e)mem_type;
        p_dcache.adr = mem_addr;
        p_dcache.wdata = mem_wdata;
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
