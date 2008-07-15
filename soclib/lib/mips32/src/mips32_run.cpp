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

#define op(x) &Mips32Iss::op_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)

Mips32Iss::func_t const Mips32Iss::opcod_table[]= {
    op4(special, bcond,    j,   jal),
    op4(    beq,   bne, blez,  bgtz),

    op4(   addi, addiu, slti, sltiu),
    op4(   andi,   ori, xori,   lui),

    op4(   cop0,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(special2,  ill,  ill,special3),

    op4(     lb,    lh,  lwl,    lw),
    op4(    lbu,   lhu,  lwr,   ill),

    op4(     sb,    sh,  swl,    sw),
    op4(    ill,   ill,  swr, cache),

    op4(     ll,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     sc,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};

#undef op
#define op(x) #x

const char *Mips32Iss::name_table[] = {
    op4(special, bcond,    j,   jal),
    op4(    beq,   bne, blez,  bgtz),

    op4(   addi, addiu, slti, sltiu),
    op4(   andi,   ori, xori,   lui),

    op4(   cop0,   ill, cop2,   ill),
    op4(   beql,  bnel,blezl, bgtzl),

    op4(    ill,   ill,  ill,   ill),
    op4(special2,  ill,  ill,   ill),

    op4(     lb,    lh,  lwl,    lw),
    op4(    lbu,   lhu,  lwr,   ill),

    op4(     sb,    sh,  swl,    sw),
    op4(    ill,   ill,  swr, cache),

    op4(     ll,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     sc,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};
#undef op
#undef op4

void Mips32Iss::run()
{
    func_t func = opcod_table[m_ins.i.op];

    if (isHighPC() && isInUserMode()) {
        m_exception = X_SYS;
        return;
    }

    (this->*func)();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
