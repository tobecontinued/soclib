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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "caba/processor/mips.h"

namespace soclib {
namespace caba {

#define op(x) &Mips::op_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)
Mips::func_t const Mips::opcod_table[] = {
	op4(special, bcond,    j,   jal),
	op4(    beq,   bne, blez,  bgtz),

	op4(   addi, addiu, slti, sltiu),
	op4(   andi,   ori, xori,   lui),

	op4(  copro,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),

	op4(    ill,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),

	op4(     lb,    lh,  ill,    lw),
	op4(    lbu,   lhu,  ill,   ill),

	op4(     sb,    sh,  ill,    sw),
	op4(    ill,   ill,  ill,   ill),

	op4(    ill,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),

	op4(    ill,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),
};

#undef op

#if MIPS_DEBUG
#define op(x) #x
static const char *name_table[] = {
	op4(special, bcond,    j,   jal),
	op4(    beq,   bne, blez,  bgtz),

	op4(   addi, addiu, slti, sltiu),
	op4(   andi,   ori, xori,   lui),

	op4(  copro,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),

	op4(    ill,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),

	op4(     lb,    lh,  ill,    lw),
	op4(    lbu,   lhu,  ill,   ill),

	op4(     sb,    sh,  ill,    sw),
	op4(    ill,   ill,  ill, cache),

	op4(    ill,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),

	op4(    ill,   ill,  ill,   ill),
	op4(    ill,   ill,  ill,   ill),
};
#endif

void Mips::run()
{
	func_t func = opcod_table[ins.op];

#if MIPS_DEBUG
	std::cout
		<< std::hex << std::showbase
		<< " Ins: " << ins.ins << std::endl
		<< " op:  " << ins.op << " (" << name_table[ins.op] << ")" << std::endl
		<< " i rs: " << ins.i.rs << " rt: "<<ins.i.rt<< " i: "<<ins.i.i << std::endl
		<< " r rs: " << ins.r.rs << " rt: "<<ins.r.rt<< " rd: "<<ins.r.rd<< " sh: "<<ins.r.sh<< " func: "<<ins.r.func << std::endl;
#endif

	(this->*func)();
}

}}
