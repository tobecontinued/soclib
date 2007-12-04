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
 *         Nicolas Pouillon <nipo@ssji.net>, 2006
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_MULTI_RAM_H
#define SOCLIB_CABA_MULTI_RAM_H

#include <systemc>
#include "caba/util/vci_target_fsm.h"
#include "caba/util/base_module.h"
#include "common/mapping_table.h"
#include "common/elf_loader.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciMultiRam
	: public soclib::caba::BaseModule
{
    soclib::caba::VciTargetFsm<vci_param,true,2,true> m_vci_fsm;
    soclib::common::ElfLoader m_loader;

	typedef unsigned int ram_t;
    ram_t **m_contents;

protected:
	SC_HAS_PROCESS(VciMultiRam);

public:
    typedef typename vci_param::addr_t vci_addr_t;
    typedef typename vci_param::data_t vci_data_t;

    sc_in<bool> p_resetn;
    sc_in<bool> p_clk;
    soclib::caba::VciTarget<vci_param> p_vci;

    VciMultiRam(
        sc_module_name insname,
        const IntTab &index,
        const MappingTable &mt,
        soclib::common::ElfLoader &loader);
    ~VciMultiRam();

private:
    bool on_write(size_t seg, vci_addr_t addr, vci_data_t data, int be);
    bool on_read( size_t seg, vci_addr_t addr, vci_data_t &data );
    void transition();
    void genMoore();
	void reload();
	void reset();

	void trace(sc_trace_file &tf, const std::string base_name, unsigned int what);
};

}}

#endif /* SOCLIB_CABA_MULTI_RAM_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

