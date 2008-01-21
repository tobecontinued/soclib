/* -*- c++ -*-
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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 */

#include "common/iss/ppc405.h"
#include "common/endian.h"
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
    r_pc = RESET_ADDR;
    r_dbe = false;
    m_ibe = false;
    m_dbe = false;
    r_mem_req = false;
    r_evpr = 0xdead0000;
    r_tb = 0;
    r_esr = 0;
    r_msr.whole = 0;
    for ( size_t i=0; i<DCR_MAX; ++i )
        r_dcr[i] = 0;
    r_dcr[DCR_PROCNUM] = m_ident;
    r_dcr[DCR_EXEC_CYCLES] = 0;
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
        m_ibe = false;
        r_esr = ESR_DIZ;
        goto handle_except;
    }

    if ( m_dbe ) {
        m_exception = EXCEPT_MACHINE_CHECK;
        m_dbe = false;
        r_esr = ESR_MCI;
        goto handle_except;
    }

    if ( r_dbe ) {
        m_exception = EXCEPT_DATA_STORAGE;
        r_dbe = false;
        r_esr = ESR_DST;
        r_dear = r_mem_addr;
        goto handle_except;
    }

    r_dcr[DCR_EXEC_CYCLES]++;
#if PPC405_DEBUG
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

    switch ( m_exception )
        {
        case (EXCEPT_NONE):
        case (EXCEPT_EXTERNAL):
        case (EXCEPT_SYSCALL):
        case (EXCEPT_PI_TIMER):
        case (EXCEPT_FI_TIMER):
            break;
        default:
            if (exceptionBypassed( m_exception ))
                goto stick;
        }

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
  stick:
    ;
}

void Ppc405Iss::setDataResponse(bool error, uint32_t rdata)
{
    r_mem_req = false;
    m_dbe = error;

    uint32_t data = rdata;
    // Swap if PPC does _not_ want reversed data (BE)
    if ( ! r_mem_reversed )
        data = soclib::endian::uint32_swap(data);

#if PPC405_DEBUG
    std::cout << m_name << std::hex
              << " mem access ret " << dataAccessTypeName(r_mem_type)
              << " @: " << r_mem_addr
              << " ->r" << r_mem_dest
              << " rdata: " << data
              << (r_mem_reversed ? " reversed" : "")
              << " error: " << error
              << std::endl;
#endif

    if ( error ) {
        return;
    }

    switch (r_mem_type ) {
    case WRITE_BYTE:
    case WRITE_WORD:
    case WRITE_HALF:
    case LINE_INVAL:
        break;
    case STORE_COND:
    {
        int cr = 0;
        if ( data == 0 ) cr |= CMP_EQ;
        if ( r_xer.so ) cr |= CMP_SO;
        crSet( 0, cr );
        break;
    }
    case READ_WORD:
    case READ_LINKED:
        r_gp[r_mem_dest] = data;
        break;
    case READ_BYTE:
        r_gp[r_mem_dest] = r_mem_unsigned ?
            (data & 0xff) :
            sign_ext8(data);
        break;
    case READ_HALF:
        r_gp[r_mem_dest] = r_mem_unsigned ?
            (data & 0xffff) :
            sign_ext16(data);
        break;
    }
}

uint32_t Ppc405Iss::getDebugRegisterValue(unsigned int reg) const
{
    switch (reg)
        {
        case 0 ... 31:
            return r_gp[reg];
        case 32 ... 63:         // FPU
            return 0;
        case 64:                // pc
            return r_pc;
        case 65:                // ps
            return r_msr.whole;
        case 66:                // cnd
            return r_cr;
        case 67:                // lr
            return r_lr;
        case 68:                // cnt
            return r_ctr;
        case 69:                // xer
            return r_xer.whole;
        case 70:                // mq
            return 0;
        case 71:                // fpscr
            return 0;
        default:
            return 0;
        }
}

size_t Ppc405Iss::getDebugRegisterSize(unsigned int reg) const
{
    switch (reg)
        {
        case 32 ... 63:         // FPU
            return 64;
        default:
            return 32;
        }
}

void Ppc405Iss::setDebugRegisterValue(unsigned int reg, uint32_t value)
{
    switch (reg)
        {
        case 0 ... 31:
            r_gp[reg] = value;
            break;
        case 64:                // pc
            r_pc = value;
            break;
        case 65:                // msr
            r_msr.whole = value;
            break;
        case 66:                // cnd
            r_cr = value;
            break;
        case 67:                // lr
            r_lr = value;
            break;
        case 68:                // cnt
            r_ctr = value;
            break;
        case 69:                // xer
            r_xer.whole = value;
            break;
        }
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
