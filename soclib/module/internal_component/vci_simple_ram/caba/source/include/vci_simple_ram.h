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
 *         Alain Greiner <alain.greiner@lip6.fr>, 2008
 *
 * Maintainers: alain
 */
#ifndef SOCLIB_CABA_VCI_SIMPLE_RAM_H
#define SOCLIB_CABA_VCI_SIMPLE_RAM_H

#include <systemc>
#include <vector>
#include <list>
#include <cassert>
#include "caba_base_module.h"
#include "vci_target.h"
#include "mapping_table.h"
#include "int_tab.h"
#include "loader.h"
#include "linked_access_buffer.h"
#include "soclib_endian.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciSimpleRam
	: public soclib::caba::BaseModule
{
public:

	typedef typename vci_param::data_t  vci_data_t;
	typedef typename vci_param::addr_t  vci_addr_t;
	typedef typename vci_param::be_t    vci_be_t;
	typedef typename vci_param::srcid_t vci_srcid_t;
	typedef typename vci_param::trdid_t vci_trdid_t;
	typedef typename vci_param::pktid_t vci_pktid_t;

	typedef unsigned int       ram_t;

    enum fsm_state_e {
        FSM_IDLE,
        FSM_WRITE_BURST,
        FSM_WRITE_BURST_RSP,
        FSM_READ_WORD,
        FSM_READ_BURST,
        FSM_SC,
        FSM_LL,
        FSM_ERROR,
    };

private:

    soclib::common::Loader                  m_loader;
    std::list<soclib::common::Segment>      m_seglist;
    soclib::common::LinkedAccessBuffer<
        vci_addr_t, vci_srcid_t>            r_llsc_buf;

    sc_signal<int>                          r_fsm_state;
    sc_signal<size_t>                       r_read_count;
    sc_signal<size_t>                       r_index;
    sc_signal<vci_addr_t>                   r_address;
    sc_signal<vci_data_t>                   r_wdata;
    sc_signal<vci_be_t>                     r_be;
    sc_signal<vci_srcid_t>                  r_srcid;
    sc_signal<vci_trdid_t>                  r_trdid;
    sc_signal<vci_pktid_t>                  r_pktid;
    sc_signal<bool>                         r_contig;
    sc_signal<bool>                         r_eop_cmd;
    sc_signal<bool>                         r_valid;
    sc_signal<bool>                         r_eop_rsp;

    size_t                                  m_nbseg;
    ram_t                                   **m_ram;
    soclib::common::Segment                 **m_seg;

protected:

	SC_HAS_PROCESS(VciSimpleRam);

public:

    // Ports
    sc_in<bool>                             p_resetn;
    sc_in<bool>                             p_clk;
    soclib::caba::VciTarget<vci_param>      p_vci;

    VciSimpleRam(sc_module_name insname,
                 const soclib::common::IntTab index,
                 const soclib::common::MappingTable &mt,
                 const soclib::common::Loader &loader);

    ~VciSimpleRam();

private:

    bool write(size_t seg, vci_addr_t addr, vci_data_t wdata, vci_be_t be);
    bool read( size_t seg, vci_addr_t addr, vci_data_t &rdata );
    void transition();
    void genMoore();
	void reload();
	void reset();

    // Activity counters
    uint32_t m_cpt_read;   // Count READ access
    uint32_t m_cpt_write;  // Count WRITE access
};

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

