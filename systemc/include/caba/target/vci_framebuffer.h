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
#ifndef SOCLIB_VCI_FRAMEBUFFER_H
#define SOCLIB_VCI_FRAMEBUFFER_H

#include <systemc.h>
#include "caba/util/vci_target_fsm.h"
#include "caba/util/base_module.h"
#include "common/mapping_table.h"
#include "common/process_wrapper.h"
#include "common/fb_controller.h"

namespace soclib {
namespace caba {

using namespace soclib;

template<typename vci_param>
class VciFrameBuffer
	: public caba::BaseModule
{
private:
    caba::VciTargetFsmHandler<vci_param, true, 2> m_vci_fsm;

    bool on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be);
    bool on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data);
    void transition();
    void genMoore();

	int m_defered_timeout;

	unsigned long m_width, m_height;

	common::FbController m_fb_controller;
protected:
    SC_HAS_PROCESS(VciFrameBuffer);

public:
    sc_signal<bool> p_clk;
    sc_signal<bool> p_resetn;
    caba::VciTarget<vci_param> p_vci;

	VciFrameBuffer(
		sc_module_name name,
		const IntTab &index,
		const MappingTable &mt,
		unsigned long width,
		unsigned long height);

    ~VciFrameBuffer();

	void trace(sc_trace_file &tf, const std::string base_name, unsigned int what);
};

}}

#endif /* SOCLIB_VCI_FRAMEBUFFER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

