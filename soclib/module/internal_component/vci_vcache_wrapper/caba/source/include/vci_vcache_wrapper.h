/* -*- c++ -*-
 * File : vci_vcache_wrapper.h
 * Copyright (c) UPMC, Lip6, SoC
 * Authors : Alain GREINER, Yang GAO
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

    enum icache_fsm_state_e {  
        ICACHE_IDLE,                // 00
        ICACHE_BIS,                 // 01
        ICACHE_TLB1_READ,           // 02
        ICACHE_TLB1_WRITE,          // 03
        ICACHE_TLB1_UPDT,           // 04
        ICACHE_TLB2_READ,           // 05
        ICACHE_TLB2_WRITE,          // 06
        ICACHE_TLB2_UPDT,           // 07
        ICACHE_TLB_FLUSH,           // 08
        ICACHE_CACHE_FLUSH,         // 09
        ICACHE_TLB_INVAL,           // 0a
        ICACHE_TLB_INVAL_DONE,      // 0b
        ICACHE_CACHE_INVAL,         // 0c
        ICACHE_CACHE_INVAL_DONE,    // 0d
        ICACHE_MISS_WAIT,           // 0e
        ICACHE_UNC_WAIT,            // 0f
        ICACHE_MISS_UPDT,           // 10
        ICACHE_ERROR,               // 11
    };

    enum dcache_fsm_state_e {  
        DCACHE_IDLE,                // 00
        DCACHE_BIS,                 // 01
        DCACHE_TLB1_READ,           // 02
        DCACHE_TLB1_WRITE,          // 03
        DCACHE_TLB1_UPDT,           // 04
        DCACHE_TLB2_READ,           // 05
        DCACHE_TLB2_WRITE,          // 06
        DCACHE_TLB2_UPDT,           // 07
        DCACHE_CTXT_SWITCH,         // 08
        DCACHE_ICACHE_FLUSH,        // 09
        DCACHE_DCACHE_FLUSH,        // 0a
        DCACHE_ITLB_INVAL,          // 0b
        DCACHE_DTLB_INVAL,          // 0c
        DCACHE_DTLB_INVAL_DONE,     // 0d
        DCACHE_ICACHE_INVAL,        // 0e
        DCACHE_DCACHE_INVAL,        // 0f
        DCACHE_DCACHE_INVAL_DONE,   // 10
        DCACHE_WRITE_UPDT,          // 11
        DCACHE_WRITE_DIRTY,         // 12
        DCACHE_WRITE_REQ,           // 13
        DCACHE_MISS_WAIT,           // 14
        DCACHE_MISS_UPDT,           // 15
        DCACHE_UNC_WAIT,            // 16
        DCACHE_ERROR,               // 17
    };

    enum cmd_fsm_state_e {      
        CMD_IDLE,           // 00
        CMD_ITLB_READ,      // 01
        CMD_ITLB_WRITE,     // 02
        CMD_INS_MISS,       // 03
        CMD_INS_UNC,        // 04
        CMD_DTLB_READ,      // 05
        CMD_DTLB_WRITE,     // 06
        CMD_DTLB_DIRTY,     // 07
        CMD_DATA_UNC,       // 08
        CMD_DATA_MISS,      // 09
        CMD_DATA_WRITE,     // 0a
    };

    enum rsp_fsm_state_e {       
        RSP_IDLE,           // 00
        RSP_ITLB_READ,      // 01
        RSP_ITLB_WRITE,     // 02
        RSP_INS_MISS,       // 03
        RSP_INS_UNC,        // 04
        RSP_DTLB_READ,      // 05
        RSP_DTLB_WRITE,     // 06
        RSP_DTLB_DIRTY,     // 07
        RSP_DATA_MISS,      // 08
        RSP_DATA_UNC,       // 09
        RSP_DATA_WRITE,     // 0a
    };

    // TLB Mode
    enum {
        TLBS_DEACTIVE,  // instruction TLB deactive and data TLB deactive
        ITLB_A_DTLB_D,  // instruction TLB   active and data TLB deactive
        ITLB_D_DTLB_A,  // instruction TLB deactive and data TLB   active
        TLBS_ACTIVE,    // instruction TLB   active and data TLB   active
    };

    // Error Type
    enum mmu_error_type_e {
        MMU_NONE                 = 0x0,   // None
        MMU_PT1_UNMAPPED         = 0x1,   // Page fault on Page Table 1           (non fatal)
        MMU_PT2_UNMAPPED         = 0x2,   // Page fault on Page Table 2           (non fatal)
        MMU_PRIVILEGE_VIOLATION  = 0x4,   // Protected access in user mode        (user error)
        MMU_WRITE_VIOLATION      = 0x8,   // write access to a non writable page  (user error)
        MMU_EXEC_VIOLATION       = 0x10,  // exec access to a non exec page       (user error)
        MMU_UNDEFINED_XTN        = 0x20,  // undefined external access address    (user error)
        MMU_PT1_ILLEGAL_ACCESS   = 0x40,  // Bus Error accessing Table 1          (kernel error)
        MMU_PT2_ILLEGAL_ACCESS   = 0x80,  // Bus Error accessing Table 2          (kernel error)
        MMU_CACHE_ILLEGAL_ACCESS = 0x100, // Bus Error in cache access            (kernel error)
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
    soclib::caba::GenericTlb<addr36_t>    icache_m_tlb;
    soclib::caba::GenericTlb<addr36_t>    icache_k_tlb;
    soclib::caba::GenericTlb<addr36_t>    dcache_m_tlb;
    soclib::caba::GenericTlb<addr36_t>    dcache_k_tlb;

    sc_signal<addr_t>       r_mmu_ptpr;             // page table pointer register
    sc_signal<int>          r_mmu_mode;             // tlb mode register

    // DCACHE FSM REGISTERS
    sc_signal<int>          r_dcache_fsm;           // state register
    sc_signal<addr36_t>     r_dcache_paddr_save;    // physical address
    sc_signal<data_t>       r_dcache_wdata_save;    // write data
    sc_signal<data_t>       r_dcache_rdata_save;    // read data
    sc_signal<type_t>       r_dcache_type_save;     // access type
    sc_signal<be_t>         r_dcache_be_save;       // byte enable
    sc_signal<bool>         r_dcache_cached_save;   // used by the write buffer
    sc_signal<addr36_t>     r_dcache_tlb_paddr;     // physical address of tlb miss
    sc_signal<bool>         r_dcache_dirty_save;    // used for TLB dirty bit update
    sc_signal<size_t>       r_dcache_tlb_set_save;  // used for TLB dirty bit update
    sc_signal<size_t>       r_dcache_tlb_way_save;  // used for TLB dirty bit update
    sc_signal<addr_t>       r_dcache_id1_save;      // used by the PT1 bypass
    sc_signal<addr36_t>     r_dcache_ptba_save;     // used by the PT1 bypass
    sc_signal<bool>         r_dcache_ptba_ok;       // used by the PT1 bypass
    sc_signal<data_t>       r_dcache_pte_update;    // used for page table update
    sc_signal<addr_t>       r_dcache_ppn_save;      // used for speculative cache access
    sc_signal<bool>         r_dcache_page_k_save;   // used in the BIS state
    sc_signal<bool>         r_dcache_buf_unc_valid; // used for uncached read

    sc_signal<data_t>       r_dcache_error_type;    // software visible register
    sc_signal<addr_t>       r_dcache_bad_vaddr;     // software visible register 

    sc_signal<bool>         r_dcache_miss_req;
    sc_signal<bool>         r_dcache_unc_req;
    sc_signal<bool>         r_dcache_write_req;
    sc_signal<bool>         r_dcache_tlb_read_req;
    sc_signal<bool>         r_dcache_tlb_write_req;
    sc_signal<bool>         r_dcache_tlb_dirty_req;

    sc_signal<bool>         r_dcache_xtn_req; 

    // ICACHE FSM REGISTERS
    sc_signal<int>          r_icache_fsm;           // state register
    sc_signal<addr36_t>     r_icache_paddr_save;    // physical address
    sc_signal<data_t>       r_icache_miss_data;
    sc_signal<addr_t>       r_icache_id1_save;      // used by the PT1 bypass
    sc_signal<addr36_t>     r_icache_ptba_save;     // used by the PT1 bypass
    sc_signal<bool>         r_icache_ptba_ok;       // used by the PT1 bypass
    sc_signal<data_t>       r_icache_pte_update;    // used for page table update
    sc_signal<addr_t>       r_icache_ppn_save;      // used for speculative cache access
    sc_signal<bool>         r_icache_page_k_save;
    sc_signal<bool>         r_icache_buf_unc_valid;

    sc_signal<data_t>       r_icache_error_type;    // software visible registers
    sc_signal<addr_t>       r_icache_bad_vaddr;     // software visible registers

    sc_signal<bool>         r_icache_miss_req;
    sc_signal<bool>         r_icache_unc_req;
    sc_signal<bool>         r_icache_tlb_read_req;
    sc_signal<bool>         r_icache_tlb_write_req;

    // VCI_CMD FSM REGISTERS
    sc_signal<int>          r_vci_cmd_fsm;
    sc_signal<size_t>       r_vci_cmd_min;       
    sc_signal<size_t>       r_vci_cmd_max;       
    sc_signal<size_t>       r_vci_cmd_cpt;     

    // VCI_RSP FSM REGISTERS
    sc_signal<int>          r_vci_rsp_fsm;
    sc_signal<size_t>       r_vci_rsp_cpt;
    sc_signal<data_t>       r_vci_rsp_itlb_miss;
    sc_signal<data_t>       r_vci_rsp_dtlb_miss;
    sc_signal<bool>         r_vci_rsp_ins_error;
    sc_signal<bool>         r_vci_rsp_data_error;


    data_t                  *r_icache_miss_buf;    
    data_t                  *r_dcache_miss_buf;  


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

