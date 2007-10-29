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
 */
#ifndef SOCLIB_VCI_SIMHELPER_H
#define SOCLIB_VCI_SIMHELPER_H

#include <systemc>
#include "caba/util/vci_target_fsm.h"
#include "caba/util/base_module.h"
#include "common/mapping_table.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciSimhelper
	: public caba::BaseModule
{
private:
    soclib::caba::VciTargetFsm<vci_param, true, 1> m_vci_fsm;

    bool on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be);
    bool on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data);
    void transition();
    void genMoore();

protected:
    SC_HAS_PROCESS(VciSimhelper);

public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
    soclib::caba::VciTarget<vci_param> p_vci;

	VciSimhelper(
		sc_module_name name,
		const soclib::common::IntTab &index,
		const soclib::common::MappingTable &mt);
};

}}

#endif /* SOCLIB_VCI_SIMHELPER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

