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
 *         Alain Greiner <alain.greiner@lip6.fr>, 2006
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */
 
#ifndef SOCLIB_CABA_VCI_XCACHE_H
#define SOCLIB_CABA_VCI_XCACHE_H

#include <inttypes.h>
#include <systemc>
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
    typedef uint32_t addr_t;
    typedef uint32_t data_t;
    typedef uint32_t tag_t;

    enum dcache_fsm_state_e {
        DCACHE_INIT,
        DCACHE_IDLE,
        DCACHE_WRITE_UPDT,
        DCACHE_WRITE_REQ,
        DCACHE_MISS_REQ,
        DCACHE_MISS_WAIT,
        DCACHE_MISS_UPDT,
        DCACHE_UNC_REQ,
        DCACHE_UNC_WAIT,
        DCACHE_INVAL,
        DCACHE_ERROR,
    };

    enum icache_fsm_state_e {
        ICACHE_INIT,
        ICACHE_IDLE,
        ICACHE_WAIT,
        ICACHE_UPDT,
        ICACHE_ERROR,
    };

    enum cmd_fsm_state_e {
        CMD_IDLE,
        CMD_DATA_MISS,
        CMD_DATA_UNC,
        CMD_DATA_WRITE,
        CMD_INS_MISS,
    };

    enum rsp_fsm_state_e {
        RSP_IDLE,
        RSP_INS_MISS,
        RSP_INS_ERROR_WAIT,
        RSP_INS_ERROR,
        RSP_INS_OK,
        RSP_DATA_MISS,
        RSP_DATA_UNC,
        RSP_DATA_WRITE,
        RSP_DATA_READ_ERROR_WAIT,
        RSP_DATA_READ_ERROR,
        RSP_DATA_MISS_OK,
        RSP_DATA_UNC_OK,
        RSP_DATA_WRITE_ERROR,
        RSP_DATA_WRITE_ERROR_WAIT,
    };

    enum {
        READ_PKTID,
        WRITE_PKTID,
    };

    typedef struct d_req_s {
        addr_t addr;
        data_t data;
        data_t prev;
        DCacheSignals::req_type_e type;
        bool cached;

        friend std::ostream &operator <<(std::ostream &o, d_req_s r)
        {
            o << "<dreq: "
              << std::dec
              << "(" << r.type << ")"
              << std::hex
              <<"@" << r.addr
              << ": " << r.data
              << ">" << std::endl;
            return o;
        }

        bool operator ==( const d_req_s &other ) const
        {
            return other.data == data &&
                other.addr == addr &&
                other.prev == prev &&
                other.type == type;
        }
    } d_req_t;

public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
    soclib::caba::ICacheCachePort           p_icache;
    soclib::caba::DCacheCachePort           p_dcache;
    soclib::caba::VciInitiator<vci_param>   p_vci;

private:

    // STRUCTURAL PARAMETERS
    soclib::common::AddressDecodingTable<uint32_t, bool> m_cacheability_table;
    int m_ident;   

    const size_t  s_dcache_lines;
    const size_t  s_dcache_words;
    const size_t  s_icache_lines;
    const size_t  s_icache_words;

    const soclib::common::AddressMaskingTable<addr_t> m_i_x;
    const soclib::common::AddressMaskingTable<addr_t> m_i_y;
    const soclib::common::AddressMaskingTable<addr_t> m_i_z;
    const size_t  m_icache_yzmask;

    const soclib::common::AddressMaskingTable<addr_t> m_d_x;
    const soclib::common::AddressMaskingTable<addr_t> m_d_y;
    const soclib::common::AddressMaskingTable<addr_t> m_d_z;
    const size_t  m_dcache_yzmask;

    // REGISTERS
    sc_signal<dcache_fsm_state_e>      r_dcache_fsm;
    sc_signal<data_t>      **r_dcache_data;
    sc_signal<tag_t>      *r_dcache_tag;
    sc_signal<d_req_t>      r_dcache_save;

    soclib::caba::GenericFifo<d_req_t,8>  m_dreq_fifo;

    sc_signal<icache_fsm_state_e>      r_icache_fsm;
    sc_signal<data_t>      **r_icache_data;
    sc_signal<tag_t>      *r_icache_tag;
    sc_signal<addr_t>      r_icache_miss_addr;
    sc_signal<bool>     r_icache_req;

    sc_signal<cmd_fsm_state_e>      r_vci_cmd_fsm;
    sc_signal<d_req_t>      r_dcache_cmd;
    sc_signal<addr_t>      r_dcache_miss_addr;
    sc_signal<size_t>      r_cmd_cpt;       
      
    sc_signal<rsp_fsm_state_e>      r_vci_rsp_fsm;
    sc_signal<data_t>      *r_icache_miss_buf;    
    sc_signal<data_t>      *r_dcache_miss_buf;    
    sc_signal<bool>     r_dcache_unc_valid;    
    sc_signal<size_t>      r_rsp_cpt;  

    sc_signal<size_t>      r_dcache_cpt_init;   
    sc_signal<size_t>      r_icache_cpt_init;  

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
        size_t icache_lines,
        size_t icache_words,
        size_t dcache_lines,
        size_t dcache_words );

private:
    void transition();
    void genMoore();
    void genMealy();

    static inline bool can_burst(const d_req_t &old, const d_req_t &next);
    static inline bool is_write(soclib::caba::DCacheSignals::req_type_e cmd);
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

