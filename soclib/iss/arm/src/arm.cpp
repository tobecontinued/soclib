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
#include "arithmetics.h"
#include <iostream>
#include <iomanip>
#include <cstring>

namespace soclib { namespace common {

const ArmIss::ArmMode ArmIss::psr_to_mode[32] = {
	MOD_Count, MOD_Count, MOD_Count, MOD_Count,
	MOD_Count, MOD_Count, MOD_Count, MOD_Count,
	MOD_Count, MOD_Count, MOD_Count, MOD_Count,
	MOD_Count, MOD_Count, MOD_Count, MOD_Count,

	MOD_USER32, MOD_FIQ32, MOD_IRQ32, MOD_SUPER32,
	MOD_Count,  MOD_Count, MOD_Count, MOD_ABORT32,
	MOD_Count,  MOD_Count, MOD_Count, MOD_UNDEF32,
	MOD_Count,  MOD_Count, MOD_Count, MOD_Count,
};

const ArmIss::ArmPsrMode ArmIss::mode_to_psr[MOD_Count] = { 
	MOD_PSR_USER32,
	MOD_PSR_FIQ32,
	MOD_PSR_IRQ32,
	MOD_PSR_SUPER32,
	MOD_PSR_ABORT32,
	MOD_PSR_UNDEF32,
};

const ArmIss::except_info_s ArmIss::except_info[EXCEPT_Count] = {
	/* EXCEPT_NONE */ {},
	/* EXCEPT_UNDEF*/ { false, MOD_PSR_UNDEF32, 0x04, 4 },
	/* EXCEPT_SWI  */ { false, MOD_PSR_SUPER32, 0x08, 4 },
	/* EXCEPT_FIQ  */ { true,  MOD_PSR_FIQ32,   0x1C, 4 },
	/* EXCEPT_IRQ  */ { false, MOD_PSR_IRQ32,   0x18, 4 },
	/* EXCEPT_PABT */ { false, MOD_PSR_ABORT32, 0x0C, 4 },
	/* EXCEPT_DABT */ { false, MOD_PSR_ABORT32, 0x10, 8 },
};

void ArmIss::cpsr_update(psr_t psr)
{
	ArmMode cur_mode = psr_to_mode[r_cpsr.mode];
	ArmMode new_mode = psr_to_mode[psr.mode];

#if defined(SOCLIB_MODULE_DEBUG)
	std::cout
		<< name() << " cpsr update "
		<< std::hex << r_cpsr.whole << " -> " << psr.whole
		<< ((new_mode != cur_mode) ? " new mode" : " no mode change")
		<< std::endl;
#endif

	if (new_mode == MOD_Count)
		return reset();

	if (new_mode != cur_mode) {
		// swap r13 and r14
		std::memcpy(r_r13_r14[cur_mode],  r_gp + 13,           sizeof(data_t) * 2);
		std::memcpy(r_gp + 13,            r_r13_r14[new_mode], sizeof(data_t) * 2);

		// swap r13 to r12 for FIQ mode
		if (cur_mode == MOD_FIQ32) {
			// From FIQ to other modes
			std::memcpy(r_r8_r12[1], r_gp + 8,    sizeof(data_t) * 5);
			std::memcpy(r_gp + 8,    r_r8_r12[0], sizeof(data_t) * 5);
		} else if (new_mode == MOD_FIQ32) {
			// From other modes to FIQ
			std::memcpy(r_r8_r12[0], r_gp + 8,    sizeof(data_t) * 5);
			std::memcpy(r_gp + 8,    r_r8_r12[1], sizeof(data_t) * 5);
		}

		r_spsr[new_mode] = r_cpsr;
	}
	r_cpsr = psr;
}

void ArmIss::getRequests(
	Iss2::InstructionRequest &ireq,
	Iss2::DataRequest &dreq
	) const
{
	ireq.valid = m_microcode_func == NULL;
	ireq.addr = m_current_pc;

	dreq = m_dreq;
}

uint32_t ArmIss::executeNCycles(
	uint32_t ncycle,
	const Iss2::InstructionResponse &irsp,
	const Iss2::DataResponse &drsp,
	uint32_t irq_state
	)
{
	m_irq_in = irq_state;
	bool r15_changed = false;

	bool accept_external_interrupts = !r_cpsr.irq_disabled && !m_microcode_func;
	bool instruction_asked = m_microcode_func == NULL;
	
	bool data_req_nok = m_dreq.valid;

	if ( instruction_asked && irsp.valid ) {
		m_ins_error |= irsp.error;
		m_opcode.ins = irsp.instruction;
		instruction_asked = false;
	}

	if ( data_req_nok && drsp.valid ) {
		m_data_error |= drsp.error;
		r15_changed = handle_data_response(drsp);
		data_req_nok = false;
	}

	// if r15 changed, the fetch error we will
	// eventually get from ifetch is broken.
	// else, the fetched instruction is not valid
	m_ins_error &= !r15_changed;
	
	if ( ncycle == 0 )
		return 0;

	if ( instruction_asked || data_req_nok ) {
		m_cycle_count += ncycle;
		return ncycle;
	}

	if ( m_ins_error || m_data_error ) {
		goto handle_except;
	}

	m_run_count += 1;
	if ( m_microcode_func ) {
		(this->*m_microcode_func)();
	} else {
		if ( r_gp[15] != m_current_pc ) {
			m_opcode.decod.cond = 0xf; // Never
		} else {
			r_gp[15] += 4;
		}

#ifdef SOCLIB_MODULE_DEBUG
		dump();
#endif
		run();
	}

	if ( irq_state ) {
		if ( accept_external_interrupts &&
			 !r_cpsr.irq_disabled &&
			 !m_microcode_func &&
			 !m_dreq.valid
			)
			m_exception = EXCEPT_IRQ;
#ifdef SOCLIB_MODULE_DEBUG
		else
			std::cout << name() << " ignoring IRQs" << std::endl;
#endif
	}

	if ( m_exception == EXCEPT_NONE )
		goto normal_end;

  handle_except:

	m_microcode_func = NULL;

	if ( m_ins_error ) {
		m_exception = EXCEPT_PABT;
		m_ins_error = false;
		m_exception_pc = m_current_pc;
	} else if ( m_data_error ) {
		m_exception = EXCEPT_DABT;
		m_data_error = false;
		m_exception_pc = m_current_pc;
		m_exception_dptr = m_dreq.addr;
	}

    if ( debugExceptionBypassed( m_exception ) )
        goto normal_end;

#if defined(SOCLIB_MODULE_DEBUG)
	std::cout << name() << " exception " << (int)m_exception << std::endl;
#endif

	assert(m_exception != EXCEPT_NONE);

	{
		const struct except_info_s & info = except_info[m_exception];

		psr_t new_psr = r_cpsr;
		new_psr.fiq_disabled |= info.disable_fiq;
		new_psr.irq_disabled = true;
		new_psr.mode = info.new_mode;

		cpsr_update(new_psr);

		r_gp[14] = r_gp[15] + info.return_offset;
		r_gp[15] = info.vector_address - 4;
	}

	m_exception = EXCEPT_NONE;

  normal_end:
	m_current_pc = m_microcode_func ? 0 : r_gp[15];
	m_cycle_count += 1;
	return 1;
}


void ArmIss::reset()
{
#ifdef SOCLIB_MODULE_DEBUG
	std::cout << name() << " reset" << std::endl;
#endif
	for ( size_t i=0; i<16; ++i )
		r_gp[i] = 0;
	for ( size_t i=0; i<3; ++i )
		m_tls_regs[i] = 0;

	r_cpsr.whole = 0x0;
	r_cpsr.fiq_disabled = true;
	r_cpsr.irq_disabled = true;
	r_cpsr.mode = mode_to_psr[MOD_SUPER32];
	m_current_pc = r_gp[15] = 0x0;
	m_ins_error = false;
	m_data_error = false;
	m_microcode_func = NULL;
	m_cycle_count = 0;
	m_run_count = 0;
	m_exception = EXCEPT_NONE;
	m_cache_info.whole = 0;
	m_cache_info.separated = 1;
	DataRequest dreq = ISS_DREQ_INITIALIZER;
	m_dreq = dreq;
	r_bus_mode = Iss2::MODE_KERNEL;
}

static const char * const cond_code[16] = {
	"EQ","NE","CS","CC","MI","PL","VS","VC",
	"HI","LS","GE","LT","GT","LE","AL","NV",
};

static const char * const flag_code[16] = {
	"    ","   V","  C ","  CV",
	" Z  "," Z V"," ZC "," ZCV",
	"-   ","-  V","- C ","- CV",
	"-Z  ","-Z V","-ZC ","-ZCV",
};

static const char *const mode_code[] = {
	"User", "FIQ", "IRQ", "Super", "Abort", "Undef"
};

void ArmIss::dump() const
{
	int8_t id = decod_main(m_opcode.ins);
	const char *ins_name = func_names[id];

    std::cout
        << m_name
		<< std::hex << std::noshowbase << std::setfill('0')
        << " PC: " << std::setw(8) << m_current_pc
        << " Ins: " << std::setw(8) << m_opcode.ins
		<< " [" << flag_code[r_cpsr.flags] << "]"
		<< " + " << cond_code[m_opcode.dp.cond]
		<< " = ";
	if ( ! cond_eval() )
		std::cout << " NO (";
	std::cout << ins_name;
	if ( ! cond_eval() )
		std::cout << ")";
	std::cout
		<< std::endl << std::dec
        << " Mode: "   << mode_code[psr_to_mode[r_cpsr.mode]]
        << " IRQ: "   << (r_cpsr.irq_disabled ? "disabled" : "enabled")
        << " FIQ: "   << (r_cpsr.fiq_disabled ? "disabled" : "enabled")
		<< std::endl << std::dec
        << " N rn: "   << m_opcode.dp.rn
        << "  rd: "    << m_opcode.dp.rd
        << "  rm: " << m_opcode.mul.rm
        << std::endl
        << " V rn: "   << r_gp[m_opcode.dp.rn]
        << "  rd: "    << r_gp[m_opcode.dp.rd]
        << "  rm: "    << r_gp[m_opcode.mul.rm]
        << "  shift: " << (m_opcode.ins & 0xfff)
        << std::endl;
    for ( size_t i=0; i<16; ++i ) {
        std::cout
			<< " " << std::dec << std::setw(2) << i << ": "
			<< std::hex << std::noshowbase << std::setw(8) << std::setfill('0')
			<< r_gp[i];
        if ( i%4 == 3 )
            std::cout << std::endl;
    }
}

ArmIss::ArmIss( const std::string &name, uint32_t cpuid )
	: Iss2(name, cpuid)
{
	reset();
}

int ArmIss::debugCpuCauseToSignal( uint32_t cause ) const
{
	switch (cause) {
	case EXCEPT_UNDEF:
	case EXCEPT_SWI:
		return 5; // Trap
	case EXCEPT_FIQ:
	case EXCEPT_IRQ:
		return 2; // Interrupt
	case EXCEPT_PABT:
	case EXCEPT_DABT:
		return 11; // SEGV
	}
	return 5;
}

}}
