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
// random()
#include <cstdlib>

namespace soclib {
namespace caba {

/*
 * Avoid duplication of source code, this kind of op
 * is easy to bug, and should be easy to debug ;)
 */
static inline uint32_t sll( uint32_t reg, uint32_t sh )
{
	return reg << sh;
}

static inline uint32_t srl( uint32_t reg, uint32_t sh )
{
	return (reg >> sh) & ((1<<(32-sh))-1);
}

static inline uint32_t sra( uint32_t reg, uint32_t sh )
{
	if ( (int32_t)reg < 0 )
		return (reg >> sh) | (~((1<<(32-sh))-1));
	else
		return (reg >> sh) & ((1<<(32-sh))-1);
}

void Mips::special_sll()
{
	gpr[ins.r.rd] = sll(gpr_rt, ins.r.sh);
}

void Mips::special_srl()
{
	gpr[ins.r.rd] = srl(gpr_rt, ins.r.sh);
}

void Mips::special_sra()
{
	gpr[ins.r.rd] = sra(gpr_rt, ins.r.sh);
}

void Mips::special_sllv()
{
	gpr[ins.r.rd] = sll(gpr_rt, gpr_rs&0x1f );
}

void Mips::special_srlv()
{
	gpr[ins.r.rd] = srl(gpr_rt, gpr_rs&0x1f );
}

void Mips::special_srav()
{
	gpr[ins.r.rd] = sra(gpr_rt, gpr_rs&0x1f );
}

void Mips::special_jr()
{
	future_next_pc = gpr_rs;
}

void Mips::special_jalr()
{
	gpr[ins.r.rd] = pc+8;
	future_next_pc = gpr_rs;
}

void Mips::special_sysc()
{
	except = X_SYS;
}

void Mips::special_brek()
{
	except = X_BP;
}

void Mips::special_mfhi()
{
	gpr[ins.r.rd] = hi;
}

void Mips::special_mthi()
{
	hi = gpr_rs;
}

void Mips::special_mflo()
{
	gpr[ins.r.rd] = lo;
}

void Mips::special_mtlo()
{
	lo = gpr_rs;
}

void Mips::special_mult()
{
	int64_t res, a, b;
	a = (int32_t)gpr_rs;
	b = (int32_t)gpr_rt;
	res = a*b;
	hi = res>>32;
	lo = res;
}

void Mips::special_multu()
{
	uint64_t res, a, b;
	a = gpr_rs;
	b = gpr_rt;
	res = a*b;
	hi = res>>32;
	lo = res;
}

void Mips::special_div()
{
	if ( ! gpr_rt ) {
		hi = random();
		lo = random();
		return;
	}
	hi = (int32_t)gpr_rs % (int32_t)gpr_rt;
	lo = (int32_t)gpr_rs / (int32_t)gpr_rt;
}

void Mips::special_divu()
{
	if ( ! gpr_rt ) {
		hi = random();
		lo = random();
		return;
	}
	hi = gpr_rs % gpr_rt;
	lo = gpr_rs / gpr_rt;
}

void Mips::special_add()
{
	uint64_t tmp = (uint64_t)gpr_rs + (uint64_t)gpr_rt;
	if ( (bool)(tmp&(uint64_t)((uint64_t)1<<32)) != (bool)(tmp&(1<<31)) )
		except = X_OV;
	else
		gpr[ins.r.rd] = tmp;
}

void Mips::special_addu()
{
	gpr[ins.r.rd] = gpr_rs + gpr_rt;
}

void Mips::special_sub()
{
	uint64_t tmp = (uint64_t)gpr_rs - (uint64_t)gpr_rt;
	if ( (bool)(tmp&(uint64_t)((uint64_t)1<<32)) != (bool)(tmp&(1<<31)) )
		except = X_OV;
	else
		gpr[ins.r.rd] = tmp;
}

void Mips::special_subu()
{
	gpr[ins.r.rd] = gpr_rs - gpr_rt;
}

void Mips::special_and()
{
	gpr[ins.r.rd] = gpr_rs & gpr_rt;
}

void Mips::special_or()
{
	gpr[ins.r.rd] = gpr_rs | gpr_rt;
}

void Mips::special_xor()
{
	gpr[ins.r.rd] = gpr_rs ^ gpr_rt;
}

void Mips::special_nor()
{
	gpr[ins.r.rd] = ~(gpr_rs | gpr_rt);
}

void Mips::special_slt()
{
	gpr[ins.r.rd] = (bool)
		((int32_t)gpr_rs < (int32_t)gpr_rt);
}

void Mips::special_sltu()
{
	gpr[ins.r.rd] = (bool)
		(gpr_rs < gpr_rt);
}

void Mips::special_ill()
{
	except = X_RI;
}

#define op(x) &Mips::special_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)
Mips::func_t const Mips::special_table[] = {
	op4(  sll,  ill,  srl,  sra),
	op4( sllv,  ill, srlv, srav),

	op4(   jr, jalr,  ill,  ill),
	op4( sysc, brek,  ill,  ill),

	op4( mfhi, mthi, mflo, mtlo),
	op4(  ill,  ill,  ill,  ill),

	op4( mult,multu,  div, divu),
	op4(  ill,  ill,  ill,  ill),

	op4(  add, addu,  sub, subu),
	op4(  and,   or,  xor,  nor),

	op4(  ill,  ill,  slt, sltu),
	op4(  ill,  ill,  ill,  ill),

	op4(  ill,  ill,  ill,  ill),
	op4(  ill,  ill,  ill,  ill),
};

void Mips::op_special()
{
	func_t func = special_table[ins.r.func];
	(this->*func)();
}

}}

