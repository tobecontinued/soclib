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
 * Copyright (c) Telecom ParisTech
 *         Alexis Polti <polti@telecom-paristech.fr>
 *
 * Maintainers: Alexis Polti
 *
 * $Id$
 */

#include "sparcv8.h"
#include "soclib_endian.h"
#include <cstring>
#include <iomanip>

namespace soclib { namespace common {

#define tmpl(x) template<unsigned int NWINDOWS> x Sparcv8Iss<NWINDOWS>

tmpl(void)::dump_pc(const std::string &msg) const
{
    std::cout
        << msg << std::endl
        << std::hex << std::showbase
        << "\tPC=" << std::setw(10) << std::setfill('0') << std::internal << std::hex << std::showbase << r_pc
        << " / NPC=" << std::setw(10) << std::setfill('0') << std::internal << std::hex << std::showbase << r_npc 
        << " / next_pc=" << std::setw(10) << std::setfill('0') << std::internal << std::hex << std::showbase << m_next_pc << std::endl
        << "\tIns=" << std::setw(10) << std::setfill('0') << std::internal << std::hex << std::showbase << m_ins.ins 
        << " (" << get_ins_name() << ")" << std::endl
        ;
}


tmpl(void)::dump_regs(const std::string &msg) const
{
#define DUMP_REG(n, prefix, val)                                            \
    std::cout <<  prefix << std::setw(1) << std::dec << n << "="            \
              << "0x" << std::setw(8) << std::setfill('0') << std::internal \
              << std::hex << std::noshowbase << val << " ";
    
    std::cout 
        << msg << std::endl
        << "\tPSR=" << std::setw(10) << std::setfill('0') << std::internal << std::hex << std::showbase << r_psr.whole
        << " (N=" << r_psr.n 
        << " Z=" << r_psr.z
        << " V=" << r_psr.v 
        << " C=" << r_psr.c 
        << " EC=" << r_psr.ec
        << " EF=" << r_psr.ef 
        << " PIL=" << r_psr.pil 
        << " PS=" << r_psr.ps
        << " ET=" << r_psr.et
        << " CWP="<<r_psr.cwp 
        << ")" << std::endl
        << "\tWIM=" << r_wim << std::endl;

    std::cout << "\t";
    for ( size_t i=0; i<8; ++i ) 
        DUMP_REG(i, "g", GPR(i));
    std::cout << std::endl;
    
    std::cout << "\t";
    for ( size_t i=0; i<8; ++i ) 
        DUMP_REG(i, "o", GPR(i+8));
    std::cout << std::endl;
    
    std::cout << "\t";
    for ( size_t i=0; i<8; ++i ) 
        DUMP_REG(i, "l", GPR(i+16));
    std::cout << std::endl;
    
    std::cout << "\t";
    for ( size_t i=0; i<8; ++i ) 
        DUMP_REG(i, "i", GPR(i+24));
    std::cout << std::endl;

#if FPU
    std::cout << std::endl << "\t";
    for ( size_t i=0; i<32; ++i ) {
        DUMP_REG(i, "ff", r_f[i]);
        if (i%8 == 7)
            std::cout << std::endl << "\t";
    }
    std::cout << std::endl;
#endif
    
#undef DUMP_REG
    
}



tmpl(int)::debugCpuCauseToSignal( uint32_t cause ) const
{
    switch (cause) {

    case TP_INSTRUCTION_ACCESS_ERROR:
    case TP_INSTRUCTION_ACCESS_EXCEPTION:
    case TP_DATA_ACCESS_ERROR:
    case TP_DATA_ACCESS_EXCEPTION:
        return 11;      // SIGSEGV

    case TP_ILLEGAL_INSTRUCTION:
    case TP_PRIVILEGED_INSTRUCTION:
    case TP_UNIMPLEMENTED_INSTRUCTION:
        return 4;       // SIGILL

    case TP_FP_DISABLED:
    case TP_CP_DISABLED:
    case TP_TAG_OVERFLOW:
        return 7;       // SIGEMT

    case TP_MEM_ADDRESS_NOT_ALIGNED:
        return 10;      // SIGBUS

    case TP_INTERRUPT_LEVEL(0) ... TP_INTERRUPT_LEVEL(0xf):
    case TP_RESET :
        return 2;       // SIGINT

    case TP_TRAP_INSTRUCTION(0) ... TP_TRAP_INSTRUCTION(0x7f):
        return 5;       // SIGTRAP

    case TP_DIVISION_BY_ZERO:
    case TP_FP_EXCEPTION:
        return 8;       // SIGFPE

    case TP_INSTRUCTION_ACCESS_MMU_MISS:
    case TP_DATA_ACCESS_MMU_MISS:
        return 11;      // SIGSEGV (found nothing better...)

    case TP_WINDOW_OVERFLOW:
    case TP_WINDOW_UNDERFLOW:
        return 2;       // SIGINT (found nothing better...)

    default:
        return 5;       // GDB SIGTRAP
    }
}

tmpl(Iss2::debug_register_t)::debugGetRegisterValue(unsigned int reg) const
{
    // TODO : implement this !
    return 0;
}


tmpl(void)::debugSetRegisterValue(unsigned int reg, debug_register_t value)
{
    // TODO : implement this !
}

tmpl(void)::setICacheInfo( size_t line_size, size_t assoc, size_t n_lines )
{
}

tmpl(void)::setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines )
{
}

tmpl(unsigned int)::debugGetRegisterCount() const
{
    // TODO : check this !
    return 32 + 6;
}

tmpl(size_t)::debugGetRegisterSize(unsigned int reg) const
{
    return 32;
}

}}


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

