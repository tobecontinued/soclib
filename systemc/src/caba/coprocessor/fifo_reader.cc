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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "caba/coprocessor/fifo_reader.h"
#include "common/register.h"
#include "common/base_module.h"
#include "common/endian.h"

namespace soclib {
namespace caba {

#define tmpl(x) template<typename word_t> x FifoReader<word_t>

tmpl(/**/)::FifoReader(
	sc_module_name insname,
    const std::string &bin,
    const std::vector<std::string> &argv )
	: soclib::caba::BaseModule(insname),
      m_wrapper( bin, argv )
{
	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();

	m_buffer = 0;
}

tmpl(void)::transition()
{
	if ( p_resetn == false )
	{
		m_woffset = 0;
		m_usage = 0;
		return;
	}

	if ( p_fifo.wok ) {
		// Pas important de savoir si il y en avait un ou pas
		m_usage = 0;
	}

	if ( !m_usage ) {
        unsigned int status;
        
		status = m_wrapper.read(((char*)&m_buffer)+m_woffset, sizeof(word_t)-m_woffset);
        if ( status <= sizeof(word_t) ) {
			m_woffset += status;
			if ( m_woffset == sizeof(word_t) ) {
				// Got it
				m_woffset = 0;
				m_usage++;
			}
		}
	}
}

tmpl(void)::genMoore()
{
	p_fifo.w = (bool)m_usage;
	p_fifo.data = machine_to_le((word_t)m_buffer);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

