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
 * Copyright (c) UPMC, Lip6, SoC
 *         Alain Greiner <alain.greiner@lip6.fr>, 2008
 *
 * Maintainers: alain
 */
 
#ifndef SOCLIB_CABA_VCI_CC_XCACHE_WRAPPER_V4_H
#define SOCLIB_CABA_VCI_CC_XCACHE_WRAPPER_V4_H

#include <inttypes.h>
#include <systemc>
#include "caba_base_module.h"
#include "write_buffer.h"
#include "generic_cache.h"
#include "vci_initiator.h"
#include "vci_target.h"
#include "mapping_table.h"
#include "static_assert.h"


namespace soclib {
namespace caba {

using namespace sc_core;

////////////////////////////////////////////
template<typename vci_param, typename iss_t>
class VciCcXCacheWrapperV4
///////////////////////////////////////////
    : public soclib::caba::BaseModule
{
    typedef sc_dt::sc_uint<40> addr_40;
    typedef uint32_t    data_t;
    typedef uint32_t    tag_t;
    typedef uint32_t    be_t;
    typedef typename vci_param::fast_addr_t vci_addr_t;
    enum dcache_fsm_state_e {
        DCACHE_IDLE,
        DCACHE_WRITE_UPDT,
        DCACHE_WRITE_REQ,
        DCACHE_MISS_WAIT,
        DCACHE_MISS_UPDT,
        DCACHE_UNC_WAIT,
        DCACHE_SC_WAIT,
        DCACHE_INVAL,
        DCACHE_ERROR,
        DCACHE_CC_CHECK,
        DCACHE_CC_INVAL,
        DCACHE_CC_UPDT,
        DCACHE_CC_CLEANUP,
    };

    enum icache_fsm_state_e {
        ICACHE_IDLE,
        ICACHE_MISS_WAIT,
        ICACHE_MISS_UPDT,
        ICACHE_UNC_WAIT,
        ICACHE_ERROR,
        ICACHE_CC_CLEANUP, 
        ICACHE_CC_CHECK,
        ICACHE_CC_INVAL,
        ICACHE_CC_UPDT,
    };

    enum cmd_fsm_state_e {
        CMD_IDLE,
        CMD_INS_MISS,
        CMD_INS_UNC,
        CMD_DATA_MISS,
        CMD_DATA_UNC,
        CMD_DATA_WRITE,
        CMD_DATA_SC,
        CMD_INS_CLEANUP,
        CMD_DATA_CLEANUP,
    };

    enum rsp_fsm_state_e {
        RSP_IDLE,
        RSP_INS_MISS,
        RSP_INS_UNC,
        RSP_DATA_MISS,
        RSP_DATA_UNC,
        RSP_DATA_WRITE,
        RSP_DATA_SC,
        RSP_INS_CLEANUP,
        RSP_DATA_CLEANUP,
    };

    enum tgt_fsm_state_e {
        TGT_IDLE,
        TGT_UPDT_WORD,
        TGT_UPDT_DATA,
        TGT_REQ_BROADCAST,
        TGT_REQ_ICACHE,
        TGT_REQ_DCACHE,
        TGT_RSP_BROADCAST,
        TGT_RSP_ICACHE,
        TGT_RSP_DCACHE,
    };

public:

    // PORTS
    sc_in<bool>                             p_clk;
    sc_in<bool>                             p_resetn;
    sc_in<bool>                             p_irq[iss_t::n_irq];
    soclib::caba::VciInitiator<vci_param>   p_vci_ini_rw;
    soclib::caba::VciInitiator<vci_param>   p_vci_ini_c;
    soclib::caba::VciTarget<vci_param>      p_vci_tgt;

private:

    // STRUCTURAL PARAMETERS
    const soclib::common::AddressDecodingTable<vci_addr_t, bool>    m_cacheability_table;
    const soclib::common::Segment                                   m_segment;
    iss_t               m_iss;
    const uint32_t      m_srcid_rw;   
    const uint32_t      m_srcid_c;   
    
    const size_t        m_dcache_ways;
    const size_t        m_dcache_words;
    const size_t        m_dcache_yzmask;
    const size_t        m_icache_ways;
    const size_t        m_icache_words;
    const size_t        m_icache_yzmask;

    // REGISTERS
    sc_signal<int>          r_dcache_fsm;
    sc_signal<int>          r_dcache_fsm_save;
    sc_signal<addr_40>      r_dcache_addr_save;
    sc_signal<data_t>       r_dcache_wdata_save;
    sc_signal<data_t>       r_dcache_rdata_save;
    sc_signal<uint64_t>     r_dcache_ll_data;
    sc_signal<addr_40>      r_dcache_ll_addr;
    sc_signal<bool>         r_dcache_ll_valid;
    sc_signal<int>          r_dcache_type_save;
    sc_signal<be_t>         r_dcache_be_save;
    sc_signal<bool>         r_dcache_cached_save;
    sc_signal<bool>         r_dcache_cleanup_req;
    sc_signal<addr_40>      r_dcache_cleanup_line;
    sc_signal<bool>         r_dcache_miss_req;
    sc_signal<bool>         r_dcache_unc_req;
    sc_signal<bool>         r_dcache_sc_req;
    sc_signal<bool>         r_dcache_write_req;
    sc_signal<bool>         r_dcache_inval_rsp;

    sc_signal<int>          r_icache_fsm;
    sc_signal<int>          r_icache_fsm_save;
    sc_signal<addr_40>      r_icache_addr_save;
    sc_signal<bool>         r_icache_miss_req;
    sc_signal<bool>         r_icache_unc_req;
    sc_signal<bool>         r_icache_cleanup_req;
    sc_signal<addr_40>      r_icache_cleanup_line;
    sc_signal<bool>         r_icache_inval_rsp;

    sc_signal<int>          r_vci_cmd_fsm;
    sc_signal<size_t>       r_vci_cmd_min;       
    sc_signal<size_t>       r_vci_cmd_max;       
    sc_signal<size_t>       r_vci_cmd_cpt;       
      
    sc_signal<int>          r_vci_rsp_fsm;
    sc_signal<bool>         r_vci_rsp_ins_error;    
    sc_signal<bool>         r_vci_rsp_data_error;    
    sc_signal<size_t>       r_vci_rsp_cpt;  

    data_t                  *r_icache_miss_buf;    
    data_t                  *r_dcache_miss_buf;    
    sc_signal<bool>         r_icache_buf_unc_valid;

    data_t                  *r_tgt_buf;
    be_t                    *r_tgt_be;

    sc_signal<int>          r_vci_tgt_fsm;
    sc_signal<addr_40>       r_tgt_addr;
    sc_signal<size_t>       r_tgt_word;
    sc_signal<bool>         r_tgt_update;
    sc_signal<bool>         r_tgt_update_data;
    sc_signal<bool>         r_tgt_brdcast;
    sc_signal<size_t>       r_tgt_srcid;
    sc_signal<size_t>       r_tgt_pktid;
    sc_signal<size_t>       r_tgt_trdid;
    sc_signal<size_t>       r_tgt_plen;
    sc_signal<bool>         r_tgt_icache_req;
    sc_signal<bool>         r_tgt_dcache_req;
    sc_signal<bool>         r_tgt_icache_rsp;
    sc_signal<bool>         r_tgt_dcache_rsp;

    WriteBuffer<addr_40>        r_wbuf;
    GenericCache<vci_addr_t>    r_icache;
    GenericCache<vci_addr_t>    r_dcache;

    // Activity counters
    uint32_t m_cpt_dcache_data_read;        // DCACHE DATA READ
    uint32_t m_cpt_dcache_data_write;       // DCACHE DATA WRITE
    uint32_t m_cpt_dcache_dir_read;         // DCACHE DIR READ
    uint32_t m_cpt_dcache_dir_write;        // DCACHE DIR WRITE

    uint32_t m_cpt_icache_data_read;        // ICACHE DATA READ
    uint32_t m_cpt_icache_data_write;       // ICACHE DATA WRITE
    uint32_t m_cpt_icache_dir_read;         // ICACHE DIR READ
    uint32_t m_cpt_icache_dir_write;        // ICACHE DIR WRITE

    uint32_t m_cpt_cc_update;               // number of coherence update packets 
    uint32_t m_cpt_cc_inval;                // number of coherence inval packets

    uint32_t m_cpt_frz_cycles;	            // number of cycles where the cpu is frozen
    uint32_t m_cpt_total_cycles;	        // total number of cycles 

    uint32_t m_cpt_read;                    // total number of read instructions
    uint32_t m_cpt_write;                   // total number of write instructions
    uint32_t m_cpt_data_miss;               // number of read miss
    uint32_t m_cpt_ins_miss;                // number of instruction miss
    uint32_t m_cpt_unc_read;                // number of read uncached
    uint32_t m_cpt_write_cached;            // number of cached write

    uint32_t m_cost_write_frz;              // number of frozen cycles related to write buffer         
    uint32_t m_cost_data_miss_frz;          // number of frozen cycles related to data miss
    uint32_t m_cost_unc_read_frz;           // number of frozen cycles related to uncached read
    uint32_t m_cost_ins_miss_frz;           // number of frozen cycles related to ins miss

    uint32_t m_cpt_imiss_transaction;       // number of VCI instruction miss transactions
    uint32_t m_cpt_dmiss_transaction;       // number of VCI data miss transactions
    uint32_t m_cpt_unc_transaction;         // number of VCI uncached read transactions
    uint32_t m_cpt_write_transaction;       // number of VCI write transactions

    uint32_t m_cost_imiss_transaction;      // cumulated duration for VCI IMISS transactions
    uint32_t m_cost_dmiss_transaction;      // cumulated duration for VCI DMISS transactions
    uint32_t m_cost_unc_transaction;        // cumulated duration for VCI UNC transactions
    uint32_t m_cost_write_transaction;      // cumulated duration for VCI WRITE transactions
    uint32_t m_length_write_transaction;    // cumulated length for VCI WRITE transactions

protected:
    SC_HAS_PROCESS(VciCcXCacheWrapperV4);

public:

    VciCcXCacheWrapperV4(
                       sc_module_name insname,
                       int proc_id,
                       const soclib::common::MappingTable &mtp,
                       const soclib::common::MappingTable &mtc,
                       const soclib::common::IntTab &initiator_index_rw,
                       const soclib::common::IntTab &initiator_index_c,
                       const soclib::common::IntTab &target_index,
                       size_t icache_ways,
                       size_t icache_sets,
                       size_t icache_words,
                       size_t dcache_ways,
                       size_t dcache_sets,
                       size_t dcache_words );

    ~VciCcXCacheWrapperV4();

    void print_cpi();
    void print_stats();

private:

    void transition();
    void genMoore();

    soclib_static_assert((int)iss_t::SC_ATOMIC == (int)vci_param::STORE_COND_ATOMIC);
    soclib_static_assert((int)iss_t::SC_NOT_ATOMIC == (int)vci_param::STORE_COND_NOT_ATOMIC);
};

}}

#endif /* SOCLIB_CABA_VCI_CC_XCACHE_WRAPPER_V4_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



