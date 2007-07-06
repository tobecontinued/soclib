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
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 *
 * History:
 * - 2007-06-15
 *   Nicolas Pouillon, Alain Greiner: Model created
 */

#include "common/iss/mips.h"

namespace soclib { namespace common {

static const uint32_t NO_EXCEPTION = (uint32_t)-1;

namespace {

static inline uint32_t align( uint32_t data, int shift, int width )
{
	uint32_t mask = (1<<width)-1;
	uint32_t ret = data >>= shift*width;
	return ret & mask;
}

static inline std::string mkname(uint32_t no)
{
	char tmp[32];
	snprintf(tmp, 32, "mips_iss%d", (int)no);
	return std::string(tmp);
}

}

MipsIss::MipsIss(uint32_t ident)
	: Iss(mkname(ident), ident)
{
	r_cp0[IDENT] = 0x80000000|ident;
	r_cp0[INDEX] = ident;

	for (size_t i=0; i<32; ++i ) {
		SOCLIB_REG_RENAME_N_NAME(m_name.c_str(), r_gp, (int)i);
		SOCLIB_REG_RENAME_N_NAME(m_name.c_str(), r_cp0, (int)i);
	}
	SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_hi);
	SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_lo);
}

MipsIss::~MipsIss()
{
}

void MipsIss::reset()
{
	r_pc = RESET_ADDRESS;
	r_npc = RESET_ADDRESS + 4;
	r_mem_type = MEM_NONE;
	r_dbe = false;
	r_cp0[STATUS] = 0;
}

void MipsIss::step()
{
	// Copy instruction from ISS capsule to field-aware instruction
	// word
	m_ins.ins = m_instruction;

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
 	m_exception = NO_EXCEPTION;
 	bool hazard = false;
 
	// Interrupt detection
 	if ( (uint32_t)r_cp0[STATUS] & (uint32_t)((m_irq&0x3f)<<10) &&
		 (uint32_t)r_cp0[STATUS] & 1 )
		m_exception = X_INT;
 
 	if (m_ibe)
		m_exception = X_IBE;
 
	// Synchronous Data bus error detection.
	//
	// If the previous instruction was a load, and there is no bus
	// error, we complete the previous instruction.
	//
	// We write the r_gp[i], and we detect a possible data dependency,
	// in order to implement the delayed load behaviour.
 	if (m_dbe) {
		m_exception = X_DBE;
 		r_cp0[BAR] = r_mem_addr;
 	} else {
		if (r_mem_dest == m_ins.r.rs || r_mem_dest == m_ins.r.rt)
			hazard = true;
		switch (r_mem_type) {
		default:
			hazard = false;
			break;
		case MEM_LW:
			r_gp[r_mem_dest] = m_rdata;
 			break;
		case MEM_LB:
			r_gp[r_mem_dest] = (int32_t)(int8_t)align(m_rdata, r_mem_addr&0x3, 8);
			break;
		case MEM_LBU:
			r_gp[r_mem_dest] = align(m_rdata, r_mem_addr&0x3, 8);
			break;
		case MEM_LH:
			r_gp[r_mem_dest] = (int32_t)(int16_t)align(m_rdata, (r_mem_addr&0x2)/2, 16);
			break;
		case MEM_LHU:
			r_gp[r_mem_dest] = align(m_rdata, (r_mem_addr&0x2)/2, 16);
			break;
 		}
 	}
 
	// Asynchronous Data bus error detection
 	if (r_dbe) {
		m_exception = X_DBE;
 		r_dbe = false;
 		r_cp0[BAR] = r_mem_addr;
	}
 
	// Execute instruction if no data dependency & no bus error
	//
	// The run() function can modify the following registers:
	// r_gp[i], r_mem_type, r_mem_addr; r_mem_wdata, r_mem_dest, r_hi, r_lo
	// as well as the m_exception, m_branch_address & m_branch_taken variables
 	if ( (m_exception == NO_EXCEPTION || m_exception == X_INT) && !hazard )
		run();
 
	// Handling exceptions, interrupts and syscalls.
	// The following registers are modified : r_pc, r_npc, r_cp0[i]
  	switch (m_exception) {
	case NO_EXCEPTION:
 		if (m_branch_taken) {
 			r_pc = r_npc;
 			r_npc = m_branch_address;
 		} else {
 			r_pc = r_npc;
 			r_npc = r_npc + 4;
 		}
		break;
	case X_INT:
		// handling interrupts (the interrupted program must resume
		// using EPC) delayed in case of branch
 		if (m_branch_taken) {
 			r_pc = r_npc;
 			r_npc = m_branch_address;
			break;
 		} else
			goto take_except;
	default:
		goto take_except;
 	}

	goto normal_end;

 take_except:
#if MIPS_DEBUG
			std::cout
				<< m_name<<" exception: "<<m_exception<<std::endl
				<< " EPC: " << r_npc
				<< " CAUSE: " << ((m_exception << 2) | ((m_irq&0x3f)<<10))
				<< " STATUS: " << (((uint32_t)r_cp0[STATUS] & ~0x3f) |
								   (((uint32_t)r_cp0[STATUS] << 2) & 0x3c))
				<< std::endl;
#endif
	r_cp0[EPC] = r_npc;
	r_cp0[CAUSE] = (m_exception << 2) | ((m_irq&0x3f)<<10);
	r_cp0[STATUS] = (((uint32_t)r_cp0[STATUS] & ~0x3f) |
					 (((uint32_t)r_cp0[STATUS] << 2) & 0x3c));
	r_pc = EXCEPT_ADDRESS;
	r_npc = EXCEPT_ADDRESS + 4;

 normal_end:
 	// House keeping
 	r_gp[0] = 0;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
