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
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Based on previous works by Alain Greiner, 2004
 */

#include "timer.h"
#include "alloc_elems.h"
#include "../include/vci_timer.h"

namespace soclib { namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciTimer<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;
	int reg = cell % TIMER_SPAN;
	int timer = cell / TIMER_SPAN;

    if (timer>=(int)m_ntimer)
        return false;

	switch (reg) {
	case TIMER_VALUE:
		r_value[timer] = data;
		break;

	case TIMER_RESETIRQ:
		r_irq[timer] = false;
		break;

	case TIMER_MODE:
		r_mode[timer] = (int)data & 0x3;
        // Reset the timer if set running
        if ( data & TIMER_RUNNING )
			r_counter[timer] = r_period[timer].read();
		break;

	case TIMER_PERIOD:
		r_period[timer] = data;
		break;
	}
    m_cpt_write++;
	return true;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    int cell = (int)addr / vci_param::B;
	int reg = cell % TIMER_SPAN;
	int timer = cell / TIMER_SPAN;

    if (timer>=(int)m_ntimer)
        return false;

	switch (reg) {
	case TIMER_VALUE:
		data = r_value[timer].read();
		break;

	case TIMER_PERIOD:
		data = r_period[timer].read();
		break;

	case TIMER_MODE:
		data = r_mode[timer].read();
		break;

	case TIMER_RESETIRQ:
		data = r_irq[timer].read();
		break;
	}
    m_cpt_read++;

	return true;
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_fsm.reset();
        m_cpt_read = 0;
        m_cpt_write = 0;

		for (size_t i = 0 ; i < m_ntimer ; i++) {
			r_value[i] = 0;
			r_period[i] = 0;
			r_counter[i] = 0;
			r_mode[i] = 0;
			r_irq[i] = false;
		}
		return;
	}

	m_vci_fsm.transition();

	// Increment value[i]
	// decrement r_counter[i] & Set irq[i]
	for(size_t i = 0 ; i < m_ntimer ; i++) {
		if ( ! (r_mode[i].read() & TIMER_RUNNING) )
			continue;

		r_value[i] = r_value[i].read() + 1;

		if ( r_counter[i].read() != 0 )
			r_counter[i] = r_counter[i].read() - 1;
		else {
			r_counter[i] = r_period[i].read();
			r_irq[i] = true;
		}
	}
}

tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();

	for (size_t i = 0 ; i < m_ntimer ; i++)
		p_irq[i] = r_irq[i].read() && (r_mode[i].read() & TIMER_IRQ_ENABLED);
}

tmpl(/**/)::VciTimer(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
    size_t nirq)
	: caba::BaseModule(name),
	  m_vci_fsm(p_vci, mt.getSegmentList(index), 1),
      m_ntimer(nirq),
      r_value(soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >("value", m_ntimer)),
      r_period(soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >("period", m_ntimer)),
      r_counter(soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >("counter", m_ntimer)),
      r_mode(soclib::common::alloc_elems<sc_signal<int> >("mode", m_ntimer)),
      r_irq(soclib::common::alloc_elems<sc_signal<bool> >("saved_irq", m_ntimer)),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci"),
      p_irq(soclib::common::alloc_elems<sc_out<bool> >("irq", m_ntimer))
{
	m_vci_fsm.on_read_write(on_read, on_write);

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

tmpl(/**/)::~VciTimer()
{
    soclib::common::dealloc_elems(r_value, m_ntimer);
    soclib::common::dealloc_elems(r_period, m_ntimer);
    soclib::common::dealloc_elems(r_counter, m_ntimer);
    soclib::common::dealloc_elems(r_mode, m_ntimer);
    soclib::common::dealloc_elems(r_irq, m_ntimer);
    soclib::common::dealloc_elems(p_irq, m_ntimer);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

