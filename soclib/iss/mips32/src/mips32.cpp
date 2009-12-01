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
#include "mips32.hpp"
#include "base_module.h"
#include "soclib_endian.h"
#include "arithmetics.h"
#include <iostream>
#include <iomanip>

namespace soclib { namespace common {

Mips32Iss::Mips32Iss(const std::string &name, uint32_t ident, bool default_little_endian)
    : Iss2(name, ident),
      m_little_endian(default_little_endian)
{
    r_config.whole = 0;
    r_config.m = 1;
    r_config.be = m_little_endian ? 0 : 1;
    r_config.ar = 1;
    r_config.mt = 7; // Reserved, let's say it's soclib generic MMU :)

    r_config1.whole = 0;
    r_config1.m = 1;
    r_config1.c2 = 1; // Advertize for Cop2 presence, i.e. generic MMU access

    r_config2.whole = 0;
    r_config2.m = 1;

    r_config3.whole = 0;
    r_config3.ulri = 1; // Advertize for TLS register

    m_cache_info.has_mmu = false;
}

void Mips32Iss::reset()
{
    struct DataRequest null_dreq = ISS_DREQ_INITIALIZER;
    r_ebase = 0x80000000 | m_ident;
    r_pc = m_reset_address;
    r_npc = m_reset_address + 4;
    m_ifetch_addr = m_reset_address;
    m_next_pc = m_jump_pc = (uint32_t)-1;
    r_cpu_mode = MIPS32_KERNEL;
    m_ibe = false;
    m_dbe = false;
    m_dreq = null_dreq;
    r_mem_dest = NULL;
    m_ins_delay = 0;
    r_status.whole = 0x00400004;
    r_cause.whole = 0;
    m_exec_cycles = 0;
    r_gp[0] = 0;
    m_microcode_func = NULL;
    r_count = 0;
    r_compare = 0;
    r_tls_base = 0;
    r_hwrena = 0;

    r_bus_mode = MODE_KERNEL;

	r_status.fr = 0;

    r_fir.whole = 0;
	r_fir.w = 1;
	r_fir.d = 1;
	r_fir.s = 1;
	r_fir.processorID = m_ident;

	r_fcsr.whole = 0;

	// Default values
	r_fcsr.enables_v = 1;
	r_fcsr.enables_z = 1;
	r_fcsr.enables_o = 1;
	r_fcsr.enables_u = 1;
	r_fcsr.enables_i = 1;

    for(int i = 0; i<32; i++)
        r_gp[i] = 0;

    m_hazard=false;
    m_exception = NO_EXCEPTION;
    update_mode();
}

void Mips32Iss::dump() const
{
    std::cout
        << std::hex << std::noshowbase
        << m_name
        << " PC: " << r_pc
        << " NPC: " << r_npc
        << " Ins: " << m_ins.ins << std::endl
        << std::dec
        << " Cause.xcode: " << r_cause.xcode << std::endl
        << " Mode: " << r_cpu_mode
        << " Status.ksu " << r_status.ksu
        << " .exl: " << r_status.exl
        << " .erl: " << r_status.erl
        << " .whole: " << std::hex << r_status.whole
        << std::endl
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
        std::cout
			<< " " << std::dec << std::setw(2) << i << ": "
			<< std::hex << std::noshowbase << std::setw(8) << std::setfill('0')
			<< r_gp[i];
        if ( i%8 == 7 )
            std::cout << std::endl;
    }
}

#define RUN_FOR(x)                                      \
    do { uint32_t __tmp = (x);                          \
        ncycle -= __tmp;                                \
        r_count += __tmp;                               \
        time_spent += __tmp;                            \
        m_ins_delay -= std::min(m_ins_delay, __tmp);    \
    } while(0)


uint32_t Mips32Iss::executeNCycles(
                                   uint32_t ncycle,
                                   const struct InstructionResponse &irsp,
                                   const struct DataResponse &drsp,
                                   uint32_t irq_bit_field )
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << name()
        << " executeNCycles( "
        << ncycle << ", "
        << irsp << ", "
        << drsp << ", "
        << irq_bit_field << ")"
        << std::endl;
#endif

    m_irqs = irq_bit_field;
    r_cause.ripl = irq_bit_field;
    m_exception = NO_EXCEPTION;
    m_jump_pc = r_npc;
    m_next_pc = r_pc;
    m_hazard = false;
    m_resume_pc = r_pc;

    uint32_t time_spent = 0;

    if ( m_ins_delay )
        RUN_FOR(std::min(m_ins_delay, ncycle));

    // The current instruction is executed in case of interrupt, but
    // the next instruction will be delayed.

    // The current instruction is not executed in case of exception,
    // and there is three types of bus error events, in order of
    // increasing priority:
    // 1 - instruction bus errors
    // 2 - read data bus errors
    // 3 - write data bus errors

    bool may_take_irq = check_irq_state();
    bool ireq_ok = handle_ifetch(irsp);
    bool dreq_ok = handle_dfetch(drsp);

    if ( m_hazard && ncycle )
        RUN_FOR(1);

    if ( m_exception != NO_EXCEPTION )
        goto got_exception;

    if ( ncycle == 0 )
        goto early_end;

    RUN_FOR(1);

    if ( dreq_ok && ireq_ok ) {
#ifdef SOCLIB_MODULE_DEBUG
        dump();
#endif
        if ( m_microcode_func ) {
            (this->*m_microcode_func)();
        } else {
            m_next_pc = r_npc;
            m_jump_pc = r_npc+4;
            m_resume_pc = r_pc;
            run();
        }
        if ( m_dreq.valid ) {
            m_pc_for_dreq = r_pc;
            m_pc_for_dreq_is_ds = m_next_pc != r_pc+4;
        }
        m_exec_cycles++;
    }

    if ( m_exception != NO_EXCEPTION )
        goto got_exception;

    if ( (r_status.im & r_cause.ip) && may_take_irq && check_irq_state() )
        goto handle_irq;

    r_npc = m_jump_pc;
    r_pc = m_next_pc;
    m_ifetch_addr = m_next_pc;
    r_gp[0] = 0;
#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << std::hex << std::showbase
        << m_name
        << " m_next_pc: " << m_next_pc
        << " m_jump_pc: " << m_jump_pc
        << " m_ifetch_addr: " << m_ifetch_addr
        << std::endl;
#endif
    goto early_end;

 handle_irq:
    m_resume_pc = m_next_pc;
    m_exception = X_INT;
 got_exception:
    handle_exception();
    return time_spent;
 early_end:
#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << std::hex << std::showbase
        << m_name
        << " early_end:"
        << " ireq_ok=" << ireq_ok
        << " dreq_ok=" << dreq_ok
        << " m_ins_delay=" << m_ins_delay
        << " ncycle=" << ncycle
        << " time_spent=" << time_spent
        << std::endl;
#endif
    return time_spent;
}

bool Mips32Iss::handle_ifetch(
                              const struct InstructionResponse &irsp
                              )
{
    if ( m_microcode_func ) {
        return true;
    }

    if ( m_ifetch_addr != r_pc || !irsp.valid ) {
        return false;
    }

    if ( irsp.error ) {
        m_exception = X_IBE;
        m_resume_pc = m_ifetch_addr;
        return true;
    }

    m_ins.ins = irsp.instruction;
    if ( ! m_little_endian )
        m_ins.ins = soclib::endian::uint32_swap(irsp.instruction);

    return true;
}

void Mips32Iss::handle_exception()
{
    m_microcode_func = NULL;
    ExceptionClass ex_class = EXCL_FAULT;
    ExceptionCause ex_cause = EXCA_OTHER;

    switch (m_exception) {
    case X_INT:
        ex_class = EXCL_IRQ;
        break;
    case X_SYS:
        ex_class = EXCL_SYSCALL;
        break;
    case X_BP:
    case X_TR:
        ex_class = EXCL_TRAP;
        break;

    case X_MOD:
    case X_reserved:
        abort();

    case X_TLBL:
    case X_TLBS:
        ex_cause = EXCA_PAGEFAULT;
        break;
    case X_ADEL:
    case X_ADES:
        ex_cause = EXCA_ALIGN;
        break;
    case X_IBE:
    case X_DBE:
        ex_cause = EXCA_BADADDR;
        break;
    case X_RI:
    case X_CPU:
        ex_cause = EXCA_ILL;
        break;
    case X_OV:
    case X_FPE:
        ex_cause = EXCA_FPU;
        break;

    default:
        assert(!"This must not happen");
    }

    if ( debugExceptionBypassed( ex_class, ex_cause ) )
        return;

    addr_t except_address = exceptBaseAddr();
    bool branch_taken = m_next_pc+4 != m_jump_pc;

    if ( m_resume_pc == r_pc )
        branch_taken = r_pc+4 != r_npc;

    if ( m_exception == X_DBE ) {
        branch_taken = m_pc_for_dreq_is_ds;
        m_resume_pc = m_pc_for_dreq;
    }

    if ( r_status.exl ) {
        except_address += 0x180;
    } else {
        r_cause.bd = branch_taken;
        r_epc = m_resume_pc - 4*branch_taken;
        except_address += exceptOffsetAddr(m_exception);
    }
    r_cause.ce = 0;
    r_cause.xcode = m_exception;
    r_status.exl = 1;
    update_mode();

#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << m_name <<" exception: "<<m_exception<<std::endl
        << " m_ins: " << m_ins.j.op
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

    r_pc = except_address;
    r_npc = except_address+4;
    m_ifetch_addr = except_address;
}

Iss2::debug_register_t Mips32Iss::debugGetRegisterValue(unsigned int reg) const
{
    switch (reg)
        {
        case 0:
            return 0;
        case 1 ... 31:
            return r_gp[reg];
        case 32:
            return r_status.whole;
        case 33:
            return r_lo;
        case 34:
            return r_hi;
        case 35:
            return r_bar;
        case 36:
            return r_cause.whole;
        case 37:
            return r_pc;
        default:
            return 0;
        }
}

void Mips32Iss::debugSetRegisterValue(unsigned int reg, debug_register_t value)
{
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

void Mips32Iss::setCacheInfo( const struct CacheInfo &info )
{
    r_config1.ia = info.icache_assoc-1;
    r_config1.is = lines_to_s(info.icache_n_lines);
    r_config1.il = line_size_to_l(info.icache_line_size);
    r_config1.da = info.dcache_assoc-1;
    r_config1.ds = lines_to_s(info.dcache_n_lines);
    r_config1.dl = line_size_to_l(info.dcache_line_size);
    m_cache_info = info;
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

void Mips32Iss::do_microcoded_sleep()
{
    if ( m_irqs ) {
        m_microcode_func = NULL;
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " IRQ while sleeping" << std::endl;
#endif
    }
}

uint32_t Mips32Iss::m_reset_address = 0xbfc00000;

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
