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
 * - summer 2006: First version developed on a first SoCLib template by Reeb, Charot.
 * - september 2007: the model has been completely rewritten and adapted to the SocLib 
 * 						rules defined during the first months of the SocLib ANR project
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

#include "nios2_fast.h"
#include "arithmetics.h"

lateResultInstruction *m_startOflriList;

// used to manage a list of late result instructions
// when necessary a new entry is added to the tail of the list
void lateResultInstruction::add(uint32_t reg) {
	lateResultInstruction * ptr;
	lateResultInstruction * newElt = new lateResultInstruction;
	newElt->reg = reg;
	newElt->cycle= 2;
	newElt->next = NULL;
	if (m_startOflriList == NULL) {
		m_startOflriList = newElt;
	} else {
		ptr = m_startOflriList;
		while (ptr->next != NULL)
			ptr = ptr->next;
		ptr->next = newElt;
	}
}

// used to manage a list of late result instructions
// 
void lateResultInstruction::update() {
	lateResultInstruction * ptr = m_startOflriList;
	lateResultInstruction * eltToBeRemoved = m_startOflriList;
	while (ptr != NULL) {
		ptr->cycle = ptr->cycle - 1;
		ptr = ptr->next;
	}
	if (eltToBeRemoved->cycle == 0) {
		m_startOflriList = eltToBeRemoved->next;
		delete eltToBeRemoved;
	}

}

void lateResultInstruction::print() {
	lateResultInstruction * ptr = m_startOflriList;
	int count = 0;
	while (ptr != NULL) {
		count++;
		std::cout << "ptr: " << ptr << " register: " << ptr->reg << " cycle: "
		<< ptr->cycle << std::endl;
		ptr = ptr->next;
	}
	std::cout << count << " instruction in list " << std::endl;
}

namespace soclib {
namespace common {

static const uint32_t NO_EXCEPTION = (uint32_t)-1;

namespace {

static inline uint32_t align(uint32_t data, int shift, int width) {
	uint32_t mask = (1<<width)-1;
	uint32_t ret = data >>= shift*width;
	return ret & mask;
}

static inline std::string mkname(uint32_t no) {
	char tmp[32];
	snprintf(tmp, 32, "Nios2_iss%d", (int)no);
	return std::string(tmp);
}

}

Nios2fIss:: Nios2fIss(uint32_t ident) :
	Iss(mkname(ident), ident) {
}

void Nios2fIss::reset() {
	// reset state is normally undefined for gpp and control registers
	// we however reset them here
	// 
	for (size_t i=0; i<32; ++i) {
		r_gpr[i] = 0;
		r_cr[i] = 0;
	}
	for (size_t i=1; i<6; ++i)
		r_ctl[i] = 0;
	r_ctl[5] = m_ident;

	r_ctl[status] = 0; 			// status control register is cleared
	r_ctl[ienable] = 0xffff;	// ienablecontrol register is set

	r_pc = RESET_ADDRESS;
	r_npc = RESET_ADDRESS + 4;

	r_dbe = false;
	m_ibe = false;
	m_dbe = false;
	r_mem_req = false;
	m_ins_delay = 0;
	m_exec_cycles = 0;

#if INSTRUCTIONMEMORYTRACE
	std::string str = "memTrace";
	std::ostringstream oss;
	oss << m_ident;
	std::string index = oss.str();
	str = str + index;
	size_t size = str.size() + 1;
	char * traceFile = new char[size];
	strncpy(traceFile, str.c_str(), size);
	memTrace.open(traceFile);
	memTrace << "Trace of the executed instruction addresses.\n";
#endif

	m_startOflriList = NULL;

}

void Nios2fIss::setDataResponse(bool error, uint32_t data) {

	m_dbe = error;
	r_mem_req = false;
	if (error) {
		return;
	}

	// detection of a possible data dependency for late result instruction	
	// late result instructions have a two cycle bubble placed between them and instructions
	// that use their results, this is managed through m_listOfLateResultInstruction
	if (m_startOflriList != NULL) {
		lateResultInstruction * ptr = m_startOflriList;
		//m_hazard = false;
		while (ptr != NULL) {
			if ( (ptr->reg == m_instruction.r.a) || (ptr->reg
					== m_instruction.r.b)) {
				m_hazard = true;
			}
			ptr = ptr->next;
		}
	}

	if ( isReadAccess(r_mem_type) ) {
#if NIOS2_DEBUG
		std::cout
		<< m_name
		<< " read to " << r_mem_dest
		<< "(" << r_mem_type << ")"
		<< " from " << std::hex << r_mem_addr
		<< ": " << data
		<< " hazard: " << m_hazard
		<< std::endl;
#endif
	}

	// We write the r_gpr[i]
	switch (r_mem_type) {
	default:
		break;
	case READ_WORD:
		r_gpr[r_mem_dest] = data;
		break;
	case READ_BYTE:
		r_gpr[r_mem_dest] = r_mem_unsigned ? (data & 0xff) : sign_ext(data, 8);
		break;
	case READ_HALF:
		r_gpr[r_mem_dest] = r_mem_unsigned ? (data & 0xffff) : sign_ext(data, 16);
		break;
	}
}

void Nios2fIss::step() {
	++r_count;
#if NIOS2_DEBUG
	std::cout << " r_count: "<<r_count << std::endl;
#endif		
	// The current instruction is executed in case of interrupt, but
	// the next instruction will be delayed.
	// The current instruction is not executed in case of exception,
	// and there is three types of bus error events,
	// 1 - instruction bus errors are synchronous events, signaled in
	// the m_ibe variable
	// 2 - read data bus errors are synchronous events, signaled in
	// the m_dbe variable
	// 3 - write data bus errors are asynchonous events signaled in
	// the r_dbe flip-flop
	// Instuction bus errors are related to the current instruction:
	// lowest priority.
	// Read Data bus errors are related to the previous instruction:
	// intermediatz priority
	// Write Data bus error are related to an older instruction:
	// highest priority

	// default values
	m_exceptionSignal = NO_EXCEPTION;

	// checking for various exceptions
	// Instruction bus error detection.
	if (m_ibe) {
		m_exceptionSignal = X_IBE;
		goto handle_exception;
	}

	// Synchronous Data bus error detection.
	if (m_dbe) {
		m_exceptionSignal = X_DBE;
		goto handle_exception;
	}

	// Asynchronous Data bus error detection
	if (r_dbe) {
		m_exceptionSignal = X_DBE;
		r_dbe = false;
#if NIOS2_DEBUG
		std::cout
		<< " Asynchronous data bus error detection (write) : X_DBE set - addr :  "<< std::hex
		<< std::showbase << r_mem_addr << std::dec << std::endl;
#endif
	}

	if (m_startOflriList != NULL) {
		lateResultInstruction * ptr = m_startOflriList;
		while (ptr != NULL) {
			if ( (ptr->reg == m_instruction.r.a) || (ptr->reg
					== m_instruction.r.b)) {
				m_hazard = true;
			}
			ptr = ptr->next;
		}
	}

	if (m_startOflriList != NULL) {

		m_listOfLateResultInstruction.update();
#if NIOS2_DEBUG
		m_listOfLateResultInstruction.print();
#endif
	}

	// ipending register indicates which interrupts are pending
	// a value of 1 in bit n means that the correspondinf irqn input is asserted and
	// that the corresponding interrupt is enable in the ienable register
	r_ctl[ipending] = r_ctl[ienable] & m_irq;

#if NIOS2_DEBUG
	std::cout << " m_irq: "<<m_irq<< " ct0: "
			<< r_ctl[status]
	        << " ctl3: " 
	        << r_ctl[ienable]<< " ctl4: "
	        << r_ctl[ipending]<< " m_exceptionSignal:   " 
	        << m_exceptionSignal << std::endl;
#endif

#if INSTRUCTIONMEMORYTRACE
	memTrace << hex << r_pc << endl;
#endif      

#if NIOS2_DEBUG
	dumpInstruction();
	dumpRegisters();
#endif
	// Execute instruction if no data dependency & no bus error
	//
	// The run() function can modify the following registers:
	// r_gpr[i], r_mem_type, r_mem_addr; r_mem_wdata, r_mem_dest
	// as well as the m_exception, m_branchAddress & m_branchTaken variables
	if (m_hazard) {
#if NIOS2_DEBUG
		std::cout <<  "  m_hazard: " << m_hazard << std::endl;
#endif
		if (m_startOflriList == NULL)
			m_hazard = false;
		goto house_keeping;
	} else {
		run();
		m_exec_cycles++;
#if NIOS2_DEBUG
		std::cout << " m_exec_cycles: "<<m_exec_cycles << std::endl;
#endif
	}

	// Interrupt detection
	// 	if ( (r_ctl[status] & 1) && (r_ctl[ienable] & 0x1 ==  m_irq & 0x1) ) {
	if (m_exceptionSignal != NO_EXCEPTION)
		goto handle_exception;

	if ((r_ctl[status] & 1) && (r_ctl[ipending] != 0)) {
		m_exceptionSignal = X_INT;
		goto handle_exception;
	}

	goto no_exception;

	handle_exception: {
#if NIOS2_DEBUG
		std::cout
		<< m_name
		<< std::hex << std::showbase
		<< " exception: "<<m_exceptionSignal<<std::endl
		<< " PC: " << r_pc
		<< " next_PC: " << r_npc
		<< std::dec
		<< std::endl<< std::endl;
#endif
		r_ctl[estatus] = r_ctl[status];/* status reg saving */
		r_ctl[status] = r_ctl[status] & 0xFFFFFFFE; /* bit PIE forced to zero */

		r_gpr[EA] = r_npc-4; // ea register storesb the address where processing can resume

		r_pc = EXCEPTION_HANDLER_ADDRESS;
		r_npc = EXCEPTION_HANDLER_ADDRESS + 4;
	}
	goto house_keeping;
	no_exception: 
	if (m_branchTaken) {
		r_pc = m_branchAddress;
		r_npc = m_branchAddress + 4;
	} else {
		r_pc = r_npc;
		r_npc = r_npc + 4;
	}

	house_keeping: r_gpr[0] = 0;
}

//int Nios2fIss::cpuCauseToSignal( uint32_t cause ) const
//{
//	switch (cause) {
//	case X_INT:
//		return 2; // Interrupt
//	case X_MOD:
//	case X_TLBL:
//	case X_TLBS:
//		return 5; // Trap (nothing better)
//	case X_ADEL:
//	case X_ADES:
//	case X_IBE:
//	case X_DBE:
//		return 11; // SEGV
//	case X_SYS:
//	case X_BP:
//	case X_TR:
//	case X_reserved:
//		return 5; // 5 Trap/breakpoint
//	case X_RI:
//	case X_CPU:
//		return 4; // Illegal instruction
//	case X_OV:
//	case X_FPE:
//		return 8; // Floating point exception
//	};
//	return 5;       // GDB SIGTRAP                                                                                                                                                                
//}


uint32_t Nios2fIss::getDebugRegisterValue(unsigned int reg) const {
	switch (reg) {
	case 0:
		return 0;
	case 1
	... 31:
	return soclib::endian::uint32_swap(r_gpr[reg]);
	case 32:
		return soclib::endian::uint32_swap(r_pc);	
	case 33:
		return soclib::endian::uint32_swap(r_ctl[0]);
	case 34:
		return soclib::endian::uint32_swap(r_ctl[1]);
	case 35:
		return soclib::endian::uint32_swap(r_ctl[2]);
	case 36:
		return soclib::endian::uint32_swap(r_ctl[3]);
	case 37:
		return soclib::endian::uint32_swap(r_ctl[4]);
	case 38:
		return soclib::endian::uint32_swap(r_ctl[5]);
	case 39:
		return soclib::endian::uint32_swap(r_ctl[6]);
	case 40:
		return soclib::endian::uint32_swap(r_ctl[7]);
	case 41:
		return soclib::endian::uint32_swap(r_ctl[8]);
	case 42:
		return soclib::endian::uint32_swap(r_ctl[9]);
	case 43:
		return soclib::endian::uint32_swap(r_ctl[10]);
	case 44:
		return soclib::endian::uint32_swap(r_ctl[11]);
	case 45:
		return soclib::endian::uint32_swap(r_ctl[12]);
	case 46:
		return soclib::endian::uint32_swap(r_ctl[13]);
	case 47:
		return soclib::endian::uint32_swap(r_ctl[14]);
	default:
		return 0;
	}
}

void Nios2fIss::setDebugRegisterValue(unsigned int reg, uint32_t value) {
	value = soclib::endian::uint32_swap(value);

	switch (reg) {
	case 1
	... 31:
	r_gpr[reg] = value;
	break;
	default:
		break;
	}
}

}
}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
