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
x VciInitSimpleReadReq<vci_param>

tmpl(/**/)::VciInitSimpleReadReq(
    uint8_t *local_buffer, uint32_t base_addr, size_t len )
    : VciInitiatorSimpleReq<vci_param>( local_buffer, base_addr, len )
{
}

tmpl(/**/)::~VciInitSimpleReadReq()
{
}

tmpl(bool)::putCmd( VciInitiator<vci_param> &p_vci, uint32_t id ) const
{
    const uint32_t cmd_ptr = VciInitiatorSimpleReq<vci_param>::m_cmd_ptr;
    const uint32_t base_addr = VciInitiatorSimpleReq<vci_param>::m_base_addr;
    const size_t len = VciInitiatorSimpleReq<vci_param>::m_len;
    const uint32_t packet = VciInitiatorReq<vci_param>::m_packet;
    const uint32_t thread = VciInitiatorReq<vci_param>::m_thread;
    bool ending = this->next_addr(cmd_ptr)>=len;

    p_vci.cmdval = true;
    p_vci.address = (base_addr+cmd_ptr)&~3;
	p_vci.be = (1<<vci_param::B)-1;
	p_vci.cmd = VCI_CMD::READ;
	p_vci.contig = 1;
//  p_vci.wdata;
	p_vci.eop = ending;
	p_vci.cons = true;
	p_vci.plen = len;
	p_vci.wrap = 0;
	p_vci.cfixed = 1;
//	TODO: clen
    p_vci.clen = 0;
	p_vci.srcid = id;
	p_vci.trdid = thread;
	p_vci.pktid = packet;

    return ending;
}

tmpl(void)::gotRsp( const VciInitiator<vci_param> &p_vci )
{
    size_t rsp_ptr = VciInitiatorSimpleReq<vci_param>::m_rsp_ptr;
    const size_t len = VciInitiatorSimpleReq<vci_param>::m_len;
    uint8_t * const dest_buffer = VciInitiatorSimpleReq<vci_param>::m_dest_buffer;
    const uint32_t base_addr = VciInitiatorSimpleReq<vci_param>::m_base_addr;

    typename VciInitiatorReq<vci_param>::data_t data = p_vci.rdata;

    const int vci_addr = (base_addr+rsp_ptr)&~3;
    const int delta = vci_addr-base_addr;

    for ( int i=0; i<vci_param::B; ++i ) {
        const int addr = vci_addr+i;
        if ( addr >= (int)base_addr &&
            addr < (int)base_addr+len ) {
            dest_buffer[delta+i] = data;
        }
        data >>= 8;
    }
    
    VciInitiatorSimpleReq<vci_param>::m_rsp_ptr
        = VciInitiatorSimpleReq<vci_param>::next_addr(rsp_ptr);

    VciInitiatorReq<vci_param>::gotRsp( p_vci );
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

