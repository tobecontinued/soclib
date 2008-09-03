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

#include "arm7tdmi.h"
#include "unisim/component/cxx/processor/arm/cpu.tcc"
#include "unisim/component/cxx/processor/arm/exception.tcc"

namespace unisim {
namespace component {
namespace cxx {
namespace processor {
namespace arm {

template
class CPU<ARM7TDMI_Config>;

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
    snprintf(tmp, 32, "arm7tdmi_iss%d", (int)no);
    return std::string(tmp);
}

}

typedef unisim::component::cxx::processor::arm::CPU<
		unisim::component::cxx::processor::arm::ARM7TDMI_Config>
		inherited_CPU;

ARM7TDMIIss::ARM7TDMIIss(uint32_t ident) :
	inherited_CPU(this),
	Iss(mkname(ident), ident) {
}

ARM7TDMIIss::~ARM7TDMIIss() {}


/************************************************************************/
/* Methods required by the ISS Wrapper                            START */
/************************************************************************/

// ISS <-> Wrapper API
 	
void 
ARM7TDMIIss::
reset() {
	// reinitialize variables to its initial state
	Reset();
}
 	
uint32_t 
ARM7TDMIIss::
isBusy() {
	return IsBusy();
}

void 
ARM7TDMIIss::
step() {
	Step();
}

void 
ARM7TDMIIss::
nullStep( uint32_t time_passed) {
	NullStep(time_passed);
}

void 
ARM7TDMIIss::
getInstructionRequest(bool &req, uint32_t &addr) const {
	GetInstructionRequest(req, addr);
}

void 
ARM7TDMIIss::
setInstruction(bool error, uint32_t val) {
	SetInstruction(error, val);
}
 	
void 
ARM7TDMIIss::
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
ARM7TDMIIss::
setDataResponse(bool error, uint32_t rdata) {
	SetDataResponse(error, rdata);
}

void 
ARM7TDMIIss::
setWriteBerr() {
	SetWriteBerr();
}
 	
void 
ARM7TDMIIss::
setIrq(uint32_t irq) {
	SetIrq(irq);
}
	
unsigned int 
ARM7TDMIIss::
getDebugRegisterCount() const {
	return GetDebugRegisterCount();
}

uint32_t 
ARM7TDMIIss::
getDebugRegisterValue(unsigned int reg) const {
	return GetDebugRegisterValue(reg);
}

void 
ARM7TDMIIss::
setDebugRegisterValue(unsigned int reg, uint32_t value) {
	SetDebugRegisterValue(reg, value);
}

size_t 
ARM7TDMIIss::
getDebugRegisterSize(unsigned int reg) const {
	return GetDebugRegisterSize(reg);
}
	
uint32_t 
ARM7TDMIIss:: 
getDebugPC() const {
	return GetDebugPC();
}

void 
ARM7TDMIIss::
setDebugPC(uint32_t pc) {
	SetDebugPC(pc);
}

//void
//ARM7TDMIIss::
//setICacheInfo( size_t line_size, size_t assoc, size_t n_lines ) {
//	SetICacheInfo(line_size, assoc, n_lines);
//}
//
//void 
//ARM7TDMIIss::
//setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines ) {
//	SetDCacheInfo(line_size, assoc, n_lines);
//}

/************************************************************************/
/* Methods required by the ISS Wrapper                              END */
/************************************************************************/

/************************************************************************/
/* Methods required by the cache interface                        START */
/************************************************************************/
	
void 
ARM7TDMIIss::
PrInvalidateBlock(uint32_t set, uint32_t way) {
	// TODO
}

void
ARM7TDMIIss::
PrFlushBlock(uint32_t set, uint32_t way) {
	// TODO
}

void 
ARM7TDMIIss::
PrCleanBlock(uint32_t set, uint32_t way) {
	// TODO
}

void 
ARM7TDMIIss::
PrReset() {
	// TODO
}

void 
ARM7TDMIIss::
PrInvalidate() {
	// TODO
}

void 
ARM7TDMIIss::
PrInvalidateSet(uint32_t set) {
	// TODO
}

void 
ARM7TDMIIss::
PrInvalidateBlock(address_t addr) {
	// TODO
}

void 
ARM7TDMIIss::
PrFlushBlock(address_t addr) {
	// TODO
}

void 
ARM7TDMIIss::
PrCleanBlock(address_t addr) {
	// TODO
}

void 
ARM7TDMIIss::
PrZeroBlock(address_t addr) {
	// TODO
}

void 
ARM7TDMIIss::
PrWrite(address_t addr, const uint8_t *buffer, uint32_t size) {
	// TODO
}

void 
ARM7TDMIIss::
PrRead(address_t addr, uint8_t *buffer, uint32_t size) {
	// TODO
}

/************************************************************************/
/* Methods required by the cache interface                          END */
/************************************************************************/

}
}

