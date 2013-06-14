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
 * Maintainers: nipo
 */

#include "../include/vci_multi_tty.h"
#include "tty.h"

#include <stdarg.h>

namespace soclib {
namespace caba {

#define tmpl(typ) template<typename vci_param> typ VciMultiTty<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;
	int reg = cell % TTY_SPAN;
	int term_no = cell / TTY_SPAN;
	char _data = data;

    if (term_no>=(int)m_term.size())
        return false;

	switch (reg) {
	case TTY_WRITE:
        m_term[term_no]->putc( _data );
        m_cpt_write++;
#if SOCLIB_MODULE_DEBUG
        std::cout << "[" << name() << "] term_no=" << term_no << " TTY_WRITE  data = " << std::hex << data << std::dec << " time = " << m_cpt_cycles << std::endl;
#endif
		return true;
	default:
		return false;
	}
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    int cell = (int)addr / vci_param::B;
	int reg = cell % TTY_SPAN;
	int term_no = cell / TTY_SPAN;

    if (term_no>=(int)m_term.size())
        return false;

	switch (reg) {
	case TTY_STATUS:
		data = m_term[term_no]->hasData();
#if SOCLIB_MODULE_DEBUG
        std::cout << "[" << name() << "] term_no=" << term_no << " TTY_STATUS data = " << std::hex << data << std::dec << " time = " << m_cpt_cycles << std::endl;
#endif
		return true;
	case TTY_READ:
        m_cpt_read++;
        if ( m_term[term_no]->hasData() ) {
            char tmp = m_term[term_no]->getc();
            data = tmp;
        }

#if SOCLIB_MODULE_DEBUG
        std::cout << "[" << name() << "] term_no=" << term_no << " TTY_READ   data = " << std::hex << data << std::dec << " time = " << m_cpt_cycles << std::endl;
#endif
        return true;
 	default:
		return false;
	}
}

////////////////////////
tmpl(void)::transition()
{
    m_cpt_cycles++;

	if (!p_resetn) {
		m_vci_fsm.reset();
        m_cpt_read = 0;
        m_cpt_write = 0;
        m_cpt_cycles = 0;
		r_counter = 0;
		return;
	}

	r_counter++;

	m_vci_fsm.transition();
}

//////////////////////
tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();

    for ( size_t i=0; i<m_term.size(); i++ )
        p_irq[i] = m_term[i]->hasData();
}

tmpl(void)::init(const std::vector<std::string> &names)
{
	m_vci_fsm.on_read_write(on_read, on_write);

    for ( std::vector<std::string>::const_iterator i = names.begin();
          i != names.end();
          ++i )
		m_term.push_back(soclib::common::allocateTty(*i));

	p_irq = new sc_out<bool>[m_term.size()];
    
	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

tmpl(/**/)::VciMultiTty(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
    const char *first_name,
    ...)
    : soclib::caba::BaseModule(name),
      m_vci_fsm(p_vci, mt.getSegmentList(index)),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci")
{
	va_list va_tty;

	va_start (va_tty, first_name);
    std::vector<std::string> args;
	const char *cur_tty = first_name;
	while (cur_tty) {
        args.push_back(cur_tty);

		cur_tty = va_arg( va_tty, char * );
	}
	va_end( va_tty );

    init(args);
}

////////////////////////
tmpl(/**/)::VciMultiTty(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
    const std::vector<std::string> &names )
    : soclib::caba::BaseModule(name),
      m_seglist(mt.getSegmentList(index)),
      m_vci_fsm(p_vci, m_seglist),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci")
{
    std::cout << "  - Building VciMultiTTy " << name << std::endl;

    std::list<soclib::common::Segment>::iterator seg;
    for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ )
    {
        std::cout << "    => segment " << seg->name()
                  << " / base = " << std::hex << seg->baseAddress()
                  << " / size = " << seg->size() << std::endl; 
    }

    init(names);
}

tmpl(/**/)::~VciMultiTty()
{
	for (unsigned int i=0; i<m_term.size(); i++ )
        delete m_term[i];

	delete[] p_irq;
}

tmpl(void)::print_stats(){
    std::cout << name() << std::endl;
    std::cout << "- READ               = " << m_cpt_read << std::endl;
    std::cout << "- WRITE              = " << m_cpt_write << std::endl;
}


}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

