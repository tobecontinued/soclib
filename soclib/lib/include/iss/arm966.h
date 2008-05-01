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
 * - 2007-06-15
 *   Nicolas Pouillon, Alain Greiner: Model created
 */

/*
 * Functionnal description:
 * The tree files mips_iss.h, mips_iss.cpp, & mips_instructions.cpp
 * define an Instruction Set Simulator for the MIPS R3000 processor.
 * The same ISS has been wrapped to build CABA, TLMT, and PV simulation
 * models,  using appropriate wrappers.
 *
 * - One instruction is executed in one "step".
 *   (In case of a timed model one step corresponds to one cycle).
 * - The MIPS R3000 delayed branchs are supported: The ISS internal
 *   state contains two program counters (r_pc & r_npc), in order
 *   to describe the delayed branch behaviour.
 * - The MIPS R3000 delayed load behaviour : in case of RaW dependency
 *   between two successive instructions, the processor is dtalled
 *   during one step.
 * - The MIPS R3000 delayed multiply and divide behaviour is NOT
 *   supported : the mul and div instructions are supposed to be
 *   executed in one step.
 */

#ifndef _SOCLIB_ARM966_ISS_H_
#define _SOCLIB_ARM966_ISS_H_

#include "unisim/component/cxx/processor/arm/cpu.hh"
#include "unisim/component/cxx/processor/arm/memory_op.hh"
#include "unisim/component/cxx/processor/arm/config.hh"
#include "unisim/component/cxx/processor/arm/exception.hh"

namespace soclib {
namespace common {

class ARM966Iss :
	public unisim::component::cxx::processor::arm::CPU<
		unisim::component::cxx::processor::arm::ARM966E_S_Config> ,
	public unisim::component::cxx::processor::arm::CacheInterface< 
		unisim::component::cxx::processor::arm::ARM966E_S_Config::address_t>,
	public soclib::common::Iss { 

private:
	typedef uint32_t address_t;

public:
	static const int n_irq = 2;
	
	ARM966Iss(uint32_t ident);
	~ARM966Iss();


	/************************************************************************/
	/* Methods required by the ISS Wrapper                            START */
	/************************************************************************/

	// ISS <-> Wrapper API
 	
    void reset();
 	
    uint32_t isBusy();
    void step();
    void nullStep( uint32_t time_passed = 1 );

	void getInstructionRequest(bool &req, uint32_t &addr) const;
    void setInstruction(bool error, uint32_t val);
 	
    void getDataRequest(bool &req, enum DataAccessType &type,
             uint32_t &addr, uint32_t &data) const;
    void setDataResponse(bool error, uint32_t rdata);
    void setWriteBerr();
 	
    void setIrq(uint32_t irq);
	
    // processor internal registers access API, used by
    // debugger. Register numbering must match gdb packet order.

    unsigned int getDebugRegisterCount() const;
    uint32_t getDebugRegisterValue(unsigned int reg) const;
    void setDebugRegisterValue(unsigned int reg, uint32_t value);
    size_t getDebugRegisterSize(unsigned int reg) const;
	
    uint32_t getDebugPC() const;
    void setDebugPC(uint32_t);

	void setICacheInfo( size_t line_size, size_t assoc, size_t n_lines );
    void setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines );

	/************************************************************************/
	/* Methods required by the ISS Wrapper                              END */
	/************************************************************************/

	/************************************************************************/
	/* Methods required by the cache interface                        START */
	/************************************************************************/
	
	// invalidate without checking if modified data
	void PrInvalidateBlock(uint32_t set, uint32_t way);

	// write modified data into memory then invalidate all copies in caches
	void PrFlushBlock(uint32_t set, uint32_t way);

	// write modified data into memory, do not invalidate block
	void PrCleanBlock(uint32_t set, uint32_t way);

	void PrReset();
	// invalidate without checking if modified data
	void PrInvalidate();
	void PrInvalidateSet(uint32_t set);
	void PrInvalidateBlock(address_t addr);

	// write modified data into memory then invalidate all copies in caches
	void PrFlushBlock(address_t addr);

	// write modified data into memory, do not invalidate block
	void PrCleanBlock(address_t addr);           //add for Arm Cache

	void PrZeroBlock(address_t addr);

	void PrWrite(address_t addr, const uint8_t *buffer, uint32_t size);
	void PrRead(address_t addr, uint8_t *buffer, uint32_t size);

	/************************************************************************/
	/* Methods required by the cache interface                          END */
	/************************************************************************/
	
};

} // end of namespace common
} // end of namespace soclib

#endif // _SOCLIB_ARM966_ISS_H_

