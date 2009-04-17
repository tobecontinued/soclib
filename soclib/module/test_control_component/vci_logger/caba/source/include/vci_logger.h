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
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_LOGGER_H
#define SOCLIB_CABA_LOGGER_H

#include <systemc>
#include "vci_monitor.h"
#include "caba_base_module.h"
#include "mapping_table.h"

namespace soclib {
namespace caba {

template<typename vci_param> class VciLoggerElem;

template<typename vci_param>
class VciLogger
	: public soclib::caba::BaseModule
{
	VciLoggerElem<vci_param> *m_pending_commands;

protected:
	SC_HAS_PROCESS(VciLogger);

public:
    sc_core::sc_in<bool> p_resetn;
    sc_core::sc_in<bool> p_clk;
    soclib::caba::VciMonitor<vci_param> p_vci;

    VciLogger(
        sc_core::sc_module_name insname,
        const soclib::common::MappingTable &mt);
    ~VciLogger();

private:
    void transition();
};

}}

#endif /* SOCLIB_CABA_LOGGER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

