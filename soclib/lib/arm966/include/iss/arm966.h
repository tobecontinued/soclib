/*
 *  Copyright (c) 2008,
 *  Commissariat a l'Energie Atomique (CEA)
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   - Neither the name of CEA nor the names of its contributors may be used to
 *     endorse or promote products derived from this software without specific 
 *     prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY 
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Daniel Gracia Perez (daniel.gracia-perez@cea.fr)
 */


#ifndef _SOCLIB_ARM966_ISS_H_
#define _SOCLIB_ARM966_ISS_H_

#include "iss.h" 
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

