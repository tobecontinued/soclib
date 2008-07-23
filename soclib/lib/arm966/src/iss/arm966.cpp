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

#include "arm966.h"
#include "unisim/component/cxx/processor/arm/cpu.tcc"
#include "unisim/component/cxx/processor/arm/exception.tcc"

namespace unisim {
namespace component {
namespace cxx {
namespace processor {
namespace arm {

template
class CPU<ARM966E_S_Config>;

}
}
}
}
}



namespace soclib { namespace common {

namespace {

static inline std::string mkname(uint32_t no)
{
    char tmp[32];
    snprintf(tmp, 32, "arm966es_iss%d", (int)no);
    return std::string(tmp);
}

}

typedef unisim::component::cxx::processor::arm::CPU<
		unisim::component::cxx::processor::arm::ARM966E_S_DebugConfig>
		inherited_CPU;

ARM966Iss::ARM966Iss(uint32_t ident) :
	inherited_CPU(this),
	Iss(mkname(ident), ident) {
	arm966es_initram = false;
	arm966es_vinithi = true;
}

ARM966Iss::~ARM966Iss() {}


/************************************************************************/
/* Methods required by the ISS Wrapper                            START */
/************************************************************************/

// ISS <-> Wrapper API
 	
void 
ARM966Iss::
reset() {
	// reinitialize variables to its initial state
	Reset();
}
 	
uint32_t 
ARM966Iss::
isBusy() {
	return IsBusy();
}

void 
ARM966Iss::
step() {
	Step();
}

void 
ARM966Iss::
nullStep( uint32_t time_passed) {
	NullStep(time_passed);
}

void 
ARM966Iss::
getInstructionRequest(bool &req, uint32_t &addr) const {
	GetInstructionRequest(req, addr);
}

void 
ARM966Iss::
setInstruction(bool error, uint32_t val) {
	SetInstruction(error, val);
}
 	
void 
ARM966Iss::
getDataRequest(bool &req, enum DataAccessType &type,
             uint32_t &addr, uint32_t &data) const {
	bool is_read;
	int size;
	GetDataRequest(req, is_read, size, addr, data);
	if(req) {
		if(is_read) {
			switch(size) {
			case 1:
				type = READ_BYTE;
				break;
			case 2:
				type = READ_HALF;
				break;
			case 4:
				type = READ_WORD;
				break;
			default:
				std::cerr << __FUNCTION__ << ":" << __FILE__ << ":" << __LINE__ << ": ERROR: invalid read size (" << size << ")" << std::endl;
				exit(-1);
				break;
			}
		} else {
			switch(size) {
			case 1:
				type = WRITE_BYTE;
				break;
			case 2:
				type = WRITE_HALF;
				break;
			case 4:
				type = WRITE_WORD;
				break;
			default:
				std::cerr << __FUNCTION__ << ":" << __FILE__ << ":" << __LINE__ << ": ERROR: invalid write size (" << size << ")" << std::endl;
				exit(-1);
				break;
			}
		}
	}
}

void 
ARM966Iss::
setDataResponse(bool error, uint32_t rdata) {
	SetDataResponse(error, rdata);
}

void 
ARM966Iss::
setWriteBerr() {
	SetWriteBerr();
}
 	
void 
ARM966Iss::
setIrq(uint32_t irq) {
	SetIrq(irq);
}
	
unsigned int 
ARM966Iss::
getDebugRegisterCount() const {
	return GetDebugRegisterCount();
}

uint32_t 
ARM966Iss::
getDebugRegisterValue(unsigned int reg) const {
	return GetDebugRegisterValue(reg);
}

void 
ARM966Iss::
setDebugRegisterValue(unsigned int reg, uint32_t value) {
	SetDebugRegisterValue(reg, value);
}

size_t 
ARM966Iss::
getDebugRegisterSize(unsigned int reg) const {
	return GetDebugRegisterSize(reg);
}
	
uint32_t 
ARM966Iss:: 
getDebugPC() const {
	return GetDebugPC();
}

void 
ARM966Iss::
setDebugPC(uint32_t pc) {
	SetDebugPC(pc);
}

void
ARM966Iss::
setICacheInfo( size_t line_size, size_t assoc, size_t n_lines ) {
	SetICacheInfo(line_size, assoc, n_lines);
}

void 
ARM966Iss::
setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines ) {
	SetDCacheInfo(line_size, assoc, n_lines);
}

/************************************************************************/
/* Methods required by the ISS Wrapper                              END */
/************************************************************************/

/************************************************************************/
/* Methods required by the cache interface                        START */
/************************************************************************/
	
void 
ARM966Iss::
PrInvalidateBlock(uint32_t set, uint32_t way) {
	inherited_CPU::external_memory_request = true;
	// TODO
}

void
ARM966Iss::
PrFlushBlock(uint32_t set, uint32_t way) {
	inherited_CPU::external_memory_request = true;
	// TODO
}

void 
ARM966Iss::
PrCleanBlock(uint32_t set, uint32_t way) {
	inherited_CPU::external_memory_request = true;
	// TODO
}

void 
ARM966Iss::
PrReset() {
	inherited_CPU::external_memory_request = true;
	// TODO
}

void 
ARM966Iss::
PrInvalidate() {
	inherited_CPU::external_memory_request = true;
	// TODO
}

void 
ARM966Iss::
PrInvalidateSet(uint32_t set) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

void 
ARM966Iss::
PrInvalidateBlock(address_t addr) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

void 
ARM966Iss::
PrFlushBlock(address_t addr) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

void 
ARM966Iss::
PrCleanBlock(address_t addr) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

void 
ARM966Iss::
PrZeroBlock(address_t addr) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

void 
ARM966Iss::
PrWrite(address_t addr, const uint8_t *buffer, uint32_t size) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

void 
ARM966Iss::
PrRead(address_t addr, uint8_t *buffer, uint32_t size) {
	// TODO
	inherited_CPU::external_memory_request = true;
}

/************************************************************************/
/* Methods required by the cache interface                          END */
/************************************************************************/

}
}

