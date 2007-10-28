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
x VciInitiatorFsm<vci_param>

tmpl(/**/)::VciInitiatorFsm(
    soclib::caba::VciInitiator<vci_param> &vci,
    const uint32_t index )
    : p_vci(vci),
      m_ident( index ),
      m_current_req(NULL)
{
}

tmpl(void)::doReq( VciInitiatorReq<vci_param> *req )
{
    m_current_req = req;
    m_current_req_gone = false;
}

tmpl(/**/)::~VciInitiatorFsm()
{
}

tmpl(void)::reset()
{
    m_current_req = NULL;
}

tmpl(void)::transition()
{
	if ( m_current_req == NULL )
        return;

    if ( p_vci.cmdval.read() &&
         p_vci.cmdack.read() ) {
        m_current_req->cmdOk();
        if ( p_vci.eop.read() )
            m_current_req_gone = true;
    }

    if ( p_vci.rspval.read() &&
         p_vci.rspack.read() ) {
        m_current_req->gotRsp( p_vci );
        if ( p_vci.reop.read() )
            m_current_req = NULL;
    }
}

tmpl(void)::genMoore()
{
    if ( m_current_req && !m_current_req_gone ) {
        m_current_req->putCmd( p_vci, m_ident );
        p_vci.rspack = true;
    } else {
        p_vci.cmdval = false;
        p_vci.rspack = m_current_req != NULL;
    }
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

