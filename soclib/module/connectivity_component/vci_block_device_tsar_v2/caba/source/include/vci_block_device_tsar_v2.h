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
 *         Eric Guthmuller  <eric.guthmuller@polytechnique.edu>
 *
 * Maintainers: nipo eric.guthmuller@polytechnique.edu
 */
#ifndef SOCLIB_VCI_BLOCK_DEVICE_TSAR_V2_H
#define SOCLIB_VCI_BLOCK_DEVICE_TSAR_V2_H

#include <stdint.h>
#include <systemc>
#include "vci_target_fsm.h"
#include "vci_initiator_fsm.h"
#include "caba_base_module.h"
#include "mapping_table.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciBlockDeviceTsarV2
	: public caba::BaseModule
{
private:
    soclib::caba::VciTargetFsm<vci_param, true> m_vci_target_fsm;
    soclib::caba::VciInitiatorFsm<vci_param> m_vci_init_fsm;
    typedef typename soclib::caba::VciInitiatorReq<vci_param> req_t;

    bool on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be);
    bool on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data);
    void read_done( req_t *req );
    void write_finish( req_t *req );
    void open_finish( req_t *req );
    void next_req();
    void transition();
    void genMoore();

	const uint32_t m_block_size;
    const uint32_t m_burst_size;    // Maximum size of the burst

	int m_fd;
	int m_op;
	uint32_t m_buffer;
	uint32_t m_count;
	uint64_t m_device_size;
	uint32_t m_lba;
    int m_status;
    uint32_t m_chunck_offset;
    uint32_t m_transfer_size;
	bool m_irq_enabled;
	bool r_irq;

	int m_current_op;

	uint8_t *m_data;

	inline void ended(int status);

protected:
    SC_HAS_PROCESS(VciBlockDeviceTsarV2);

public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
    soclib::caba::VciTarget<vci_param> p_vci_target;
    soclib::caba::VciInitiator<vci_param> p_vci_initiator;
    sc_out<bool> p_irq;

	VciBlockDeviceTsarV2(
		sc_module_name name,
		const soclib::common::MappingTable &mt,
		const soclib::common::IntTab &srcid,
		const soclib::common::IntTab &tgtid,
        const std::string &filename,
        const uint32_t block_size = 512,
        const uint32_t burst_size = 64);
};

}}

#endif /* SOCLIB_VCI_BLOCK_DEVICE_TSAR_V2_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
