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
 *    Nicolas Pouillon <nipo@ssji.net>, 2008
 *
 * Maintainers: nipo
 *
 * $Id$
 */

#include "mips32.h"
#include "base_module.h"
#include "soclib_endian.h"
#include "arithmetics.h"

namespace soclib { namespace common {

Mips32Iss::Mips32Iss(const std::string &name, uint32_t ident, bool default_little_endian)
    : Iss2(name, ident),
      m_little_endian(default_little_endian)
{
    r_config.whole = 0;
    r_config.m = 1;
    r_config.be = m_little_endian ? 0 : 1;

    r_config1.whole = 0;
}

void Mips32Iss::reset()
{
    struct DataRequest null_dreq = ISS_DREQ_INITIALIZER;
    r_ebase = 0x80000000 | m_ident;
    r_pc = RESET_ADDRESS;
    r_npc = RESET_ADDRESS + 4;
    r_dbe = false;
    m_ibe = false;
    m_dbe = false;
    m_dreq = null_dreq;
    r_mem_dest = 0;
    m_skip_next_instruction = false;
    m_ins_delay = 0;
    r_status.whole = 0x400004;
    r_cause.whole = 0;
    m_exec_cycles = 0;
    r_gp[0] = 0;
    m_sleeping = false;
    r_count = 0;
    r_compare = 0;

    for(int i = 0; i<32; i++)
        r_gp[i] = 0;

    m_hazard=false;
    m_exception = NO_EXCEPTION;
}

void Mips32Iss::dump() const
{
    std::cout
        << std::hex << std::showbase
        << m_name
        << " PC: " << r_pc
        << " Ins: " << m_ins.ins << std::endl
        << std::dec
        << " Cause.xcode: " << r_cause.xcode << std::endl
        << " Status.ksu " << r_status.ksu << std::endl
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
        << " V rs: " << r_gp[m_ins.i.rs]
        << " rt: "<<r_gp[m_ins.i.rt]
        << std::endl;
    for ( size_t i=0; i<32; ++i ) {
        std::cout << " " << std::dec << i << ": " << std::hex << std::showbase << r_gp[i];
        if ( i%8 == 7 )
            std::cout << std::endl;
    }
}

uint32_t Mips32Iss::executeNCycles( uint32_t ncycle, uint32_t irq_bit_field )
{
    m_exception = NO_EXCEPTION;

    if ( m_sleeping ) {
        if ( ((r_status.im>>2) & irq_bit_field)
             && r_status.ie ) {
            m_exception = X_INT;
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << name() << " IRQ while sleeping" << std::endl;
#endif
        } else {
            r_count += ncycle;
            return ncycle;
        }
    }
    if ( ! m_ireq_ok || ! m_dreq_ok || m_ins_delay ) {
        uint32_t t = ncycle;
        if ( m_ins_delay ) {
            if ( m_ins_delay < ncycle )
                t = m_ins_delay;
            m_ins_delay -= t;
        }
        m_hazard = false;
        r_count += t;
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " Frozen " << m_ireq_ok << m_dreq_ok<< " " << m_ins_delay << std::endl;
#endif
        return t;
    }

    if ( m_hazard && ncycle > 1 ) {
        ncycle = 2;
        m_hazard = false;
    } else {
        ncycle = 1;
    }
    r_count += ncycle;

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

    if (m_ibe) {
        m_exception = X_IBE;
        goto handle_except;
    }

    if ( m_dbe ) {
        m_exception = X_DBE;
        r_bar = m_dreq.addr;
        goto handle_except;
    }

    if ( r_dbe ) {
        m_exception = X_DBE;
        r_dbe = false;
        r_bar = m_dreq.addr;
    }

#ifdef SOCLIB_MODULE_DEBUG
    dump();
#endif
    // run() can modify the following registers: r_gp[i], r_mem_req,
    // r_mem_type, m_dreq.addr; r_mem_wdata, r_mem_dest, r_hi, r_lo,
    // m_exception, m_next_pc
    if ( m_hazard ) {
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " hazard, seeing next cycle" << std::endl;
#endif
        m_hazard = false;
        goto house_keeping;
    } else {
        m_exec_cycles++;
        run();
    }

    if ( m_exception == NO_EXCEPTION
         && ((r_status.im>>2) & irq_bit_field)
         && r_status.ie ) {
        m_exception = X_INT;
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " Taking irqs " << irq_bit_field << std::endl;
    } else {
        if ( irq_bit_field )
            std::cout << name() << " Ignoring irqs " << irq_bit_field << std::endl;
#endif
    }

    if ( m_exception != NO_EXCEPTION && debugExceptionBypassed( m_exception ) )
        m_exception = NO_EXCEPTION;
    
    if (  m_exception == NO_EXCEPTION || r_status.exl || r_status.erl )
        goto no_except;

 handle_except:

    {
        addr_t except_address = exceptBaseAddr();
        bool branch_taken = m_next_pc != r_npc+4;

        if ( r_status.exl ) {
            except_address += 0x180;
        } else {
            if ( m_sleeping && m_exception == X_INT ) {
                r_cause.bd = 0;
                r_epc = r_pc;
                m_sleeping = false;
            } else {
                r_cause.bd = branch_taken;
                if ( branch_taken )
                    r_epc = r_pc;
                else
                    r_epc = r_npc;
            }
            except_address += exceptOffsetAddr(m_exception);
        }
        r_cause.ce = 0;
        r_cause.xcode = m_exception;
        r_cause.ip  = irq_bit_field<<2;
        r_status.exl = 1;

#ifdef SOCLIB_MODULE_DEBUG
        std::cout
            << m_name <<" exception: "<<m_exception<<std::endl
            << " epc: " << r_epc
            << " error_epc: " << r_error_epc
            << " bar: " << m_dreq.addr
            << " cause.xcode: " << r_cause.xcode
            << " .bd: " << r_cause.bd
            << " .ip: " << r_cause.ip
            << " status.exl: " << r_status.exl
            << " .erl: " << r_status.erl
            << " exception address: " << except_address
            << std::endl;
#endif

        m_next_pc = except_address;
        m_skip_next_instruction = true;
    }
 no_except:
    if (m_skip_next_instruction) {
        r_pc = m_next_pc;
        r_npc = m_next_pc+4;
        m_skip_next_instruction = false;
    } else {
        r_pc = r_npc;
        r_npc = m_next_pc;
    }
 house_keeping:
 stick:
    r_gp[0] = 0;
    return ncycle;
}

int Mips32Iss::debugCpuCauseToSignal( uint32_t cause ) const
{
    switch (cause) {
    case X_INT:
        return 2; // Interrupt
    case X_MOD:
    case X_TLBL:
    case X_TLBS:
        return 5; // Trap (nothing better)
    case X_ADEL:
    case X_ADES:
    case X_IBE:
    case X_DBE:
        return 11; // SEGV
    case X_SYS:
    case X_BP:
    case X_TR:
    case X_reserved:
        return 5; // Trap/breakpoint
    case X_RI:
    case X_CPU:
        return 4; // Illegal instruction
    case X_OV:
    case X_FPE:
        return 8; // Floating point exception
    };
    return 5;       // GDB SIGTRAP                                                                                                                                                                
}

Iss2::debug_register_t Mips32Iss::debugGetRegisterValue(unsigned int reg) const
{
    switch (reg)
        {
        case 0:
            return 0;
        case 1 ... 31:
            return soclib::endian::uint32_swap(r_gp[reg]);
        case 32:
            return soclib::endian::uint32_swap(r_status.whole);
        case 33:
            return soclib::endian::uint32_swap(r_lo);
        case 34:
            return soclib::endian::uint32_swap(r_hi);
        case 35:
            return soclib::endian::uint32_swap(r_bar);
        case 36:
            return soclib::endian::uint32_swap(r_cause.whole);
        case 37:
            return soclib::endian::uint32_swap(r_pc);
        default:
            return 0;
        }
}

void Mips32Iss::debugSetRegisterValue(unsigned int reg, debug_register_t value)
{
    value = soclib::endian::uint32_swap(value);

    switch (reg)
        {
        case 1 ... 31:
            r_gp[reg] = value;
            break;
        case 32:
            r_status.whole = value;
            break;
        case 33:
            r_lo = value;
            break;
        case 34:
            r_hi = value;
            break;
        case 35:
            r_bar = value;
            break;
        case 36:
            r_cause.whole = value;
            break;
        case 37:
            r_pc = value;
            r_npc = value+4;
            break;
        default:
            break;
        }
}

namespace {
static size_t lines_to_s( size_t lines )
{
    return clamp<size_t>(0, uint32_log2(lines)-6, 7);
}
static size_t line_size_to_l( size_t line_size )
{
    if ( line_size == 0 )
        return 0;
    return clamp<size_t>(1, uint32_log2(line_size/4)+1, 7);
}
}

void Mips32Iss::setICacheInfo( size_t line_size, size_t assoc, size_t n_lines )
{
    r_config1.ia = assoc-1;
    r_config1.is = lines_to_s(n_lines);
    r_config1.il = line_size_to_l(line_size);
}

void Mips32Iss::setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines )
{
    r_config1.da = assoc-1;
    r_config1.ds = lines_to_s(n_lines);
    r_config1.dl = line_size_to_l(line_size);
}

Mips32Iss::addr_t Mips32Iss::exceptOffsetAddr( enum ExceptCause cause ) const
{
    if ( r_cause.iv ) {
        if ( r_status.bev || !r_intctl.vs )
            return 0x200;
        else {
            int vn;

            if ( r_config3.veic )
                vn = r_cause.ip>>2;
            else {
                // TODO
                SOCLIB_WARNING("Handling exception offset address when iv and !bev is still to do !");
                vn = 0;
            }
            return 0x200 + vn * (r_intctl.vs<<5);
        }
    } else {
        return 0x180;
    }
}

Mips32Iss::addr_t Mips32Iss::exceptBaseAddr() const
{
    if ( r_status.bev )
        return 0xbfc00200;
    else
        return r_ebase & 0xfffff000;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
