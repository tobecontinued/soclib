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
 *         alain.greiner@lip6.fr
 *
 * Maintainers: alain
 */
#ifndef SOCLIB_VCI_MULTI_DMA_H
#define SOCLIB_VCI_MULTI_DMA_H

#include <stdint.h>
#include <systemc>
#include "caba_base_module.h"
#include "mapping_table.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciMultiDma
	: public caba::BaseModule
{
private:

    // methods
    void transition();
    void genMoore();

    // registers
    sc_signal<int>				r_tgt_fsm;
    sc_signal<typename vci_param::srcid_t>	r_srcid;
    sc_signal<typename vci_param::trdid_t>	r_trdid;
    sc_signal<typename vci_param::pktid_t>	r_pktid;
    sc_signal<typename vci_param::data_t>	r_rdata;

    sc_signal<bool>*				r_activate;		// channel[k] activated

    sc_signal<int>*				r_channel_fsm;		// channel[k] state
    sc_signal<typename vci_param::addr_t>* 	r_src_addr;		// source address for channel[k]
    sc_signal<typename vci_param::addr_t>* 	r_dst_addr;		// dest address for channel[k]
    sc_signal<size_t>*				r_length;		// buffer length (bytes) for k	
    sc_signal<typename vci_param::data_t>**	r_buf;			// local bufferfor channel[k]

    sc_signal<bool>*				r_done;			// transfer completed for [k]
    sc_signal<bool>*				r_error;		// VCI error signaled for [k]

    sc_signal<int>				r_cmd_fsm;
    sc_signal<size_t>				r_cmd_count;		// bytes counter for a command
    sc_signal<size_t>				r_cmd_index;		// channel index for a command
    sc_signal<size_t>				r_cmd_length;		// actual write burst length (bytes)

    sc_signal<int>				r_rsp_fsm;
    sc_signal<size_t>				r_rsp_count;		// bytes counter for a response
    sc_signal<size_t>				r_rsp_index;		// channel index for a response
    sc_signal<size_t>				r_rsp_length;		// actual read burst length (bytes)


    // sructural parameters
    soclib::common::Segment			m_segment;
    const size_t				m_burst_max_length;	// number of bytes
    const size_t				m_channels;		// no more than 16
    const size_t				m_srcid;

protected:
    SC_HAS_PROCESS(VciMultiDma);

public:
    // FSM states
    enum tgt_fsm_state_e {
        TGT_IDLE,
        TGT_READ,
        TGT_WRITE,
        TGT_ERROR,
    };
    enum channel_fsm_state_e {
        CHANNEL_IDLE,
        CHANNEL_READ_REQ,
        CHANNEL_READ_WAIT,
        CHANNEL_WRITE_REQ,
        CHANNEL_WRITE_WAIT,
        CHANNEL_DONE,
        CHANNEL_ERROR,
    };
    enum cmd_fsm_state_e {
        CMD_IDLE,
        CMD_READ,
        CMD_WRITE,
    };
    enum rsp_fsm_state_e {
        RSP_IDLE,
        RSP_READ,
        RSP_WRITE,
    };

    // ports
    sc_in<bool> 				p_clk;
    sc_in<bool> 				p_resetn;
    soclib::caba::VciTarget<vci_param> 		p_vci_target;
    soclib::caba::VciInitiator<vci_param> 	p_vci_initiator;
    sc_out<bool>* 				p_irq;

    void print_trace();

    VciMultiDma( sc_module_name 			name,
		const soclib::common::MappingTable 	&mt,
		const soclib::common::IntTab 		&srcid,
		const soclib::common::IntTab 		&tgtid,
		const size_t 				burst_max_length,
                const size_t				channels);
};

}}

#endif /* SOCLIB_VCI_MULTI_DMA_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

