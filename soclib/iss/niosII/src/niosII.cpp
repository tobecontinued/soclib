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

#include "niosII.h"
#include "base_module.h"
#include "arithmetics.h"
#include <iostream>
#include <iomanip>

lateResultInstruction *m_startOflriList;

// used to manage a list of late result instructions
// when necessary a new entry is added to the tail of the list
void lateResultInstruction::add(uint32_t reg)
{
	lateResultInstruction * ptr;
	lateResultInstruction * newElt = new lateResultInstruction;
	newElt->reg = reg;
	newElt->cycle= 3;
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
void lateResultInstruction::update()
{
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

void lateResultInstruction::print()
{
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

namespace {

template<typename data_t>
data_t be_to_mask( data_t be )
{
    size_t i;
    data_t ret = 0;
    data_t be_up = (1<<(sizeof(data_t)-1));

    for (i=0; i<sizeof(data_t); ++i) {
        ret <<= 8;
        if ( be_up & be )
            ret |= 0xff;
        be <<= 1;
    }
    return ret;
}


static inline uint32_t align(uint32_t data, int shift, int width)
{
	uint32_t mask = (1<<width)-1;
	uint32_t ret = data >>= shift*width;
	return ret & mask;
}


static inline std::string mkname(uint32_t no)
{
	char tmp[32];
	snprintf(tmp, 32, "Nios2_iss%d", (int)no);
	return std::string(tmp);
}

}


Nios2fIss::Nios2fIss(const std::string &name, uint32_t cpuid) :
    Iss2(name, cpuid)
#ifdef SOCLIB_MODULE_DEBUG
    , m_log(mkname(cpuid).c_str())
#endif
{
//	reset();
}


Nios2fIss::~Nios2fIss()
{
#ifdef SOCLIB_MODULE_DEBUG
	m_log.close();
#endif
}


void Nios2fIss::reset()
{

	struct DataRequest null_dreq = ISS_DREQ_INITIALIZER;

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

	r_ctl[status] = 0; // status control register is cleared
	r_ctl[ienable] = 0; // ienable control register is cleared

	r_pc = RESET_ADDRESS;
	r_npc = RESET_ADDRESS + 4;

    r_count = 0;

    r_dbe = false;
	m_ibe = false;
	m_dbe = false;

	r_mem_dest = 0;

	m_dreq = null_dreq;

	m_ins_delay = 0;
	m_exec_cycles = 0;

	m_hazard = false;
	m_exceptionSignal = NO_EXCEPTION;

	m_startOflriList = NULL;
}

uint32_t Nios2fIss::executeNCycles(uint32_t ncycle,
                                   const struct InstructionResponse &irsp,
                                   const struct DataResponse &drsp, uint32_t irq_bit_field)
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " executeNCycles( " << ncycle << ", "<< irsp << ", " << drsp << ", " << irq_bit_field << ")" << std::endl;
#endif

    // default values
	m_exceptionSignal = NO_EXCEPTION;

	if (drsp.valid)
		setDataResponse(drsp);

	m_ireq_ok = irsp.valid;

	if ( !irsp.valid || (m_dreq.valid && !drsp.valid ) || m_ins_delay) {
		uint32_t t = ncycle;

#ifdef SOCLIB_MODULE_DEBUG
	std::cout << name() << " Frozen " << m_ireq_ok << " " << m_dreq_ok<< " " << m_ins_delay << std::endl;
#endif
        if (m_ins_delay) {
            if (m_ins_delay < ncycle)
                t = m_ins_delay;
            m_ins_delay -= t;
        }
		r_count += t;
        return t;
	}

	m_ibe = irsp.error;
	m_irq = irq_bit_field;
	m_instruction.ins = irsp.instruction;


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
//	if (r_dbe) {
//		m_exceptionSignal = X_DBE;
//		r_dbe = false;
//#if SOCLIB_MODULE_DEBUG
//		std::cout
//            << " Asynchronous data bus error detection (write) : X_DBE set - addr :  "<< std::hex
//            << std::showbase << m_dreq.addr << std::dec << std::endl;
//#endif
//	}

	if (m_startOflriList != NULL) {
		lateResultInstruction * ptr = m_startOflriList;
		while (ptr != NULL) {
			if ( (ptr->reg == m_instruction.r.a) || (ptr->reg == m_instruction.r.b) ) {
				m_hazard = true;
			}
			ptr = ptr->next;
		}
	}

	if (m_startOflriList != NULL) {

		m_listOfLateResultInstruction.update();
		if (m_startOflriList == NULL) m_hazard = false;
#ifdef SOCLIB_MODULE_DEBUG
		m_listOfLateResultInstruction.print();
#endif
	}
	// ipending register indicates which interrupts are pending
	// a value of 1 in bit n means that the corresponding irqn input is asserted and
	// that the corresponding interrupt is enable in the ienable register
	r_ctl[ipending] = r_ctl[ienable] & m_irq;
#ifdef SOCLIB_MODULE_DEBUG
	if (m_irq)
		std::cout << "m_irq" << m_irq << std::endl;

	if ( r_ctl[ipending])
		std::cout << "r_ctl[ipending]" << r_ctl[ipending] << std::endl;
#endif

#ifdef SOCLIB_MODULE_DEBUG
	std::cout
        << " m_irq: "<<m_irq<< " ctl0: "<<r_ctl[status] <<
        " ctl3: " <<r_ctl[ienable] <<
        " ctl4: "<<r_ctl[ipending] <<
        " m_exceptionSignal:   " <<
        m_exceptionSignal << std::endl;
#endif

#ifdef SOCLIB_MODULE_DEBUG
	m_log << std::hex << r_pc << std::endl;
#endif


#ifdef SOCLIB_MODULE_DEBUG
	dumpInstruction();
	dumpRegisters();
#endif
	// Execute instruction if no data dependency & no bus error
	//
	// The run() function can modify the following registers:
	// r_gpr[i], r_mem_type, r_mem_addr; r_mem_wdata, r_mem_dest
	// as well as the m_exception, m_branchAddress & m_branchTaken variables
	if (m_hazard) {
#ifdef SOCLIB_MODULE_DEBUG
		std::cout << "  m_hazard: " << m_hazard << std::endl;
#endif
		r_count += ncycle;

		if (m_startOflriList == NULL)
			m_hazard = false;
		goto house_keeping;
	} else {
		run();
		m_exec_cycles++;
#ifdef SOCLIB_MODULE_DEBUG
		std::cout << " m_exec_cycles: "<<m_exec_cycles << std::endl;
#endif
	}

	if (m_exceptionSignal != NO_EXCEPTION)
		goto handle_exception;

	// Interrupt detection
	// 	if ( (r_ctl[status] & 1) && (r_ctl[ienable] & 0x1 ==  m_irq & 0x1) ) {
	if ((r_ctl[status] & 1) && (r_ctl[ipending] != 0)) {
		m_exceptionSignal = X_INT;

#ifdef SOCLIB_MODULE_DEBUG
        std::cout << name() << " Taking irqs " << irq_bit_field << std::endl;
#endif
        goto handle_exception;
	}
	goto no_exception;

 handle_exception: {

	    ExceptionClass ex_class = EXCL_FAULT;
	    ExceptionCause ex_cause = EXCA_OTHER;

	    switch (m_exceptionSignal) {
	    case X_INT:
        case X_RESET:
	        ex_class = EXCL_IRQ;
	        break;
	    case X_SYS:
	        ex_class = EXCL_SYSCALL;
	        break;
	    case X_BP:
	    case X_TR:
	        ex_class = EXCL_TRAP;
	        break;

	    case X_MOD:
	    case X_reserved:
	        abort();

	    case X_TLBL:
	    case X_TLBS:
	        ex_cause = EXCA_PAGEFAULT;
	        break;
	    case X_ADEL:
	    case X_ADES:
        case X_MALLDATAADR:
        case X_MALLDESTADR:
	        ex_cause = EXCA_ALIGN;
	        break;
	    case X_IBE:
	    case X_DBE:
	        ex_cause = EXCA_BADADDR;
	        break;
	    case X_RI:
	    case X_CPU:
	    case X_ILLEGAL:
        case X_UIINST:
	        ex_cause = EXCA_ILL;
	        break;
	    case X_OV:
	    case X_FPE:
	        ex_cause = EXCA_FPU;
	        break;
        case X_DIV:
            ex_cause = EXCA_DIVBYZERO;
            break;

        default:
            assert(!"This must not happen");	    }

	    if ( debugExceptionBypassed( ex_class, ex_cause  ) )
            goto no_exception;

#ifdef SOCLIB_MODULE_DEBUG
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

		r_ctl[exception] = m_exceptionSignal;

		r_gpr[EA] = r_npc-4; // ea register stores the address where processing can resume

		r_pc = EXCEPTION_HANDLER_ADDRESS;
		r_npc = EXCEPTION_HANDLER_ADDRESS + 4;

	}
	goto house_keeping;

 no_exception: {
	 if (m_branchTaken) {
		r_pc = m_branchAddress;
		r_npc = m_branchAddress + 4;
	} else {
		r_pc = r_npc;
		r_npc = r_npc + 4;
	}
	r_count += ncycle;
 }

 house_keeping:
//	 r_gpr[0] = 0;
	 return ncycle;
}

void Nios2fIss::setDataResponse(const struct DataResponse &drsp)
{
	if ( !m_dreq.valid)
		return;

	m_dreq.valid = false;
	m_dreq_ok = drsp.valid;
	m_dbe = drsp.error;

	int nb = 0;

	switch (m_dreq.be) {
	case 1:
	case 2:
	case 4:
	case 8:
		nb = 1;
		break;
	case 3:
	case 0xc:
		nb = 2;
		break;
	case 0xf:
		nb = 4;
		break;
	}

	// detection of a possible data dependency for late result instruction
	// late result instructions have a two cycle bubble placed between them and instructions
	// that use their results, this is managed through m_listOfLateResultInstruction
	if (m_startOflriList != NULL) {
		lateResultInstruction * ptr = m_startOflriList;
		//m_hazard = false;
		while (ptr != NULL) {
			if ( (ptr->reg == m_instruction.r.a) || (ptr->reg == m_instruction.r.b)) {
				m_hazard = true;
			}
			ptr = ptr->next;
		}
	}

	// when destination register is zero, this is a load or a store instruction
	if (r_mem_dest == 0)
		return;

	data_t data = drsp.rdata;
	int byte_count = r_mem_byte_count;

	data >>= 8*r_mem_byte_le;

	//   if ( isReadAccess(m_dreq.type) ) {
#ifdef SOCLIB_MODULE_DEBUG
	std::cout
        << m_name
        << " read to " << r_mem_dest
        //            << "(" << m_dreq.type << ")"
        << " from " << std::hex << m_dreq.addr
        << ": " << data
        << " hazard: " << m_hazard
        << std::endl;
#endif
	//    }

    switch (r_mem_do_sign_extend) {
    case 2:
        data = soclib::common::sign_ext(data, 16);
        byte_count = 4;
        break;
    case 1:
        data = soclib::common::sign_ext(data, 8);
        byte_count = 4;
        break;
    case 8:
        data = !data;
        break;
    case -2:
        data = data & 0xffff;
        byte_count = 4;
        break;
    case -1:
        data = data & 0xff;
        byte_count = 4;
        break;
    }

    data_t mask = be_to_mask<data_t>(((1<<byte_count)-1) & 0xf);
    data <<= 8*r_mem_offset_byte_in_reg;
    mask <<= 8*r_mem_offset_byte_in_reg;

    data_t new_data = (data&mask) | (r_gpr[r_mem_dest]&~mask);
#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << name()
        << " setData: " << drsp
        << " off: " << r_mem_offset_byte_in_reg
        << " count: " << r_mem_byte_count
        << " le: " << r_mem_byte_le
        << " old: " << r_gpr[r_mem_dest]
        << " from_mem: " << drsp.rdata
        << " mask: " << mask
        << " new_data: " << new_data
        << std::endl;
#endif
    r_gpr[r_mem_dest] = new_data;

}



uint32_t Nios2fIss::debugGetRegisterValue(unsigned int reg) const
{
	switch (reg) {
	case 0:
		return 0;
	case 1 ... 31:
		return r_gpr[reg];
	case 32:
		return r_pc;
	case 33:
		return r_ctl[0];
	case 34:
		return r_ctl[1];
	case 35:
		return r_ctl[2];
	case 36:
		return r_ctl[3];
	case 37:
		return r_ctl[4];
	case 38:
		return r_ctl[5];
	case 39:
		return r_ctl[6];
	case 40:
		return r_ctl[7];
	case 41:
		return r_ctl[8];
	case 42:
		return r_ctl[9];
	case 43:
		return r_ctl[10];
	case 44:
		return r_ctl[11];
	case 45:
		return r_ctl[12];
	case 46:
		return r_ctl[13];
	case 47:
		return r_ctl[14];
	case 48:
		return r_ctl[15];
	default:
		return 0;
	}
}

void Nios2fIss::debugSetRegisterValue(unsigned int reg, uint32_t value)
{
	switch (reg) {
	case 1 ... 31:
        r_gpr[reg] = value;
        break;
    case 32:
        r_pc = value;
        r_npc = value+4;
        break;
    default:
        break;
	}
}

uint32_t Nios2fIss::controlRegisterGet( uint32_t reg ) const
{
    switch(reg) {
    case cpuid:
        return m_ident;
    case count:
        return r_count;
    default:
        return 0;
    }
}

void Nios2fIss::controlRegisterSet( uint32_t reg, uint32_t val )
{
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
