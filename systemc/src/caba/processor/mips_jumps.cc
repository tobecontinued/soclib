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

// TODO
#define check_privs() do{}while(0)

#define sign_ext16(i) ((int32_t)(int16_t)(uint16_t)i)

void Mips::op_bcond()
{
	bool taken;

	taken = (int32_t)gpr_rs < 0;
	taken ^= (bool)(ins.i.rt & 1);

	// and link ?
	if (ins.i.rt & 0x20)
		gpr[31] = pc+8;

	if (taken)
		future_next_pc = sign_ext16(ins.i.i)*4+pc+4;
}

void Mips::op_j()
{
	future_next_pc = (pc&0xf0000000) | (ins.j*4);
}

void Mips::op_jal()
{
	gpr[31] = pc+8;
	future_next_pc = (pc&0xf0000000) | (ins.j*4);
}

void Mips::op_beq()
{
	if ( gpr_rs == gpr_rt )
		future_next_pc = sign_ext16(ins.i.i)*4+pc+4;
}

void Mips::op_bne()
{
	if ( gpr_rs != gpr_rt )
		future_next_pc = sign_ext16(ins.i.i)*4+pc+4;
}

void Mips::op_blez()
{
	if ( (int32_t)gpr_rs <= 0 )
		future_next_pc = sign_ext16(ins.i.i)*4+pc+4;
}

void Mips::op_bgtz()
{
	if ( (int32_t)gpr_rs > 0 )
		future_next_pc = sign_ext16(ins.i.i)*4+pc+4;
}

void Mips::op_addi()
{
	uint64_t tmp = (uint64_t)gpr_rs - (uint64_t)sign_ext16(ins.i.i);
	if ( (bool)(tmp&(uint64_t)((uint64_t)1<<32)) != (bool)(tmp&(1<<31)) )
		except = X_OV;
	else
		gpr[ins.i.rt] = tmp;
}

void Mips::op_addiu()
{
	gpr[ins.i.rt] = gpr_rs + sign_ext16(ins.i.i);
}

void Mips::op_slti()
{
	gpr[ins.i.rt] = (bool)
		((int32_t)gpr_rs < (int32_t)sign_ext16(ins.i.i));
}

void Mips::op_sltiu()
{
	gpr[ins.i.rt] = (bool)
		((int32_t)gpr_rs < sign_ext16(ins.i.i));
}

void Mips::op_andi()
{
	gpr[ins.i.rt] = gpr_rs & ins.i.i;
}

void Mips::op_ori()
{
	gpr[ins.i.rt] = gpr_rs | ins.i.i;
}

void Mips::op_xori()
{
	gpr[ins.i.rt] = gpr_rs ^ ins.i.i;
}

void Mips::op_lui()
{
	gpr[ins.i.rt] = ins.i.i << 16;
}

#define case2(x) case x: case (x+1)
#define case4(x) case2(x): case2(x+2)
#define case8(x) case4(x): case4(x+4)
#define case16(x) case8(x): case8(x+8)

void Mips::op_copro()
{
	check_privs();

	switch (ins.r.rs) {
	case16(0x10):
		// C0 ins
		switch ( ins.r.func ) {
		case 0x10: // rfe
			cp0_reg[CP0_STATUS] = (cp0_reg[CP0_STATUS]&(~0xf)) | ((cp0_reg[CP0_STATUS]>>2)&0xf);
			return;
		}
		// Not handled, so raise except
	default:
		op_ill();
	case 4:
		// mtc0
		cp0_reg[ins.r.rd] = gpr_rt;
		break;
	case 0:
		// mfc0
		gpr[ins.r.rt] = cp0_reg[ins.r.rd];
		break;
	}
}

void Mips::op_ill()
{
	except = X_RI;
}

void Mips::op_lb()
{
	mem_access = MEM_LB;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_dest = ins.i.rt;
}

void Mips::op_lh()
{
	mem_access = MEM_LH;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_dest = ins.i.rt;
}

void Mips::op_lw()
{
	if ( ins.i.rt )
		mem_access = MEM_LW;
	else {
		std::cout
			<< "Warning, 'lw $0,x(reg)' is not portable\n" << std::endl;
		mem_access = MEM_INVAL;
	}
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_dest = ins.i.rt;
}

void Mips::op_lbu()
{
	mem_access = MEM_LBU;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_dest = ins.i.rt;
}

void Mips::op_lhu()
{
	mem_access = MEM_LHU;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_dest = ins.i.rt;
}

void Mips::op_sb()
{
	uint8_t tmp = gpr_rt;
	mem_access = MEM_SB;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_data = tmp
		| (tmp << 8)
		| (tmp << 16)
		| (tmp << 24);
}

void Mips::op_sh()
{
	uint16_t tmp = gpr_rt;
	mem_access = MEM_SH;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_data = tmp | (tmp << 16);
}

void Mips::op_sw()
{
	mem_access = MEM_SW;
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_data = gpr_rt;
}

void Mips::op_cache()
{
	mem_addr = gpr_rs+sign_ext16(ins.i.i);
	mem_data = gpr_rt;
	switch (ins.i.rt) {
	case 0:
		// Flush
		mem_access = MEM_INVAL;
	}
}

}}
