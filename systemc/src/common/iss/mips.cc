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
 * Copyright (c) UPMC, Lip6
 *    Nicolas Pouillon <nipo@ssji.net>, 2007
 *    Alain Greiner <alain.greiner@lip6.fr>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 *
 * History:
 * - 2007-06-15
 *     Nicolas Pouillon, Alain Greiner: Model created
 */

#include "common/iss/mips.h"
#include "common/arithmetics.h"

namespace soclib { namespace common {

static const uint32_t NO_EXCEPTION = (uint32_t)-1;

namespace {

static inline uint32_t align( uint32_t data, int shift, int width )
{
    uint32_t mask = (1<<width)-1;
    uint32_t ret = data >>= shift*width;
    return ret & mask;
}

static inline std::string mkname(uint32_t no)
{
    char tmp[32];
    snprintf(tmp, 32, "mips_iss%d", (int)no);
    return std::string(tmp);
}

}

MipsIss::MipsIss(uint32_t ident)
    : Iss(mkname(ident), ident)
{
}

void MipsIss::reset()
{
    Iss::reset(RESET_ADDRESS);
    r_npc = RESET_ADDRESS + 4;
    r_status.whole = 0;
}

void MipsIss::dump() const
{
    std::cout
        << std::hex << std::showbase
        << m_name
        << " PC: " << r_pc
        << " Ins: " << m_ins.ins << std::endl
        << std::dec
        << " Status.ie " << r_status.iec
        << " .im " << r_status.im
        << " .um " << r_status.kuc << std::endl
        << " op:  " << m_ins.i.op << " (" << name_table[m_ins.i.op] << ")" << std::endl
        << " i rs: " << m_ins.i.rs
        << " rt: "<<m_ins.i.rt
        << " i: "<<std::hex << m_ins.i.imd
        << std::endl << std::dec
        << " r rs: " << m_ins.r.rs
        << " rt: "<<m_ins.r.rt
        << " rd: "<<m_ins.r.rd
        << " sh: "<<m_ins.r.sh << std::hex
        << " func: "<<m_ins.r.func
        << std::endl
        << " V rs: " << m_rs
        << " rt: "<<m_rt
        << std::endl;
    for ( size_t i=0; i<32; ++i ) {
        std::cout << " " << std::dec << i << ": " << std::hex << std::showbase << r_gp[i];
        if ( i%8 == 7 )
            std::cout << std::endl;
    }
}

void MipsIss::step()
{
    ++r_count;

    // The current instruction is executed in case of interrupt, but
    // the next instruction will be delayed.
    // The current instruction is not executed in case of exception,
    // and there is three types of bus error events,
    // 1 - instruction bus errors are synchronous events, signaled in
    // the m_ibe variable
    // 2 - read data bus errors are synchronous events, signaled in
    // the m_dbe variable
    // 3 - write data bus errors are asynchonous events signaled in
    // the r_dbe flip-flop
    // Instuction bus errors are related to the current instruction:
    // lowest priority.
    // Read Data bus errors are related to the previous instruction:
    // intermediatz priority
    // Write Data bus error are related to an older instruction:
    // highest priority

    m_next_pc = r_npc+4;
    m_exception = NO_EXCEPTION;
    bool hazard = false;

    if (m_ibe) {
        m_exception = X_IBE;
        goto handle_except;
    }

    if ( m_dbe ) {
        m_exception = X_DBE;
        r_bar = r_mem_addr;
        goto handle_except;
    }

    // We write the  r_gp[i], and we detect a possible data dependency,
    // in order to implement the delayed load behaviour.
    if ( r_mem_type != MEM_NONE ) {
        uint32_t reg_use = curInstructionUsesRegs();
        if ( (reg_use & USE_S && r_mem_dest == m_ins.r.rs) ||
             (reg_use & USE_T && r_mem_dest == m_ins.r.rt) )
            hazard = true;
        
        switch (r_mem_type) {
        default:
            break;
        case MEM_LW:
            r_gp[r_mem_dest] = m_rdata;
            break;
        case MEM_LB:
            r_gp[r_mem_dest] = sign_ext8(align(m_rdata, r_mem_addr&0x3, 8));
            break;
        case MEM_LBU:
            r_gp[r_mem_dest] = align(m_rdata, r_mem_addr&0x3, 8);
            break;
        case MEM_LH:
            r_gp[r_mem_dest] = sign_ext16(align(m_rdata, (r_mem_addr&0x2)/2, 16));
            break;
        case MEM_LHU:
            r_gp[r_mem_dest] = align(m_rdata, (r_mem_addr&0x2)/2, 16);
            break;
        }
        r_mem_dest = 0;
        r_mem_type = MEM_NONE;
    }

    if ( r_dbe ) {
        m_exception = X_DBE;
        r_dbe = false;
        r_bar = r_mem_addr;
    }

#if MIPS_DEBUG
    dump();
#endif
    // run() can modify the following registers: r_gp[i], r_mem_type,
    // r_mem_addr; r_mem_wdata, r_mem_dest, r_hi, r_lo, m_exception,
    // m_next_pc
    if ( ! hazard )
        run();
    else
        goto house_keeping;

    if ( m_exception != NO_EXCEPTION )
        goto handle_except;

    if ( m_irq
         && r_status.im & m_irq
         && r_status.iec ) {
        m_exception = X_INT;
        goto handle_except;
    }
    
    goto no_except;

 handle_except:
    {
        bool branch_taken = m_next_pc != r_npc+4;

        r_cause.xcode = m_exception;
        r_cause.irq  = m_irq;
        r_cause.bd = branch_taken;
        r_status.kuo = r_status.kup;
        r_status.ieo = r_status.iep;
        r_status.kup = r_status.kuc;
        r_status.iep = r_status.iec;
        r_status.kuc = 0;
        r_status.iec = 0;
        r_epc = branch_taken ? r_pc : r_npc;
#if MIPS_DEBUG
        std::cout
            << m_name <<" exception: "<<m_exception<<std::endl
            << " epc: " << r_epc
            << " bar: " << r_mem_addr
            << " cause.xcode: " << r_cause.xcode
            << " .bd: " << r_cause.bd
            << " .irq: " << r_cause.irq
            << std::endl;
#endif
    }
    r_pc = EXCEPT_ADDRESS;
    r_npc = EXCEPT_ADDRESS + 4;
    goto house_keeping;
 no_except:
    r_pc = r_npc;
    r_npc = m_next_pc;
 house_keeping:
    r_gp[0] = 0;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
