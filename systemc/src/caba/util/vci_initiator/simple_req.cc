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

#include "caba/util/vci_initiator_fsm.h"
#include "common/register.h"
#include "common/base_module.h"

namespace soclib {
namespace caba {

#undef tmpl
#define tmpl(x) template<typename vci_param>\
x VciInitiatorSimpleReq<vci_param>

tmpl(/**/)::VciInitiatorSimpleReq(
    uint8_t *local_buffer, uint32_t base_addr, size_t len )
    : m_dest_buffer(local_buffer),
      m_base_addr(base_addr),
      m_len(len),
      m_cmd_ptr(0),
      m_rsp_ptr(0)
{
}

tmpl(/**/)::~VciInitiatorSimpleReq()
{
}

tmpl(void)::cmdOk()
{
    VciInitiatorReq<vci_param>::cmdOk();
    m_cmd_ptr = next_addr(m_cmd_ptr);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

