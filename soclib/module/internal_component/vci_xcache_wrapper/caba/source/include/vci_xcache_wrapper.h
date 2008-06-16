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
 * Maintainers: alain
 */
 
#ifndef SOCLIB_CABA_VCI_XCACHE_WRAPPER_H
#define SOCLIB_CABA_VCI_XCACHE_WRAPPER_H

#include <inttypes.h>
#include <systemc>
#include "caba_base_module.h"
#include "generic_fifo.h"
#include "vci_initiator.h"
#include "soclib_endian.h"
#include "mapping_table.h"

namespace soclib {
namespace caba {

    using namespace sc_core;

#define ICACHE_DATA(i,j,k) r_icache_data[(i*m_icache_sets*m_icache_words)+(j*m_icache_words)+k]
#define ICACHE_TAG(i,j)    r_icache_tag[(i*m_icache_sets)+j]

#define DCACHE_DATA(i,j,k) r_dcache_data[(i*m_dcache_sets*m_dcache_words)+(j*m_dcache_words)+k]
#define DCACHE_TAG(i,j)    r_dcache_tag[(i*m_dcache_sets)+j]

template<typename   vci_param, typename   iss_t>
class VciXcacheWrapper
    : public soclib::caba::BaseModule
{
    typedef uint32_t addr_t;
    typedef uint32_t data_t;
    typedef uint32_t tag_t;
    typedef typename iss_t::DataAccessType type_t;


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

public:

    // PORTS
    sc_in<bool>                             p_clk;
    sc_in<bool>                             p_resetn;
    sc_in<bool>                             p_irq[iss_t::n_irq];
    soclib::caba::VciInitiator<vci_param>   p_vci;

private:

    // STRUCTURAL PARAMETERS
    soclib::common::AddressDecodingTable<uint32_t, bool> m_cacheability_table;
    int             m_srcid;   
    iss_t           m_iss;

    const size_t    m_dcache_sets;
    const size_t    m_dcache_words;
    const size_t    m_dcache_ways;
    const size_t    m_icache_sets;
    const size_t    m_icache_words;
    const size_t    m_icache_ways;

    const soclib::common::AddressMaskingTable<addr_t> m_i_x;
    const soclib::common::AddressMaskingTable<addr_t> m_i_y;
    const soclib::common::AddressMaskingTable<addr_t> m_i_z;
    const size_t  m_icache_yzmask;

    const soclib::common::AddressMaskingTable<addr_t> m_d_x;
    const soclib::common::AddressMaskingTable<addr_t> m_d_y;
    const soclib::common::AddressMaskingTable<addr_t> m_d_z;
    const size_t  m_dcache_yzmask;

    // CACHES ARRAYS
    data_t                  *r_dcache_data;
    tag_t                   *r_dcache_tag;
    data_t                  *r_icache_data;
    tag_t                   *r_icache_tag;

    // REGISTERS
    sc_signal<int>          r_dcache_fsm;
    sc_signal<addr_t>       r_dcache_addr_save;
    sc_signal<data_t>       r_dcache_data_save;
    sc_signal<data_t>       r_dcache_prev_save;
    sc_signal<int>          r_dcache_type_save;
    sc_signal<size_t>       r_dcache_way_save;
    sc_signal<bool>         r_dcache_cached_save;

    GenericFifo<addr_t>     m_dreq_addr_fifo;
    GenericFifo<data_t>     m_dreq_data_fifo;
    GenericFifo<int>        m_dreq_type_fifo;
    GenericFifo<bool>       m_dreq_cached_fifo;

    sc_signal<int>          r_icache_fsm;
    sc_signal<addr_t>       r_icache_miss_addr;
    sc_signal<bool>         r_icache_req;

    sc_signal<int>          r_vci_cmd_fsm;
    sc_signal<addr_t>       r_dcache_addr_cmd;
    sc_signal<data_t>       r_dcache_data_cmd;
    sc_signal<data_t>       r_dcache_prev_cmd;
    sc_signal<int>          r_dcache_type_cmd;
    sc_signal<bool>         r_dcache_cached_cmd;
    sc_signal<addr_t>       r_dcache_miss_addr;
    sc_signal<size_t>       r_cmd_cpt;       
      
    sc_signal<int>          r_vci_rsp_fsm;
    sc_signal<data_t>       *r_icache_miss_buf;    
    sc_signal<data_t>       *r_dcache_miss_buf;    
    sc_signal<bool>         r_dcache_unc_valid;    
    sc_signal<size_t>       r_rsp_cpt;  

    sc_signal<size_t>       r_dcache_cpt_init;   
    sc_signal<size_t>       r_icache_cpt_init;  

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
    SC_HAS_PROCESS(VciXcacheWrapper);

public:
    VciXcacheWrapper(
        sc_module_name insname,
        int proc_id,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &index,
        size_t icache_sets,
        size_t icache_words,
        size_t icache_ways,
        size_t dcache_sets,
        size_t dcache_words,
        size_t dcache_ways );

private:
    void transition();
    void genMoore();
    static inline bool can_burst( type_t old_type, addr_t old_addr,
                                  type_t new_type, addr_t new_addr );
    static inline bool is_write(type_t cmd);

    inline data_t &icache_data( size_t way, size_t set, size_t word );
    inline tag_t &icache_tag( size_t way, size_t set );

    inline data_t &dcache_data( size_t way, size_t set, size_t word );
    inline tag_t &dcache_tag( size_t way, size_t set );

    std::string dump_dcache_line(size_t way, size_t set);
};

}}

#endif /* SOCLIB_CABA_VCI_XCACHE_WRAPPER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

