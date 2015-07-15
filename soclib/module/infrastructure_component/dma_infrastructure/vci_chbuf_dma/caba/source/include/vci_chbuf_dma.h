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
#ifndef SOCLIB_VCI_CHBUF_DMA_H
#define SOCLIB_VCI_CHBUF_DMA_H

#include <stdint.h>
#include <systemc>
#include "vci_target.h"
#include "vci_initiator.h"
#include "caba_base_module.h"
#include "mapping_table.h"
#include "../../../../../../../lib/generic_fifo/include/caba/generic_fifo.h"

namespace soclib {
namespace caba {

using namespace sc_core;

template<typename vci_param>
class VciChbufDma
	: public caba::BaseModule
{
private:

    // methods
    void transition();
    void genMoore();

    // registers
    sc_signal<int>			r_tgt_fsm;
    sc_signal<typename vci_param::srcid_t>	r_tgt_srcid;
    sc_signal<typename vci_param::trdid_t>	r_tgt_trdid;
    sc_signal<typename vci_param::pktid_t>	r_tgt_pktid;
    sc_signal<typename vci_param::data_t>	r_tgt_rdata;

    sc_signal<int>*	        r_channel_fsm;          // channel state
    sc_signal<bool>*        r_channel_run;          // channel running
    sc_signal<uint32_t>*    r_channel_buf_size;     // single buffer size (bytes)

    sc_signal<uint64_t>*    r_channel_src_desc;     // SRC descriptor base address
    sc_signal<uint32_t>*    r_channel_src_nbufs;    // number of SRC buffers
    sc_signal<uint32_t>*    r_channel_src_buf_addr; // current SRC buffer 32 LSB address
    sc_signal<uint32_t>*    r_channel_src_sts_addr; // current SRC buffer status 32 LSB address
    sc_signal<uint32_t>*    r_channel_src_ext;      // current SRC buffer and status address extension
    sc_signal<uint32_t>*    r_channel_src_index;    // current SRC buffer index
    sc_signal<bool>*        r_channel_src_full;     // current SRC buffer status

    sc_signal<uint64_t>*    r_channel_dst_desc;     // DST descriptor address
    sc_signal<uint32_t>*    r_channel_dst_nbufs;    // number of DST buffers
    sc_signal<uint32_t>*    r_channel_dst_buf_addr; // current DST buffer 32 LSB address
    sc_signal<uint32_t>*    r_channel_dst_sts_addr; // current DST buffer status 32 LSB address
    sc_signal<uint32_t>*    r_channel_dst_ext;      // current SRC buffer and status address extension
    sc_signal<uint32_t>*    r_channel_dst_index;    // current DST buffer index
    sc_signal<bool>*        r_channel_dst_full;     // current DST buffer status 

    sc_signal<uint32_t>*    r_channel_timer;        // cycle counter for polling
    sc_signal<uint32_t>*    r_channel_period;       // status polling period    
    sc_signal<uint32_t>*    r_channel_todo_bytes;   // number of bytes to transfer
    sc_signal<uint32_t>**   r_channel_bytes;        // burst length
    sc_signal<uint32_t>*    r_channel_burst_id;     // current burst index
    sc_signal<bool>*        r_channel_vci_req;      // valid request to CMD FSM
    sc_signal<int>*	        r_channel_vci_req_type; // request type  to CMD FSM

    // valid response from RSP FSM
    sc_signal<bool>*        r_channel_vci_rsp;      // valid complete response from RSP FSM

    // specific response from RSP FSM for burst read transactions
    // rsp_read[k][burst_id] = true if valid frist flit of response for READ transaction number burst_id
    sc_signal<bool>**       r_channel_rsp_read;

    // specific response from RSP FSM for burst write transactions
    // rsp_write[k][burst_id] = true if valid response for WRITE transaction number burst_id
    sc_signal<bool>**       r_channel_rsp_write;
       
    sc_signal<bool>*        r_channel_vci_error;    // error signaled from RSP FSM 
    sc_signal<bool>*        r_channel_last;         // last transaction

    GenericFifo<uint64_t>*  r_channel_fifo;         // Hardware FIFO where data is stored

    sc_signal<int>          r_cmd_fsm;
    sc_signal<size_t>       r_cmd_count;	        // bytes counter (shared)
    sc_signal<uint64_t>	    r_cmd_address;	        // VCI address for a command
    sc_signal<uint32_t>     r_cmd_channel;          // channel index for a command
    sc_signal<uint32_t>     r_cmd_bytes;            // VCI packet length

    sc_signal<int>          r_rsp_fsm;
    sc_signal<size_t>*      r_rsp_count;	        // bytes counter (one per channel)
    sc_signal<uint32_t>     r_rsp_channel;	        // channel index for a response
    sc_signal<uint32_t>     r_rsp_bytes;            // VCI packet length
    sc_signal<uint32_t>*    r_rsp_next_read;        // next expected read response burst index
    sc_signal<uint32_t>*    r_rsp_next_write;       // next expected write response burst index

    // sructural parameters
    std::list<soclib::common::Segment>	m_seglist;
    const uint32_t			            m_burst_max_length;	// number of bytes
    const uint32_t                      m_pipelined_bursts; // number of pipelined bursts
    const uint32_t			            m_channels;		    // no more than 8
    const uint32_t			            m_srcid;            // DMA component SRCID

    enum vci_req_type_e 
    {
        REQ_READ_SRC_DESC,
        REQ_READ_SRC_STATUS,
        REQ_READ_DST_DESC,
        REQ_READ_DST_STATUS,
        REQ_READ_DATA,
        REQ_WRITE_DATA,
        REQ_WRITE_SRC_STATUS,
        REQ_WRITE_DST_STATUS,
    };

protected:
    SC_HAS_PROCESS(VciChbufDma);

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

        CHANNEL_DATA_ERROR,
        CHANNEL_SRC_DESC_ERROR,
        CHANNEL_DST_DESC_ERROR,
        CHANNEL_SRC_STATUS_ERROR,
        CHANNEL_DST_STATUS_ERROR,

        CHANNEL_READ_SRC_DESC,
        CHANNEL_READ_SRC_DESC_WAIT,
        CHANNEL_READ_SRC_STATUS,
        CHANNEL_READ_SRC_STATUS_WAIT,
        CHANNEL_READ_SRC_STATUS_DELAY,

        CHANNEL_READ_DST_DESC,
        CHANNEL_READ_DST_DESC_WAIT,
        CHANNEL_READ_DST_STATUS,
        CHANNEL_READ_DST_STATUS_WAIT,
        CHANNEL_READ_DST_STATUS_DELAY,

        CHANNEL_BURST,
        CHANNEL_READ_REQ,
        CHANNEL_READ_REQ_WAIT,
        CHANNEL_RSP_WAIT,
        CHANNEL_WRITE_REQ,
        CHANNEL_WRITE_REQ_WAIT,
        CHANNEL_BURST_RSP_WAIT,

        CHANNEL_SRC_STATUS_WRITE,
        CHANNEL_SRC_STATUS_WRITE_WAIT,
        CHANNEL_DST_STATUS_WRITE,
        CHANNEL_DST_STATUS_WRITE_WAIT,
        CHANNEL_NEXT_BUFFERS,
    };
    enum cmd_fsm_state_e {
        CMD_IDLE,
        CMD_READ,
        CMD_WRITE,
    };
    enum rsp_fsm_state_e {
        RSP_IDLE,
        RSP_READ_SRC_DESC,
        RSP_READ_SRC_STATUS,
        RSP_READ_DST_DESC,
        RSP_READ_DST_STATUS,
        RSP_READ_DATA,
        RSP_WRITE,
    };

    // ports
    sc_in<bool> 				            p_clk;
    sc_in<bool> 				            p_resetn;
    soclib::caba::VciTarget<vci_param> 		p_vci_target;
    soclib::caba::VciInitiator<vci_param> 	p_vci_initiator;
    sc_out<bool>* 				            p_irq;

    void print_trace();

    VciChbufDma( sc_module_name 			name,
		const soclib::common::MappingTable 	&mt,
		const soclib::common::IntTab 		&srcid,
		const soclib::common::IntTab 		&tgtid,
		const uint32_t 				        burst_max_length,
        const uint32_t				        channels,
        const uint32_t                      pipelined_bursts = 4 );
    ~VciChbufDma();
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

