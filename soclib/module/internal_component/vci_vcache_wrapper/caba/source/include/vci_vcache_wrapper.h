/* -*- c++ -*-
 * File : vci_vcache_wrapper.h
 * Copyright (c) UPMC, Lip6, SoC
 * Authors : Alain Greiner, Yang GAO
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
 */
 
#ifndef SOCLIB_CABA_VCI_VCACHE_WRAPPER_H
#define SOCLIB_CABA_VCI_VCACHE_WRAPPER_H

#include <inttypes.h>
#include <systemc>
#include "caba_base_module.h"
#include "write_buffer.h"
#include "generic_cache.h"
#include "vci_initiator.h"
#include "mapping_table.h"
#include "generic_tlb.h"

namespace soclib {
namespace caba {

using namespace sc_core;

////////////////////////////////////////////
template<typename vci_param, typename iss_t>
class VciVCacheWrapper
////////////////////////////////////////////
    : public soclib::caba::BaseModule
{
    typedef uint32_t addr_t;
    typedef uint32_t data_t;
    typedef uint32_t tag_t;
    typedef uint32_t be_t;
    typedef uint32_t type_t;
    typedef typename iss_t::DataOperationType data_op_t;
    typedef sc_dt::sc_uint<36> addr36_t;

    enum ivcache_fsm_state_e {  
        IVCACHE_CACHE_INIT, // 00.instruction vcache cache init
        IVCACHE_TLB_INIT,   // 01.instruction vcache tlb init
        IVCACHE_IDLE,       // 02.instruction vcache idle
        IVCACHE_BIS,        // 03.instruction vcache redo in cache
        IVCACHE_TLB1_WAIT,  // 04.instruction vcache tlb miss ptd or pte read wait
        IVCACHE_T1_LL_WAIT, // 05.instruction vcache tlb miss ll response wait 
        IVCACHE_T1_SC_WAIT, // 06.instruction vcache tlb miss sc response wait 
        IVCACHE_TLB1_UPDT,  // 07.instruction vcache tlb update if it has one level page table
        IVCACHE_TLB2_WAIT,  // 08.instruction vcache tlb miss pte read wait
        IVCACHE_T2_LL_WAIT, // 09.instruction vcache tlb miss ll response wait 
        IVCACHE_T2_SC_WAIT, // 0a.instruction vcache tlb miss sc response wait 
        IVCACHE_TLB2_UPDT,  // 0b.instruction vcache tlb update if it has two levels page table
        IVCACHE_TLB_ERROR,  // 0c.instruction vcache tlb error
        IVCACHE_TLB_INVAL,  // 0d.instruction vcache tlb invalid from processor
        IVCACHE_CACHE_INVAL,// 0e.instruction vcache cache invalid from processor
        IVCACHE_MISS_WAIT,  // 0f.instruction vcache cache cached read miss wait
        IVCACHE_MISS_UPDT,  // 10.instruction vcache cache cached read miss update
        IVCACHE_CACHE_ERR,  // 11.instruction vcache cache error
    };

    enum dvcache_fsm_state_e {  
        DVCACHE_CACHE_INIT, // 00.data vcache cache init
        DVCACHE_TLB_INIT,   // 01.data vcache tlb init
        DVCACHE_IDLE,       // 02.data vcache idle
        DVCACHE_BIS,        // 03.data vcache redo in cache
        DVCACHE_TLB1_WAIT,  // 04.data vcache tlb miss ptd or pte read wait
        DVCACHE_T1_LL_WAIT, // 05.data vcache tlb miss ll response wait
        DVCACHE_T1_SC_WAIT, // 06.data vcache tlb miss sc response wait 
        DVCACHE_TLB1_UPDT,  // 07.data vcache tlb update if it has one level page table
        DVCACHE_TLB2_WAIT,  // 08.data vcache tlb miss pte read wait
        DVCACHE_T2_LL_WAIT, // 09.data vcache tlb miss ll response wait
        DVCACHE_T2_SC_WAIT, // 0a.data vcache tlb miss sc response wait 
        DVCACHE_TLB2_UPDT,  // 0b.data vcache tlb update if it has two levels page table 
        DVCACHE_TLB_ERROR,  // 0c.data vcache tlb error
        DVCACHE_TLB_INVAL,  // 0d.data vcache tlb invalid from processor
        DVCACHE_WRITE_UPDT, // 0e.data vcache cache write update
        DVCACHE_WRITE_REQ,  // 0f.data vcache cache write request
        DVCACHE_MISS_WAIT,  // 10.data vcache cache cached read miss wait
        DVCACHE_MISS_UPDT,  // 11.data vcache cache read miss update
        DVCACHE_UNC_WAIT,   // 12.data vcache cache uncached read miss wait
        DVCACHE_CACHE_ERR,  // 13.data vcache cache error
        DVCACHE_CACHE_INVAL,// 14.data vcache cache invalid from processor
    };

    enum cmd_fsm_state_e {      
        CMD_IDLE,           // 00.vci idle
        CMD_ITLB_MISS,      // 01.vci instruction vcache tlb miss ptd or pte request
        CMD_ITLB_LL,        // 02.vci instruction vcache tlb miss linked load request
        CMD_ITLB_SC,        // 03.vci instruction vcache tlb miss store conditional request
        CMD_INS_MISS,       // 04.vci instruction vcache cache miss request
        CMD_DTLB_MISS,      // 05.vci data vcache tlb miss ptd or pte request
        CMD_DTLB_LL,        // 06.vci data vcache tlb miss linked load request
        CMD_DTLB_SC,        // 07.vci data vcache tlb miss store conditional request
        CMD_DATA_UNC,       // 08.vci data vcache cache uncached read miss request
        CMD_DATA_MISS,      // 09.vci data vcache cache read miss request
        CMD_DATA_WRITE,     // 0a.vci data vcache cache write request
    };

    enum rsp_fsm_state_e {       
        RSP_IDLE,           // 00.vci idle
        RSP_ITLB_MISS,      // 01.vci instruction vcache tlb miss read ptd or pte response
        RSP_ITLB_LL,        // 02.vci instruction vcache tlb miss linked load response
        RSP_ITLB_SC,        // 03.vci instruction vcache tlb miss store conditional response
        RSP_INS_MISS,       // 04.vci instruction vcache cache miss
        RSP_DTLB_MISS,      // 05.vci data vcache tlb miss read ptd or pte response
        RSP_DTLB_LL,        // 06.vci data vcache tlb miss linked load response
        RSP_DTLB_SC,        // 07.vci data vcache tlb miss store conditional response
        RSP_DATA_MISS,      // 08.vci data vcache cache read miss response 
        RSP_DATA_UNC,       // 09.vci data vcache cache uncached read miss response
        RSP_DATA_WRITE,     // 0a.vci data vcache cache write response
    };

    enum {
        READ_PKTID,
        WRITE_PKTID,
    };

    // TLB Mode
    enum {
        TLBS_DEACTIVE,  // instruction TLB deactive and data TLB deactive
        ITLB_A_DTLB_D,  // instruction TLB   active and data TLB deactive
        ITLB_D_DTLB_A,  // instruction TLB deactive and data TLB   active
        TLBS_ACTIVE,    // instruction TLB   active and data TLB   active
    };

    // Error Type
    enum xtn_err_type_e {
        XTN_ERR_NONE,       // None
        XTN_ERR_ACC_BUS,    // Access bus error
        XTN_ERR_PROTECTION, // Protection error
        XTN_ERR_PRIVI_VIOLA,// Privilege violation error
        XTN_ERR_INVAL_ADR,  // Invalid address error
        XTN_ERR_TRANS,      // Translation error
        XTN_ERR_INTERNAL,   // Internal error
    };


public:
    sc_in<bool>                             p_clk;
    sc_in<bool>                             p_resetn;
    sc_in<bool>                             p_irq[iss_t::n_irq];
    soclib::caba::VciInitiator<vci_param>   p_vci;

private:
    // STRUCTURAL PARAMETERS
    soclib::common::AddressDecodingTable<uint32_t, bool>    m_cacheability_table;
    const uint32_t                                          m_srcid;
    iss_t                                                   m_iss;   

    const size_t  m_itlb_m_ways;
    const size_t  m_itlb_m_sets;
    const size_t  m_itlb_k_ways;
    const size_t  m_itlb_k_sets;

    const size_t  m_dtlb_m_ways;
    const size_t  m_dtlb_m_sets;
    const size_t  m_dtlb_k_ways;
    const size_t  m_dtlb_k_sets;

    const size_t  m_icache_ways;
    const size_t  m_icache_yzmask;
    const size_t  m_icache_words;

    const size_t  m_dcache_ways;
    const size_t  m_dcache_yzmask;
    const size_t  m_dcache_words;

    // instruction and data vcache tlb instances 
    soclib::caba::GenericTlb<addr36_t>    ivcache_m_tlb;
    soclib::caba::GenericTlb<addr36_t>    ivcache_k_tlb;
    soclib::caba::GenericTlb<addr36_t>    dvcache_m_tlb;
    soclib::caba::GenericTlb<addr36_t>    dvcache_k_tlb;

    sc_signal<addr_t>   r_tlb_ptpr; // page table pointer register
    sc_signal<int>      r_tlb_mode; // tlb mode register

    // REGISTERS
    sc_signal<int>          r_dvcache_fsm;
    sc_signal<addr36_t>     r_dcache_addr_save;
    sc_signal<data_t>       r_dcache_wdata_save;
    sc_signal<data_t>       r_dcache_rdata_save;
    sc_signal<type_t>       r_dcache_type_save;
    sc_signal<be_t>         r_dcache_be_save;
    sc_signal<bool>         r_dcache_cached_save; 
    sc_signal<addr36_t>     r_dtlb_pte_addr;
    sc_signal<addr_t>       r_dtlb_ptpr_save;
    sc_signal<addr_t>       r_dtlb_ptp;
    sc_signal<addr_t>       r_dtlb_id1_save;
    sc_signal<size_t>       r_dtlb_et_save;
    sc_signal<addr_t>       r_dtlb_ppn_save;
    sc_signal<bool>         r_dcache_buf_unc_valid; 

    // request registers
    sc_signal<bool>         r_dcache_miss_req;
    sc_signal<bool>         r_dcache_unc_req;
    sc_signal<bool>         r_dcache_write_req;
    sc_signal<bool>         r_dtlb_req;
    sc_signal<bool>         r_dtlb_ll_req;
    sc_signal<bool>         r_dtlb_sc_req;

    sc_signal<int>          r_ivcache_fsm;
    sc_signal<addr36_t>     r_ivcache_miss_addr;
    sc_signal<data_t>       r_ivcache_miss_data;
    sc_signal<bool>         r_icache_miss_req;
    sc_signal<bool>         r_itlb_req;
    sc_signal<bool>         r_itlb_ll_req;
    sc_signal<bool>         r_itlb_sc_req;
    sc_signal<addr_t>       r_itlb_ptpr_save;
    sc_signal<addr_t>       r_itlb_ptp;
    sc_signal<addr_t>       r_itlb_id1_save;
    sc_signal<size_t>       r_itlb_et_save;
    sc_signal<addr_t>       r_itlb_ppn_save;

    sc_signal<int>          r_vci_cmd_fsm;
    sc_signal<size_t>       r_vci_cmd_min;       
    sc_signal<size_t>       r_vci_cmd_max;       
    sc_signal<size_t>       r_vci_cmd_cpt;     

    sc_signal<int>          r_vci_rsp_fsm;
    sc_signal<size_t>       r_vci_rsp_cpt;
  
    sc_signal<data_t>       r_itlb_miss_rsp;
    sc_signal<data_t>       r_dtlb_miss_rsp;
    sc_signal<bool>         r_ivcache_rsp_error;
    sc_signal<bool>         r_dvcache_rsp_error;

    data_t                  *r_icache_miss_buf;    
    data_t                  *r_dcache_miss_buf;  

    sc_signal<data_t>       r_err_type; // error type register
    sc_signal<data_t>       r_ivcache_err;
    sc_signal<data_t>       r_dvcache_err;

    sc_signal<addr_t>       r_bvar;     // bad virtual address register
    sc_signal<addr_t>       r_i_bvar;     
    sc_signal<addr_t>       r_d_bvar;     

    // special command registers
    sc_signal<bool>         r_context_sw_itlb;
    sc_signal<bool>         r_context_sw_dtlb;
    sc_signal<bool>         r_icache_flush;
    sc_signal<bool>         r_dcache_flush;

    WriteBuffer<addr36_t>     r_wbuf;
    GenericCache<addr36_t>    r_icache;
    GenericCache<addr36_t>    r_dcache;

    // Activity counters
    uint32_t m_cpt_dcache_data_read;        // DCACHE DATA READ
    uint32_t m_cpt_dcache_data_write;       // DCACHE DATA WRITE
    uint32_t m_cpt_dcache_dir_read;         // DCACHE DIR READ
    uint32_t m_cpt_dcache_dir_write;        // DCACHE DIR WRITE

    uint32_t m_cpt_icache_data_read;        // ICACHE DATA READ
    uint32_t m_cpt_icache_data_write;       // ICACHE DATA WRITE
    uint32_t m_cpt_icache_dir_read;         // ICACHE DIR READ
    uint32_t m_cpt_icache_dir_write;        // ICACHE DIR WRITE

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
    
    uint32_t m_cpt_itlbmiss_transaction;
    uint32_t m_cpt_dtlbmiss_transaction;


protected:
    SC_HAS_PROCESS(VciVCacheWrapper);

public:
    VciVCacheWrapper(
        sc_module_name insname,
        int proc_id,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &initiator_index,
        size_t itlb_m_ways,
        size_t itlb_m_sets,
        size_t itlb_k_ways,
        size_t itlb_k_sets,
        size_t dtlb_m_ways,
        size_t dtlb_m_sets,
        size_t dtlb_k_ways,
        size_t dtlb_k_sets,
        size_t icache_ways,
        size_t icache_sets,
        size_t icache_words,
        size_t dcache_ways,
        size_t dcache_sets,
        size_t dcache_words );

    ~VciVCacheWrapper();

    void print_cpi();
    void print_stats();

private:
    static inline bool is_write(data_op_t cmd);
    static inline data_t be_to_mask( typename iss_t::be_t );

    void transition();
    void genMoore();

};

}}

#endif /* SOCLIB_CABA_VCI_VCACHE_WRAPPER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

