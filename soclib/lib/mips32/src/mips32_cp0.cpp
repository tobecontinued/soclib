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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 *
 * History:
 * - 2007-09-19
 *   Nicolas Pouillon: Fix overflow
 *
 * - 2007-06-15
 *   Nicolas Pouillon, Alain Greiner: Model created
 */

#include "mips32.h"
#include "base_module.h"
#include "arithmetics.h"

#include <strings.h>

namespace soclib { namespace common {

#ifndef SOCLIB_MODULE_DEBUG
#define MIPS32_DEBUG 0
#else
#define MIPS32_DEBUG 1
#endif

#define COPROC_REGNUM(no, sel) (((no)<<3)+sel)

enum Cp0Reg {
    INDEX = COPROC_REGNUM(0,0),
    BAR = COPROC_REGNUM(8,0),
    COUNT = COPROC_REGNUM(9,0),
    STATUS = COPROC_REGNUM(12,0),
    CAUSE = COPROC_REGNUM(13,0),
    EPC = COPROC_REGNUM(14,0),
    EBASE = COPROC_REGNUM(15,1),
    CONFIG = COPROC_REGNUM(16,0),
    CONFIG_1 = COPROC_REGNUM(16,1),

    // Implementation dependant,
    // count of non-frozen cycles
    EXEC_CYCLES = COPROC_REGNUM(9,6),
};

static inline uint32_t merge(uint32_t oldval, uint32_t newval, uint32_t newmask)
{
    return (oldval & ~newmask) | (newval & newmask);
}

uint32_t Mips32Iss::cp0Get( uint32_t reg, uint32_t sel ) const
{
    switch(COPROC_REGNUM(reg,sel)) {
    case INDEX:
        return m_ident;
    case BAR:
        return r_bar;
    case COUNT:
        return r_count;
    case STATUS:
        return r_status.whole;
    case CAUSE:
        return r_cause.whole;
    case EPC:
        return r_epc;
    case EBASE:
        return r_ebase;
    case EXEC_CYCLES:
        return m_exec_cycles;
    case CONFIG:
        return m_config.whole;
    case CONFIG_1:
        return m_config1.whole;
    default:
        return 0;
    }
}

void Mips32Iss::cp0Set( uint32_t reg, uint32_t sel, uint32_t val )
{
    switch(COPROC_REGNUM(reg, sel)) {
    case STATUS:
        r_status.whole = val;
        return;
    case EBASE:
        r_ebase = merge(r_ebase, val, 0x3ffff000);
        break;
    default:
        return;
    }
}

void Mips32Iss::update_mode()
{
	switch (r_status.ksu) {
	case MIPS32_KERNEL_MODE:
		r_cpu_mode = MODE_KERNEL;
		break;
	case MIPS32_SUPERVISOR_MODE:
		r_cpu_mode = MODE_HYPER;
		break;
	case MIPS32_USER_MODE:
		r_cpu_mode = MODE_USER;
		break;
	case MIPS32_RESERVED_MODE:
		assert(0&&"Invalid user mode set in status register");
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
