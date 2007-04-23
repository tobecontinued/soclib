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
 */

#include "soclib/timer.h"
#include "common/register.h"
#include "caba/target/vci_timer.h"

namespace soclib {
namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciTimer<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
	int reg = ((int)addr & 0xc)>>2;
	int timer = ((int)addr & 0xff0)>>4;

	switch (reg) {
	case TIMER_VALUE:
		r_value[timer] = data;
		break;

	case TIMER_RESETIRQ:
		r_irq[timer] = false;
		break;

	case TIMER_MODE:
		r_mode[timer] = (int)data & 0x3;
		break;

	case TIMER_PERIOD:
		r_period[timer] = data;
		break;
	}
	return true;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
	int reg = ((int)addr & 0xc)>>2;
	int timer = ((int)addr & 0xff0)>>4;

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

	return true;
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_fsm.reset();

		for (size_t i = 0 ; i < n_irq ; i++) {
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
	for(size_t i = 0 ; i < n_irq ; i++) {
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

	for (size_t i = 0 ; i < n_irq ; i++)
		p_irq[i] = r_irq[i].read() && (r_mode[i].read() & TIMER_IRQ_ENABLED);
}

tmpl(/**/)::VciTimer(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
    size_t nirq)
	: caba::BaseModule(name),
	  m_vci_fsm(p_vci, mt.getSegmentList(index))
{
	m_vci_fsm.on_read_write(on_read, on_write);

	n_irq = nirq;

	r_value   = new sc_signal<typename vci_param::data_t>[nirq];
	r_period  = new sc_signal<typename vci_param::data_t>[nirq];
	r_counter = new sc_signal<typename vci_param::data_t>[nirq];
	r_mode    = new sc_signal<int>[nirq];
	r_irq     = new sc_signal<bool>[nirq];
	p_irq     = new sc_out<bool>[nirq];

	for (size_t i = 0; i < n_irq; i++) {
		SOCLIB_REG_RENAME_N(r_value, i);
		SOCLIB_REG_RENAME_N(r_period, i);
		SOCLIB_REG_RENAME_N(r_counter, i);
		SOCLIB_REG_RENAME_N(r_mode, i);
		SOCLIB_REG_RENAME_N(r_irq, i);
	}

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

