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
 */

#include <cstdlib>
#include <sstream>

#include "soclib/simhelper.h"
#include "common/register.h"
#include "caba/target/vci_simhelper.h"

namespace soclib { namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciSimhelper<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;

	switch (cell) {
    case SIMHELPER_SC_STOP:
        sc_stop();
        break;
    case SIMHELPER_END_WITH_RETVAL:
        std::cout << "Simulation exiting, retval=" << (uint32_t)data << std::endl;
        ::exit(data);
    case SIMHELPER_EXCEPT_WITH_VAL:
    {
        std::ostringstream o;
        o << "Simulation yielded error level " << (uint32_t)data;
        throw soclib::exception::RunTimeError(o.str());
    }
    case SIMHELPER_PAUSE_SIM:
    {
        std::cout << "Simulation paused, press ENTER" << std::endl;
        std::string a;
        std::cin >> a;
    }
	}
	return true;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
	return false;
}

tmpl(void)::transition()
{
	if (!p_resetn)
		m_vci_fsm.reset();

	m_vci_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();
}

tmpl(/**/)::VciSimhelper(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt)
	: caba::BaseModule(name),
	  m_vci_fsm(p_vci, mt.getSegmentList(index)),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci")
{
	m_vci_fsm.on_read_write(on_read, on_write);

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

