//////////////////////////////////////////////////////////////////////////
// File     : vci_xcache.h
// Date     : 17/07/2007
// Copyright: UPMC/LIP6
/////////////////////////////////////////////////////////////////////////
 
#ifndef SOCLIB_CABA_VCI_XCACHE_H
#define SOCLIB_CABA_VCI_XCACHE_H

#include <inttypes.h>
#include <systemc>
#include "common/static_assert.h"
#include "common/static_log2.h"
#include "caba/util/base_module.h"
#include "caba/util/generic_fifo.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/xcache_cache_ports.h"
#include "common/mapping_table.h"

namespace soclib {
namespace caba {

    using namespace sc_core;

template<typename    vci_param>
class VciXCache
    : public soclib::caba::BaseModule
{
public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
    soclib::caba::ICacheCachePort           p_icache;
    soclib::caba::DCacheCachePort           p_dcache;
    soclib::caba::VciInitiator<vci_param>   p_vci;

private:

    // STRUCTURAL PARAMETERS
    soclib::common::AddressDecodingTable<uint32_t, bool> m_cacheability_table;
    int                                                  m_ident;   

    const int       s_dcache_lines;
    const int       s_dcache_words;
    const int       s_icache_lines;
    const int       s_icache_words;

    const int       s_icache_xshift;
    const int       s_icache_yshift;
    const int       s_icache_zshift;
    const int       s_icache_xmask;
    const int       s_icache_ymask;
    const int       s_icache_zmask;

    const int       s_dcache_xshift;
    const int       s_dcache_yshift;
    const int       s_dcache_zshift;
    const int       s_dcache_xmask;
    const int       s_dcache_ymask;
    const int       s_dcache_zmask;

    // REGISTERS
    sc_signal<int>      r_dcache_fsm;
    sc_signal<int>      **r_dcache_data;
    sc_signal<int>      *r_dcache_tag;
    sc_signal<int>      r_dcache_save_addr;
    sc_signal<int>      r_dcache_save_data;
    sc_signal<int>      r_dcache_save_type;
    sc_signal<int>      r_dcache_save_prev;

    soclib::caba::GenericFifo<sc_dt::sc_uint<32>,8>  m_data_fifo;
    soclib::caba::GenericFifo<sc_dt::sc_uint<32>,8>  m_addr_fifo;
    soclib::caba::GenericFifo<sc_dt::sc_uint<4>,8>   m_type_fifo;

    sc_signal<int>      r_icache_fsm;
    sc_signal<int>      **r_icache_data;
    sc_signal<int>      *r_icache_tag;
    sc_signal<int>      r_icache_miss_addr;
    sc_signal<bool>     r_icache_req;

    sc_signal<int>      r_vci_cmd_fsm;
    sc_signal<int>      r_dcache_cmd_addr;
    sc_signal<int>      r_dcache_cmd_data;
    sc_signal<int>      r_dcache_cmd_type;
    sc_signal<int>      r_dcache_miss_addr;
    sc_signal<int>      r_cmd_cpt;       
      
    sc_signal<int>      r_vci_rsp_fsm;
    sc_signal<int>      *r_icache_miss_buf;    
    sc_signal<int>      *r_dcache_miss_buf;    
    sc_signal<bool>     r_dcache_unc_valid;    
    sc_signal<int>      r_rsp_cpt;  

    sc_signal<int>      r_dcache_cpt_init;   
    sc_signal<int>      r_icache_cpt_init;  

    // Activity counters
    uint32_t m_cpt_dcache_data_read;  // for DCACHE DATA READ
    uint32_t m_cpt_dcache_data_write; // for DCACHE DATA WRITE    
    uint32_t m_cpt_dcache_dir_read;   // for DCACHE DIR READ
    uint32_t m_cpt_dcache_dir_write;  // for DCACHE DIR WRITE
    uint32_t m_cpt_icache_data_read;  // for ICACHE DATA READ
    uint32_t m_cpt_icache_data_write; // for ICACHE DATA WRITE
    uint32_t m_cpt_icache_dir_read;   // for ICACHE DIR READ
    uint32_t m_cpt_icache_dir_write;  // for ICACHE DIR WRITE
    uint32_t m_cpt_fifo_read;         // for FIFO READ
    uint32_t m_cpt_fifo_write;        // for FIFO WRITE

protected:
    SC_HAS_PROCESS(VciXCache);

public:
    VciXCache(
        sc_module_name insname,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &index,
        int icache_lines,
        int icache_words,
        int dcache_lines,
        int dcache_words );

private:
    void transition();
    void genMoore();
    void genMealy();
};

}}

#endif /* SOCLIB_CABA_VCI_XCACHE_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

