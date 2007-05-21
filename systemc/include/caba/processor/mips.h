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
#ifndef SOCLIB_CABA_MIPS_H_
#define SOCLIB_CABA_MIPS_H_

#include <inttypes.h>
#include "caba/util/base_module.h"
#include "common/endian.h"
#include "caba/interface/xcache_processor_ports.h"

namespace soclib { namespace caba {

using namespace soclib;

class Mips
	: soclib::caba::BaseModule
{
public:
	soclib::caba::ICacheProcessorPort p_icache;
	soclib::caba::DCacheProcessorPort p_dcache;
	sc_in<bool> p_irq[6];
	sc_in<bool> p_resetn;
	sc_in<bool> p_clk;

private:
	enum MipsAddresses {
		EXCEPT_ADDR = 0x80000080,
		RESET_ADDR = 0xbfc00000
	};

	enum MipsMemAccess {
		MEM_NONE,
		MEM_LB,
		MEM_LBU,
		MEM_LH,
		MEM_LHU,
		MEM_LW,
		MEM_SB,
		MEM_SH,
		MEM_SW,
		MEM_INVAL
	};

	enum MipsCp0Reg {
		CP0_IDENT = 0,
		CP0_BADVADDR = 8,
		CP0_COUNT = 9,
		CP0_STATUS = 12,
		CP0_CAUSE = 13,
		CP0_EPC = 14,
		CP0_INFOS = 15,
	};

	enum MipsExceptCause {
		X_INT, // Interrupt
		X_MOD, // TLB Modification
		X_TLBL,// TLB Load error
		X_TLBS,// TLB Store error

		X_ADEL,// Address error (load or fetch)
		X_ADES,// Address error (store)
		X_IBE, // Ins bus error
		X_DBE, // Data bus error (load/store)

		X_SYS, // Syscall
		X_BP,  // Break point
		X_RI,  // Reserved
		X_CPU, // Coproc unusable

		X_OV,  // Overflow
		X_TR,  // Trap
		X_reserved, // Reserved
		X_FPE, // Floating point
	};

	sc_signal<uint32_t> gpr[32];
	sc_signal<uint32_t> pc;
	sc_signal<uint32_t> next_pc;
	uint32_t future_next_pc;
	sc_signal<uint32_t> hi;
	sc_signal<uint32_t> lo;

	sc_signal<uint32_t> cp0_reg[32];

	sc_signal<uint32_t> m_interrupt_delayed;

	int id;

	/*
	 * Those aren't registers, they are just
	 * instance-global variables used to port
	 * data accross function calls
	 */
	uint32_t mem_access;
	uint32_t mem_addr;
	uint32_t mem_data;
	uint32_t mem_dest;

	uint32_t gpr_rs;
	uint32_t gpr_rt;

	uint32_t except;

	typedef union {
		struct {
			union {
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t j:26
					);
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rs:5,
					uint32_t rt:5,
					uint32_t i:16
					) i;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rs:5,
					uint32_t rt:5,
					uint32_t rd:5,
					uint32_t sh:5,
					uint32_t func:6
					) r;
			} __attribute__((packed));
		} __attribute__((packed));
		uint32_t ins;
	} ins_t;
	ins_t ins;

	void transition();
	void genMoore();

	void run();

	typedef void (Mips::*func_t)();
	static func_t const opcod_table[64];

	void op_special();
	void op_bcond();
	void op_j();
	void op_jal();
	void op_beq();
	void op_bne();
	void op_blez();
	void op_bgtz();
	void op_addi();
	void op_addiu();
	void op_slti();
	void op_sltiu();
	void op_andi();
	void op_ori();
	void op_xori();
	void op_lui();
	void op_copro();
	void op_ill();
	void op_lb();
	void op_lh();
	void op_lw();
	void op_lbu();
	void op_lhu();
	void op_sb();
	void op_sh();
	void op_sw();
	void op_cache();

	static func_t const special_table[64];

	void special_sll();
	void special_srl();
	void special_sra();
	void special_sllv();
	void special_srlv();
	void special_srav();
	void special_jr();
	void special_jalr();
	void special_sysc();
	void special_brek();
	void special_mfhi();
	void special_mthi();
	void special_mflo();
	void special_mtlo();
	void special_mult();
	void special_multu();
	void special_div();
	void special_divu();
	void special_add();
	void special_addu();
	void special_sub();
	void special_subu();
	void special_and();
	void special_or();
	void special_xor();
	void special_nor();
	void special_slt();
	void special_sltu();
	void special_ill();

protected:
	SC_HAS_PROCESS(Mips);

public:
	Mips( sc_module_name insname, int ident );
 	~Mips();
};

}}

#endif /* SOCLIB_CABA_MIPS_H_ */
