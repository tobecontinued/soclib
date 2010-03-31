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
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 * Alexandre Becoulet <alexandre.becoulet@free.fr>, 2010
 *
 * Maintainers: nipo becoulet
 *
 * $Id$
 *
 */

#ifndef _SOCLIB_ISS_MEMCHECKER_H_
#define _SOCLIB_ISS_MEMCHECKER_H_

#include <stdint.h>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <systemc.h>
#include "iss2.h"
#include "exception.h"
#include "soclib_endian.h"
#include "mapping_table.h"
#include "loader.h"

namespace soclib { namespace common {

namespace __iss_memchecker {
class ContextState;
class RegionInfo;
class MemoryState;
}

template<typename iss_t>
class IssMemchecker
    : public iss_t
{
    uint32_t m_comm_address;
    __iss_memchecker::ContextState *m_current_context;
    __iss_memchecker::ContextState *m_last_context;
    __iss_memchecker::RegionInfo *m_last_region_touched;
    bool m_has_data_answer;
    const uint32_t m_cpuid;
    uint32_t m_data_answer_value;
    struct iss_t::DataRequest m_last_data_access;
    struct iss_t::DataRequest m_blast_data_access;
    uint32_t m_enabled_checks;
    uint32_t m_r1;
    uint32_t m_r2;
    uint32_t m_last_sp;

    // processor spinlocks
    typedef std::map<uint32_t, bool /* cycle */> held_locks_map_t;
    held_locks_map_t m_held_locks;

    bool m_opt_dump_iss;
    bool m_opt_dump_access;
    bool m_opt_show_ctx;
    bool m_opt_show_ctxsw;
    bool m_opt_show_region;
    bool m_opt_show_lockops;

    uint32_t m_trap_mask;
    uint32_t m_report_mask;

    uint32_t m_no_repeat_mask;

    enum magic_state_e {
        MAGIC_NONE,
        MAGIC_BE,
        MAGIC_LE,
    };
    enum magic_state_e m_magic_state;

public:

    IssMemchecker(const std::string &name, uint32_t ident);

    uint32_t executeNCycles( uint32_t ncycle, struct iss_t::InstructionResponse irsp,
                             struct iss_t::DataResponse drsp, uint32_t irq_bit_field );

    inline void getRequests(
        struct iss_t::InstructionRequest &ireq,
        struct iss_t::DataRequest &dreq) const
    {
        iss_t::getRequests(ireq, dreq);
        if ( dreq.valid ) {
            if ( (dreq.addr & ~(uint32_t)0xff) == m_comm_address ) {
                dreq.valid = false;
            }
        }
    }

    static void init( const soclib::common::MappingTable &mt,
                      const soclib::common::Loader &loader,
                      const std::string &exclusions = "" );

private:

    void update_context(__iss_memchecker::ContextState *nc);

    bool register_set( uint32_t reg_no, uint32_t value );
    uint32_t register_get( uint32_t reg_no ) const;
    bool handle_comm( const struct iss_t::DataRequest &dreq );
    bool check_data_access( const struct iss_t::DataRequest &dreq,
                            const struct iss_t::DataResponse &drsp );

    bool report_error( uint32_t errors, uint32_t extra = 0 );
    void report_current_ctx();

    uint32_t get_cpu_sp() const;
    uint32_t get_cpu_fp() const;
    uint32_t get_cpu_pc() const;
};

}}

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
