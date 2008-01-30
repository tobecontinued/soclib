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
#ifndef SOCLIB_CABA_VCI_VGMN_H_
#define SOCLIB_CABA_VCI_VGMN_H_

#include <systemc>
#include "caba_base_module.h"
#include "vci_initiator.h"
#include "vci_target.h"
#include "vci_buffers.h"
#include "address_decoding_table.h"
#include "address_masking_table.h"
#include "mapping_table.h"

namespace soclib { namespace caba {

using namespace sc_core;

namespace _vgmn {

template<typename vci_pkt_t> class OutputPortQueue;
template<typename vci_pkt_t> class InputRouter;
template<typename router_t, typename queue_t> class MicroNetwork;

}

template<typename vci_param>
class VciVgmn
    : public soclib::caba::BaseModule
{
public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;

    soclib::caba::VciInitiator<vci_param> *p_to_target;
    soclib::caba::VciTarget<vci_param> *p_from_initiator;

private:
    size_t m_nb_initiat;
    size_t m_nb_target;
    size_t m_min_latency;
    size_t m_fifo_depth;

    typedef _vgmn::OutputPortQueue<soclib::caba::VciCmdBuffer<vci_param> > cmd_queue_t;
    typedef _vgmn::OutputPortQueue<soclib::caba::VciRspBuffer<vci_param> > rsp_queue_t;
    typedef _vgmn::InputRouter<cmd_queue_t> cmd_router_t;
    typedef _vgmn::InputRouter<rsp_queue_t> rsp_router_t;

    _vgmn::MicroNetwork<cmd_router_t,cmd_queue_t> *m_cmd_mn;
    _vgmn::MicroNetwork<rsp_router_t,rsp_queue_t> *m_rsp_mn;

    void transition();
    void genMoore();

protected:
    SC_HAS_PROCESS(VciVgmn);

public:
    VciVgmn( sc_module_name name,
              const soclib::common::MappingTable &mt,
              size_t nb_initiat,
              size_t nb_target,
              size_t min_latency,
              size_t fifo_depth );
};

}}

#endif /* SOCLIB_CABA_VCI_VGMN_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
