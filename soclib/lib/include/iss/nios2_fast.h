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
 * NIOSII Instruction Set Simulator for the Altera NIOSII processor core
 * developed for the SocLib Projet
 * 
 * Copyright (C) IRISA/INRIA, 2007-2008
 *         François Charot <charot@irisa.fr>
 *
 * Contributing authors:
 * 				Delphine Reeb
 * 				François Charot <charot@irisa.fr>
 * 
 * Maintainer: charot
 *
 * History:
 * - summer 2006: First version developed on a first SoCLib template by Reeb and Charot.
 * - september 2007: the model has been completely rewritten and adapted to the SocLib 
 * 						rules defined during the first months of the SocLib ANR project.
 *
 * Functional description:
 * Four files: 
 * 		nios2_fast.h
 * 		nios2_ITypeInst.cpp
 * 		nios2_RTypeInst.cpp
 * 		nios2_customInst.cpp
 * define the Instruction Set Simulator for the NIOSII processor.
 *
 * 
 */

#ifndef _SOCLIB_NIOS2_ISS_H_
#define _SOCLIB_NIOS2_ISS_H_

#include <systemc.h>
#include "iss.h"
#include "soclib_endian.h"
#include "register.h"

#define NIOS2_DEBUG 0
#define INSTRUCTIONMEMORYTRACE 0
#define DEBUGCustom 0

// this class is used to manage a list of late result instructions
class lateResultInstruction {
public:
	uint32_t reg;
	uint32_t cycle;

	lateResultInstruction *next;

	void add(uint32_t);
	void update();
	void print();
};

extern lateResultInstruction *m_startOflriList;

namespace soclib {
namespace common {

class Nios2fIss : public soclib::common::Iss {
public:
	static const int n_irq = 32;

private:
	enum Addresses {
		RESET_ADDRESS = 0x00802000,
		EXCEPTION_HANDLER_ADDRESS = 0x00800820,
		BREAK_HANDLER_ADDRESS = 0x01000020
	};

	// general-purpose registers: some registers having names recognized by the assembler 
	// they can be used by the simulator
	enum genPurposeRegName {
		ET = 24,
		BT = 25,
		GP = 26,
		SP = 27,
		FP = 28,
		EA = 29,
		BA = 30,
		RA = 31
	};

	// control registers: names recognized by the assembler and used by the simulator
	enum ctrlRegisterName {
		status,
		estatus,
		bstatus,
		ienable,
		ipending,
		cpuid
	};

	// NIOSII exception code are not given in the NIOSII documentation
	// MIPS exception code are used here
	enum ExceptionCause {
		X_INT, 	// Interrupt
		X_MOD, 	// TLB Modification
		X_TLBL, // TLB Load error
		X_TLBS, // TLB Store error
		X_ADEL, // Address error (load or fetch)
		X_ADES, // Address error (store)
		X_IBE, 	// Ins bus error
		X_DBE, 	// Data bus error (load/store)
		X_SYS, 	// Syscall
		X_BP, 	// Break point
		X_RI, 	// Reserved
		X_CPU,	 // Coproc unusable
		X_OV, 	// Overflow
		X_TR, 	// Trap
		X_reserved, // Reserved
		X_FPE, 	// Floating point
	};

	// member variables (internal registers)

	uint32_t r_gpr[32]; // General Registers
	uint32_t r_cr[32]; 	// Coprocessor Registers
	uint32_t r_ctl[6]; 	// Control registers
	uint32_t r_pc; 		// rogram Counter
	uint32_t r_npc; 	// Next Program Counter

	enum DataAccessType r_mem_type; // Data Cache access type
	uint32_t r_mem_addr; 			// Data Cache address
	uint32_t r_mem_wdata; 			// Data Cache data value (write)
	uint32_t r_mem_dest; 			// Data Cache destination register (read)
	bool r_dbe; 					// Asynchronous Data Bus Error (write)
	bool r_mem_req;					// memory request ?
	bool r_mem_unsigned;			// unsigned or signed memory request

	uint32_t r_count;

	typedef union {
		struct {
			union {
				PACKED_BITFIELD(
						uint32_t immed26:26,
						uint32_t op:6,
				) j;
				PACKED_BITFIELD(
						uint32_t a:5,
						uint32_t b:5,
						uint32_t imm16:16,
						uint32_t op:6,
				) i;
				PACKED_BITFIELD(
						uint32_t a:5,
						uint32_t b:5,
						uint32_t c:5,
						uint32_t opx:6,
						uint32_t sh:5,
						uint32_t op:6,
				) r;
			}__attribute__((packed));
		}__attribute__((packed));
		uint32_t ins;
	} ins_t;

	uint32_t m_ins_delay;		// Instruction latency simulation

	// member variables used for communication between
	// member functions (they are not registers)

	ins_t m_instruction;
	uint32_t m_exceptionSignal;
	uint32_t m_gprA;
	uint32_t m_gprB;
	uint32_t m_branchAddress;
	uint32_t m_next_pc;
	bool m_branchTaken;
	bool m_hazard;
	uint32_t m_exec_cycles;	// number of executed instructions

	uint32_t m_rdata;
	uint32_t m_irq;
	bool m_ibe;
	bool m_dbe;

	lateResultInstruction m_listOfLateResultInstruction;

	// control signal and data used by custom instruction
	union {
		int32_t i;
		float f;
	} m_resultCustomInstruction, m_operandA, m_operandB;
	bool m_readra, m_readrb, m_writerc;

public:
	Nios2fIss(uint32_t ident);

	void step();
	void reset();
	void dumpInstruction() const;
	void dumpRegisters() const;

	inline void nullStep(uint32_t time_passed = 1) {
		if (m_ins_delay) {
			if (m_ins_delay > time_passed)
				m_ins_delay -= time_passed;
			else
				m_ins_delay = 0;
		}
		++r_count;

#if NIOS2_DEBUG
		std::cout << " r_count in null step : "<<r_count << "  m_hazard: " << m_hazard << std::endl;
#endif   	  
		if (m_startOflriList != NULL) {
			m_listOfLateResultInstruction.update();
#if NIOS2_DEBUG
			m_listOfLateResultInstruction.print();
#endif
		}
		if (m_startOflriList == NULL)
			m_hazard = false;
	}

	inline uint32_t isBusy() {
		return m_ins_delay;
	}

	inline void getInstructionRequest(bool &req, uint32_t &address) const {
		req = true;
		address = r_pc;
	}

	inline void getDataRequest(bool &valid, 
			enum DataAccessType &type,
			uint32_t &address, 
			uint32_t &wdata) const {
		valid = r_mem_req;
		address = r_mem_addr;
		wdata = r_mem_wdata;
		type = r_mem_type;
	}

	inline void setWriteBerr() {
		r_dbe = true;
	}

	inline void setIrq(uint32_t irq) {
		m_irq = irq;
	}

	inline void setInstruction(bool error, uint32_t val) {
		m_ibe = error;
		m_instruction.ins = val;
	}

	void setDataResponse(bool error, uint32_t rdata);


	// processor internal registers access API, used by debugger
	inline unsigned int getDebugRegisterCount() const {
		return 32;
	}

	uint32_t getDebugRegisterValue(unsigned int reg) const;

	inline size_t getDebugRegisterSize(unsigned int reg) const {
		return 32;
	}

	void setDebugRegisterValue(unsigned int reg, uint32_t value);

	inline uint32_t getDebugPC() const {
		return r_pc;
	}

	inline void setDebugPC(uint32_t pc) {
		r_pc = pc;
		r_npc = pc+4;
	}

private:
	void run();

	inline void setInsDelay(uint32_t delay) {
		assert(delay > 0);
		m_ins_delay = delay-1;
	}

	typedef void (Nios2fIss::*func_t)();

	static func_t const opcodeTable[64];
	static func_t const RTypeTable[64];
	static func_t const customInstTable[256];

	ofstream memTrace;

	void op_addi();
	void op_andhi();
	void op_andi();
	void op_beq();
	void op_bge();
	void op_bgeu();
	void op_blt();
	void op_bltu();
	void op_bne();
	void op_br();
	void op_call();
	void op_cmpeqi();
	void op_cmpgei();
	void op_cmpgeui();
	void op_cmplti();
	void op_cmpltui();
	void op_cmpnei();
	void op_custom();
	void op_flushd();
	void op_flushda();
	void op_initd();
	void op_ldb();
	void op_ldbio();
	void op_ldbu();
	void op_ldbuio();
	void op_ldh();
	void op_ldhio();
	void op_ldhu();
	void op_ldhuio();
	void op_ldw();
	void op_ldwio();
	void op_muli();
	void op_ori();
	void op_orhi();
	void op_stb();
	void op_stbio();
	void op_sth();
	void op_sthio();
	void op_stw();
	void op_stwio();
	void op_xorhi();
	void op_xori();
	void op_RType();
	void op_illegal();

	void RType_add();
	void RType_and();
	void RType_break();
	void RType_bret();
	void RType_callr();
	void RType_cmpeq();
	void RType_cmpge();
	void RType_cmpgeu();
	void RType_cmplt();
	void RType_cmpltu();
	void RType_cmpne();
	void RType_eret();
	void RType_div();
	void RType_divu();
	void RType_flushi();
	void RType_flushp();
	void RType_initi();
	void RType_jmp();
	void RType_mul();
	void RType_mulxss();
	void RType_mulxsu();
	void RType_mulxuu();
	void RType_nextpc();
	void RType_nor();
	void RType_or();
	void RType_rdctl();
	void RType_ret();
	void RType_rol();
	void RType_roli();
	void RType_ror();
	void RType_sll();
	void RType_slli();
	void RType_sra();
	void RType_srai();
	void RType_srl();
	void RType_srli();
	void RType_sub();
	void RType_sync();
	void RType_trap();
	void RType_wrctl();
	void RType_xor();
	void RType_illegal();

	void custom_ill();
	void custom_addsf3();
	void custom_subsf3();
	void custom_mulsf3();
	void custom_divsf3();
	void custom_minsf3();
	void custom_maxsf3();
	void custom_negsf2();
	void custom_abssf2();
	void custom_sqrtsf2();
	void custom_cossf2();
	void custom_sinsf2();
	void custom_tansf2();
	void custom_atansf2();
	void custom_expsf2();
	void custom_logsf2();
	void custom_fcmplts();
	void custom_fcmples();
	void custom_fcmpgts();
	void custom_fcmpges();
	void custom_fcmpeqs();
	void custom_fcmpnes();
	void custom_floatsisf2();
	void custom_floatunsisf2();
	void custom_fixsfsi2();
	void custom_fixunsfsi2();

	static const char *opNameTable[64];
	static const char *opxNameTable[];
};

}}

#endif // _SOCLIB_NIOS2_ISS_H_
