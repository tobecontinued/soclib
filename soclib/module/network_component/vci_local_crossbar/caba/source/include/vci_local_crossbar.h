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
 *         Nicolas Pouillon <nipo@ssji.net>, 2008
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_VCI_LOCAL_CROSSBAR_H_
#define SOCLIB_CABA_VCI_LOCAL_CROSSBAR_H_

#include <systemc>
#include "../include/vci_simple_crossbar.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciLocalCrossbar
    : public VciSimpleCrossbar<vci_param>
{
public:
    soclib::caba::VciInitiator<vci_param> &p_initiator_to_up;
    soclib::caba::VciTarget<vci_param> &p_target_to_up;

protected:
    SC_HAS_PROCESS(VciLocalCrossbar);

public:
    VciLocalCrossbar( sc_module_name name,
					  const soclib::common::MappingTable &mt,
					  const soclib::common::IntTab &index,
					  size_t nb_initiat,
					  size_t nb_target );
};

}}

#endif /* SOCLIB_CABA_VCI_LOCAL_CROSSBAR_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
