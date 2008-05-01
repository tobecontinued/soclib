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
 *    Nicolas Pouillon <nipo@ssji.net>, 2007
 *    Alain Greiner <alain.greiner@lip6.fr>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 *
 * History:
 * - 2007-06-15
 *     Nicolas Pouillon, Alain Greiner: Model created
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
		unisim::component::cxx::processor::arm::ARM966E_S_Config>
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
	// TODO
	bool is_read;
	int size;
	GetDataRequest(req, is_read, size, addr, data);
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
	// TODO
}

void
ARM966Iss::
PrFlushBlock(uint32_t set, uint32_t way) {
	// TODO
}

void 
ARM966Iss::
PrCleanBlock(uint32_t set, uint32_t way) {
	// TODO
}

void 
ARM966Iss::
PrReset() {
	// TODO
}

void 
ARM966Iss::
PrInvalidate() {
	// TODO
}

void 
ARM966Iss::
PrInvalidateSet(uint32_t set) {
	// TODO
}

void 
ARM966Iss::
PrInvalidateBlock(address_t addr) {
	// TODO
}

void 
ARM966Iss::
PrFlushBlock(address_t addr) {
	// TODO
}

void 
ARM966Iss::
PrCleanBlock(address_t addr) {
	// TODO
}

void 
ARM966Iss::
PrZeroBlock(address_t addr) {
	// TODO
}

void 
ARM966Iss::
PrWrite(address_t addr, const uint8_t *buffer, uint32_t size) {
	// TODO
}

void 
ARM966Iss::
PrRead(address_t addr, uint8_t *buffer, uint32_t size) {
	// TODO
}

/************************************************************************/
/* Methods required by the cache interface                          END */
/************************************************************************/

}
}

