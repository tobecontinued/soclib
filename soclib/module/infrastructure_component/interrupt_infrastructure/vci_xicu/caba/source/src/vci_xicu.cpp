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
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 */

#include <strings.h>

#include "xicu.h"
#include "register.h"
#include "arithmetics.h"
#include "alloc_elems.h"
#include "../include/vci_xicu.h"

namespace soclib {
namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciXicu<vci_param>

#define CHECK_BOUNDS(x) do { if ( idx >= (m_##x##_count) ) return false; } while(0)

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
	size_t cell = (size_t)addr / vci_param::B;
	size_t idx = cell & 0x1f;
	size_t func = (cell >> 5) & 0x1f;

    if ( be != 0xf )
        return false;

	switch (func) {
    case XICU_WTI_REG:
        CHECK_BOUNDS(wti);
        r_wti_reg[idx] = data;
        r_wti_pending |= 1<<idx;
        return true;
    case XICU_PTI_PER:
        CHECK_BOUNDS(pti);
        r_pti_per[idx] = data;
        if ( !data ) {
            r_pti_pending &= ~(1<<idx);
            r_pti_val[idx] = 0;
        } else if (r_pti_val[idx] == 0) {
            r_pti_val[idx] = data;
        }
        return true;
    case XICU_PTI_VAL:
        CHECK_BOUNDS(pti);
        r_pti_val[idx] = data;
        return true;
    case XICU_MSK_PTI:
        CHECK_BOUNDS(irq);
        r_msk_pti[idx] = data;
        return true;
    case XICU_MSK_PTI_ENABLE:
        CHECK_BOUNDS(irq);
        r_msk_pti[idx] |= data;
        return true;
    case XICU_MSK_PTI_DISABLE:
        CHECK_BOUNDS(irq);
        r_msk_pti[idx] &= ~data;
        return true;
    case XICU_MSK_HWI:
        CHECK_BOUNDS(irq);
        r_msk_hwi[idx] = data;
        return true;
    case XICU_MSK_HWI_ENABLE:
        CHECK_BOUNDS(irq);
        r_msk_hwi[idx] |= data;
        return true;
    case XICU_MSK_HWI_DISABLE:
        CHECK_BOUNDS(irq);
        r_msk_hwi[idx] &= ~data;
        return true;
    case XICU_MSK_WTI:
        CHECK_BOUNDS(irq);
        r_msk_wti[idx] = data;
        return true;
    case XICU_MSK_WTI_ENABLE:
        CHECK_BOUNDS(irq);
        r_msk_wti[idx] |= data;
        return true;
    case XICU_MSK_WTI_DISABLE:
        CHECK_BOUNDS(irq);
        r_msk_wti[idx] &= ~data;
        return true;
	}
	return false;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
	size_t cell = (size_t)addr / vci_param::B;
	size_t idx = cell & 0x1f;
	size_t func = (cell >> 5) & 0x1f;

	switch (func) {
    case XICU_WTI_REG:
        CHECK_BOUNDS(wti);
        data = r_wti_reg[idx];
        r_wti_pending &= ~(1<<idx);
        return true;
    case XICU_PTI_PER:
        CHECK_BOUNDS(pti);
        data = r_pti_per[idx];
        return true;
    case XICU_PTI_VAL:
        CHECK_BOUNDS(pti);
        data = r_pti_val[idx];
        return true;
    case XICU_PTI_ACK:
        CHECK_BOUNDS(pti);
        r_pti_pending &= ~(1<<idx);
        data = 0;
        return true;
    case XICU_MSK_PTI:
        CHECK_BOUNDS(irq);
        data = r_msk_pti[idx];
        return true;
    case XICU_PTI_ACTIVE:
        CHECK_BOUNDS(irq);
        data = r_msk_pti[idx] & r_pti_pending;
        return true;
    case XICU_MSK_HWI:
        CHECK_BOUNDS(irq);
        data = r_msk_hwi[idx];
        return true;
    case XICU_HWI_ACTIVE:
        CHECK_BOUNDS(irq);
        data = r_msk_hwi[idx] & r_hwi_pending;
        return true;
    case XICU_MSK_WTI:
        CHECK_BOUNDS(irq);
        data = r_msk_wti[idx];
        return true;
    case XICU_WTI_ACTIVE:
        CHECK_BOUNDS(irq);
        data = r_msk_wti[idx] & r_wti_pending;
        return true;
    case XICU_PRIO:
        CHECK_BOUNDS(irq);
        data = 
            (((r_msk_pti[idx] & r_pti_pending) ? 1 : 0) << 0) |
            (((r_msk_hwi[idx] & r_hwi_pending) ? 1 : 0) << 1) |
            (((r_msk_wti[idx] & r_wti_pending) ? 1 : 0) << 2) |
            ((soclib::common::ctz<uint32_t>(r_msk_pti[idx] & r_pti_pending) & 0x1f) <<  8) |
            ((soclib::common::ctz<uint32_t>(r_msk_hwi[idx] & r_hwi_pending) & 0x1f) << 16) |
            ((soclib::common::ctz<uint32_t>(r_msk_wti[idx] & r_wti_pending) & 0x1f) << 24);
        return true;
	}
	return false;
}

tmpl(void)::transition()
{
	if (!p_resetn.read()) {
		m_vci_fsm.reset();

        for ( size_t i = 0; i<m_pti_count; ++i ) {
            r_pti_per[i] = 0;
            r_pti_val[i] = 0;
        }
        for ( size_t i = 0; i<m_wti_count; ++i )
            r_wti_reg[i] = 0;
        for ( size_t i = 0; i<m_irq_count; ++i ) {
            r_msk_pti[i] = 0;
            r_msk_wti[i] = 0;
            r_msk_hwi[i] = 0;
        }
        r_pti_pending = 0;
        r_wti_pending = 0;
        r_hwi_pending = 0;

		return;
	}

    for ( size_t i = 0; i<m_pti_count; ++i ) {
        uint32_t per = r_pti_per[i];

        if ( per && --r_pti_val[i] == 0 ) {
            r_pti_pending |= 1<<i;
            r_pti_val[i] = per;
        }
    }

    uint32_t hwi_pending = 0;
    for ( size_t i = 0; i<m_hwi_count; ++i )
        hwi_pending |= (p_hwi[i].read() ? 1 : 0) << i;
    r_hwi_pending = hwi_pending;

	m_vci_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();

    for ( size_t i = 0; i<m_irq_count; ++i ) {
        p_irq[i] = 
            (r_msk_pti[i] & r_pti_pending) ||
            (r_msk_wti[i] & r_wti_pending) ||
            (r_msk_hwi[i] & r_hwi_pending);
    }
}

tmpl(/**/)::VciXicu(
    sc_core::sc_module_name name,
    const MappingTable &mt,
    const IntTab &index,
    size_t pti_count,
    size_t hwi_count,
    size_t wti_count,
    size_t irq_count)
           : caba::BaseModule(name),
           m_vci_fsm(p_vci, mt.getSegmentList(index)),
           m_pti_count(pti_count),
           m_hwi_count(hwi_count),
           m_wti_count(wti_count),
           m_irq_count(irq_count),
           r_msk_pti(new uint32_t[irq_count]),
           r_msk_wti(new uint32_t[irq_count]),
           r_msk_hwi(new uint32_t[irq_count]),
           r_pti_pending(0),
           r_wti_pending(0),
           r_hwi_pending(0),
           r_pti_per(new uint32_t[pti_count]),
           r_pti_val(new uint32_t[pti_count]),
           r_wti_reg(new uint32_t[wti_count]),
           p_clk("clk"),
           p_resetn("resetn"),
           p_vci("vci"),
           p_irq(soclib::common::alloc_elems<sc_core::sc_out<bool> >("irq", irq_count)),
           p_hwi(soclib::common::alloc_elems<sc_core::sc_in<bool> >("hwi", hwi_count))
{
	m_vci_fsm.on_read_write(on_read, on_write);

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

tmpl(/**/)::~VciXicu()
{
    delete r_msk_pti;
    delete r_msk_wti;
    delete r_msk_hwi;
    delete r_pti_per;
    delete r_pti_val;
    delete r_wti_reg;
    soclib::common::dealloc_elems(p_irq, m_irq_count);
    soclib::common::dealloc_elems(p_hwi, m_hwi_count);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

