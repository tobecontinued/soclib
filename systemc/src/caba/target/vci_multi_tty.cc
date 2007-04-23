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

#include "caba/target/vci_multi_tty.h"
#include "soclib/tty.h"

#include <stdarg.h>

namespace soclib {
namespace caba {

#define tmpl(typ) template<typename vci_param> typ VciMultiTty<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
	int reg = (int)addr & 0xc;
	int term_no = (int)addr >> 4;
	char _data = data;

	switch (reg) {
	case TTY_WRITE:
        if ( _data == '\a' ) {
            char tmp[32];
            size_t ret = snprintf(tmp, sizeof(tmp), "[%10ld] ", r_counter);

            for ( size_t i=0; i<ret; ++i )
                m_term[term_no]->putc( tmp[i] );
        } else
            m_term[term_no]->putc( _data );
		return true;
	default:
		return false;
	}
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
	int reg = (int)addr & 0xc;
	int term_no = (int)addr >> 4;

	switch (reg) {
	case TTY_STAUS:
		data = m_term[term_no]->hasData();
		return true;
	case TTY_READ:
        if ( m_term[term_no]->hasData() ) {
            char tmp = m_term[term_no]->getc();
            data = tmp;
        }
        return true;
	default:
		return false;
	}
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_fsm.reset();
		r_counter = 0;
		return;
	}

	r_counter++;

	m_vci_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();

	for ( size_t i=0; i<m_term.size(); i++ )
		p_irq[i] = m_term[i]->hasData();
}

tmpl(/**/)::VciMultiTty(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
    const char *first_name,
    ...)
        : soclib::caba::BaseModule(name), m_vci_fsm(p_vci, mt.getSegmentList(index))
{
	m_vci_fsm.on_read_write(on_read, on_write);

	va_list va_tty;

	va_start (va_tty, first_name);
	const char *cur_tty = first_name;
	while (cur_tty) {
		m_term.push_back(soclib::common::allocateTty(cur_tty));

		cur_tty = va_arg( va_tty, char * );
	}
	va_end( va_tty );

	p_irq = new sc_out<bool>[m_term.size()];

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

tmpl(/**/)::~VciMultiTty()
{
	for (unsigned int i=0; i<m_term.size(); i++ )
        delete m_term[i];

	delete[] p_irq;
}

tmpl(void)::trace(sc_trace_file &tf, const std::string base_name, unsigned int what)
{

}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

