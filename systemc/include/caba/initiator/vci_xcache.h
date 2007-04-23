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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_VCI_XCACHE_H
#define SOCLIB_CABA_VCI_XCACHE_H

#include <systemc.h>
#include "common/static_assert.h"
#include "common/static_log2.h"
#include "caba/util/base_module.h"
#include "caba/util/generic_fifo.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/xcache_cache_ports.h"
#include "common/mapping_table.h"

namespace soclib {
namespace caba {

template<
    size_t WRITE_BUFFER_DEPTH,
    size_t ICACHE_LINES,
    size_t ICACHE_WORDS,
    size_t DCACHE_LINES,
    size_t DCACHE_WORDS,
    typename vci_param>
class VciXCache
    : public soclib::caba::BaseModule
{
public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
    soclib::caba::ICacheCachePort p_icache;
    soclib::caba::DCacheCachePort p_dcache;

    soclib::caba::VciInitiator<vci_param> p_vci;

private:
    soclib::common::AddressDecodingTable<uint32_t, bool> m_cacheability_table;

    // STRUCTURAL PARAMETERS
    int          m_ident;            //  VCI SRCID value

    static_assert(IS_POW_OF_2(ICACHE_LINES));
    static_assert(IS_POW_OF_2(DCACHE_LINES));
    static_assert(IS_POW_OF_2(ICACHE_WORDS));
    static_assert(IS_POW_OF_2(DCACHE_WORDS));
    static_assert(ICACHE_WORDS);
    static_assert(DCACHE_WORDS);
    static_assert(ICACHE_LINES);
    static_assert(DCACHE_LINES);
    static_assert(ICACHE_WORDS <= 16);
    static_assert(DCACHE_WORDS <= 16);
    static_assert(ICACHE_LINES <= 1024);
    static_assert(DCACHE_LINES <= 1024);

    static const int s_icache_xshift = 2;
    static const int s_icache_yshift = static_log2(ICACHE_WORDS)+s_icache_xshift;
    static const int s_icache_xmask  = ((1<<static_log2(ICACHE_WORDS))-1)<<s_icache_xshift;
    static const int s_icache_zshift = static_log2(ICACHE_LINES)+s_icache_yshift;
    static const int s_icache_ymask  = ((1<<static_log2(ICACHE_LINES))-1)<<s_icache_yshift;
    static const int s_icache_zmask  = ((~0x0) << s_icache_zshift);

    static const int s_dcache_xshift  = 2;
    static const int s_dcache_yshift  = static_log2(DCACHE_WORDS)+s_dcache_xshift;
    static const int s_dcache_xmask   = ((1<<static_log2(DCACHE_WORDS))-1)<<s_dcache_xshift;
    static const int s_dcache_zshift  = static_log2(DCACHE_LINES)+s_dcache_yshift;
    static const int s_dcache_ymask   = ((1<<static_log2(DCACHE_LINES))-1)<<s_dcache_yshift;
    static const int s_dcache_zmask   = ((~0x0) << s_dcache_zshift);

    // REGISTERS
    sc_signal<int> r_dcache_fsm;
    sc_signal<int>         DCACHE_DATA[DCACHE_LINES][DCACHE_WORDS];
    sc_signal<int>         DCACHE_TAG[DCACHE_LINES];
    sc_signal<int>         DCACHE_SAVE_ADDR;
    sc_signal<int>         DCACHE_SAVE_DATA;
    sc_signal<int>         DCACHE_SAVE_TYPE;
    sc_signal<int>         DCACHE_SAVE_PREV;

    soclib::caba::GenericFifo<sc_uint<32>,WRITE_BUFFER_DEPTH> m_data_fifo;
    soclib::caba::GenericFifo<sc_uint<32>,WRITE_BUFFER_DEPTH> m_addr_fifo;
    soclib::caba::GenericFifo<sc_uint<4>,WRITE_BUFFER_DEPTH>  m_type_fifo;

    sc_signal<int> r_icache_fsm;
    sc_signal<int>         ICACHE_DATA[ICACHE_LINES][ICACHE_WORDS];
    sc_signal<int>         ICACHE_TAG[ICACHE_LINES];
    sc_signal<int>         ICACHE_MISS_ADDR;
    sc_signal<bool>        ICACHE_REQ;

    sc_signal<int> r_vci_cmd_fsm;
    sc_signal<int>         DCACHE_CMD_ADDR;
    sc_signal<int>         DCACHE_CMD_DATA;
    sc_signal<int>         DCACHE_CMD_TYPE;
    sc_signal<int>         DCACHE_MISS_ADDR;
    sc_signal<int>         CMD_CPT;        // counter for VCI request packet
      
    sc_signal<int> r_vci_rsp_fsm;
    sc_signal<int>         ICACHE_MISS_BUF[ICACHE_WORDS];    
    sc_signal<bool>        ICACHE_VAL_BUF[ICACHE_WORDS];    
    sc_signal<int>         DCACHE_MISS_BUF[DCACHE_WORDS];    
    sc_signal<bool>        DCACHE_VAL_BUF[DCACHE_WORDS];    
    sc_signal<int>         RSP_CPT;        // counter for VCI response packet

    sc_signal<int>         DCACHE_CPT_INIT;    // Counter for DCACHE initialisation
    sc_signal<int>         ICACHE_CPT_INIT;    // Counter for ICACHE initialisation

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
        const soclib::common::IntTab &index);

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

