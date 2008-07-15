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
 *         Nicolas Pouillon <nipo@ssji.net>, 2008
 *
 * Maintainers: nipo
 *
 * $Id$
 */

#include "mips32.h"
#include "base_module.h"
#include "arithmetics.h"

namespace soclib { namespace common {

void Mips32Iss::op_special3()
{
    enum {
        EXT = 0,
		INS = 0x4,
		BSHFL = 0x20,
    };

	enum {
		SEB = 0x10,
		SEH = 0x18,
		WSBH = 0x2,
	};

    switch ( m_ins.r.func ) {
    case EXT:
		size_t size = m_ins.r.rd + 1;
		size_t lsb = m_ins.r.sh;
        r_gp[m_ins.r.rt] = (r_gp[m_ins.r.rs] >> lsb) & ((1<<size)-1);
        break;
    case INS: {
		size_t lsb = m_ins.r.sh;
		size_t msb = m_ins.r.rd;
		data_t mask = (1<<(msb+1)) ^ (1<<lsb);
        r_gp[m_ins.r.rt] = ((r_gp[m_ins.r.rs]<<lsb) & mask) | (r_gp[m_ins.r.rt] & ~mask);
        break;
	}
	case BSHFL: {
		switch ( m_ins.r.sh ) {
		case SEB:
			r_gp[m_ins.r.rd] = sign_ext8(r_gp[m_ins.r.rt]);
			break;
		case SEH:
			r_gp[m_ins.r.rd] = sign_ext16(r_gp[m_ins.r.rt]);
			break;
		case WSBH: {
			data_t tmp = soclib::endian::uint16_swap(r_gp[m_ins.r.rt]);
			data_t tmp2 = soclib::endian::uint16_swap(r_gp[m_ins.r.rt]>>16);
			tmp |= tmp2<<16;
			r_gp[m_ins.r.rd] = tmp;
			break;
		}
		}
	}
    default:
        op_ill();
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
