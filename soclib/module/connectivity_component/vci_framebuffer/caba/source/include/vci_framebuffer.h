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
#ifndef SOCLIB_VCI_FRAMEBUFFER_H
#define SOCLIB_VCI_FRAMEBUFFER_H

#include <systemc>
#include "vci_target_fsm.h"
#include "caba_base_module.h"
#include "mapping_table.h"
#include "process_wrapper.h"
#include "fb_controller.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciFrameBuffer
	: public caba::BaseModule
{
private:
    caba::VciTargetFsm<vci_param, true, 2> m_vci_fsm;

    bool on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be);
    bool on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data);
    void transition();
    void genMoore();

	int m_defered_timeout;

	common::FbController m_fb_controller;
protected:
    SC_HAS_PROCESS(VciFrameBuffer);

public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
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

