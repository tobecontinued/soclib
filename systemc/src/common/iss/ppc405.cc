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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 */

#include "common/iss/ppc405.h"
#include "common/endian.h"

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
	snprintf(tmp, 32, "ppc405_iss%d", (int)no);
	return std::string(tmp);
}

static inline std::string crTrad( uint32_t cr )
{
    const char *orig = "<>=o";
    char dest[5] = "    ";
    
    for ( size_t i=0; i<4; ++i )
        if ( cr & (1<<i) )
            dest[i] = orig[i];

    return dest;
}

}

Ppc405Iss::Ppc405Iss(uint32_t ident)
	: Iss(mkname(ident), ident)
{
}

void Ppc405Iss::reset()
{
    Iss::reset(RESET_ADDR);
    r_tb = 0;
    r_esr = 0;
    r_msr.whole = 0;
    for ( size_t i=0; i<DCR_MAX; ++i )
        r_dcr[i] = 0;
    r_dcr[DCR_PROCNUM] = m_ident;
}

void Ppc405Iss::dump() const
{
    std::cout
        << m_name << std::hex
        << " PC: " << r_pc
        << " Ins: " << m_ins.ins
        << " lr: " << r_lr
        << " msr " << r_msr.whole
        << " .ce " << r_msr.ce
        << " .ee " << r_msr.ee
        << " .pr " << r_msr.pr
        << " irq: " << m_irq
        << std::endl;
    for ( size_t i=0; i<32; ++i ) {
        std::cout << " " << std::dec << i << ": " << std::hex << std::showbase << r_gp[i];
        if ( i%8 == 7 )
            std::cout << std::endl;
    }
    for ( size_t i=0; i<8; ++i ) {
        std::cout << " " << std::dec << i << ": " << crTrad(crGet(i));
    }
    std::cout << std::endl;
}

void Ppc405Iss::step()
{
    r_tb++;

    m_next_pc = r_pc+4;

    m_exception = EXCEPT_NONE;

    // IRQs
    r_dcr[DCR_EXTERNAL] = !!(m_irq&(1<<IRQ_EXTERNAL));
    r_dcr[DCR_CRITICAL] = !!(m_irq&(1<<IRQ_CRITICAL_INPUT));

    if (m_ibe) {
        m_exception = EXCEPT_INSTRUCTION_STORAGE;
        r_esr = ESR_DIZ;
        goto handle_except;
    }

    if ( m_dbe ) {
        m_exception = EXCEPT_MACHINE_CHECK;
        r_esr = ESR_MCI;
        goto handle_except;
    }

    switch (r_mem_type ) {
    default:
        break;
    case MEM_LW:
        r_gp[r_mem_dest] = m_rdata;
        break;
    case MEM_LWBR:
        r_gp[r_mem_dest] = soclib::endian::uint32_swap(m_rdata);
        break;
    case MEM_LB:
        r_gp[r_mem_dest] = (int32_t)(int8_t)align(m_rdata, 3-r_mem_addr&0x3, 8);
        break;
    case MEM_LBU:
        r_gp[r_mem_dest] = align(m_rdata, 3-r_mem_addr&0x3, 8);
        break;
    case MEM_LH:
        r_gp[r_mem_dest] = (int32_t)(int16_t)align(m_rdata, 1-(r_mem_addr&0x2)/2, 16);
        break;
    case MEM_LHBR:
        r_gp[r_mem_dest] = (int32_t)(int16_t)soclib::endian::uint16_swap(align(m_rdata, 1-(r_mem_addr&0x2)/2, 16));
        break;
    case MEM_LHU:
        r_gp[r_mem_dest] = align(m_rdata, 1-(r_mem_addr&0x2)/2, 16);
        break;
    }
    r_mem_type = MEM_NONE;

    if ( r_dbe ) {
        m_exception = EXCEPT_DATA_STORAGE;
        r_dbe = false;
        r_esr = ESR_DST;
        r_dear = r_mem_addr;
        goto handle_except;
    }

#if PPC405_DEBUG
    if (r_mem_type > MEM_NONE && r_mem_type < MEM_SB)
        std::cout << m_name << " read to " << r_mem_dest << "(" << r_mem_type << ") from "
                  << std::hex << r_mem_addr << ": " << m_rdata << std::endl;
    dump();
#endif
    run();

    if (m_exception != EXCEPT_NONE)
        goto handle_except;

    if ( r_msr.ce && r_dcr[DCR_CRITICAL] ) {
        m_exception = EXCEPT_CRITICAL;
        goto handle_except;
    }

    if ( r_msr.ee && r_dcr[DCR_EXTERNAL] ) {
        m_exception = EXCEPT_EXTERNAL;
        goto handle_except;
    }

    goto no_except;

  handle_except:
#if PPC405_DEBUG
    std::cout << m_name << " except: " << m_exception << std::endl;
#endif

    // 1/2 : Save status to SRR
    {
        int except_base = 0;
        uint32_t ra = (m_exception == EXCEPT_SYSCALL ? 4:0) + r_pc;
        switch (m_exception) {
        case EXCEPT_CRITICAL:
        case EXCEPT_WATCHDOG:
        case EXCEPT_DEBUG:
        case EXCEPT_MACHINE_CHECK:
            except_base += 2;
        default:
            r_srr[except_base+0] = ra;
            r_srr[except_base+1] = r_msr.whole;
        }
    }

    // 3: Update ESR (Done)
    // 4: Update DEAR (Done)

    // 5: Load new program state in MSR
    {
        msr_t new_msr;
        new_msr.whole = 0;
        if ( m_exception != EXCEPT_CRITICAL &&
             m_exception != EXCEPT_MACHINE_CHECK &&
             m_exception != EXCEPT_WATCHDOG &&
             m_exception != EXCEPT_DEBUG ) {
            new_msr.ce = r_msr.ce;
            new_msr.de = r_msr.de;
        }
        if ( m_exception != EXCEPT_MACHINE_CHECK )
            new_msr.me = r_msr.me;
        r_msr = new_msr;
    }

    // 7: Load next instruction address
    m_next_pc = r_evpr+except_addresses[m_exception];

  no_except:
    r_pc = m_next_pc;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
