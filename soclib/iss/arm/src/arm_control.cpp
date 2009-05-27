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
 *         Alexandre Becoulet <alexandre.becoulet@free.fr>, 2009
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo becoulet
 *
 * $Id$
 *
 */

#include "arm.h"

namespace soclib { namespace common {

bool ArmIss::cond_eval() const
{
	uint16_t cond_word = cond_table[m_opcode.dp.cond];
	return (cond_word >> r_cpsr.flags) & 1;
}

void ArmIss::run()
{
	if ( ! cond_eval() )
		return;
	int8_t id = decod_main(m_opcode.ins);
	func_t func = funcs[id];
	(this->*func)();
}

uint16_t const ArmIss::cond_table[16] = {
    /* EQ */ 0xf0f0, /* NE */ 0x0f0f, /* CS */ 0xcccc, /* CC */ 0x3333,
    /* MI */ 0xff00, /* PL */ 0x00ff, /* VS */ 0xaaaa, /* VC */ 0x5555,
    /* HI */ 0x0c0c, /* LS */ 0xf3f3, /* GE */ 0xaa55, /* LT */ 0x55aa,
    /* GT */ 0x0a05, /* LE */ 0xf5fa, /* AL */ 0xffff, /* NV */ 0x0000,
};

}}

