/*
 *  Copyright (c) 2007,
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
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 *  SERVICES;LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 *  SUCH DAMAGE.
 *
 * Authors: Gilles Mouchard (gilles.mouchard@cea.fr)
 */

#include "mpc7447a.h"
#include "unisim/util/queue/queue.hh"
#include "unisim/util/queue/queue.tcc"

namespace soclib {
namespace common {

const char *MPC7447AIss::debug_register_map[] = {
	   "r0",    "r1",    "r2",    "r3",    "r4",    "r5",    "r6",    "r7",
	   "r8",    "r9",   "r10",   "r11",   "r12",   "r13",   "r14",   "r15",
	  "r16",   "r17",   "r18",   "r19",   "r20",   "r21",   "r22",   "r23",
	  "r24",   "r25",   "r26",   "r27",   "r28",   "r29",   "r30",   "r31",
	   "f0",    "f1",    "f2",    "f3",    "f4",    "f5",    "f6",    "f7", 
	   "f8",    "f9",   "f10",   "f11",   "f12",   "f13",   "f14",   "f15",
	  "f16",   "f17",   "f18",   "f19",   "f20",   "f21",   "f22",   "f23",
	  "f24",   "f25",   "f26",   "f27",   "f28",   "f29",   "f30",   "f31",
	  "cia",   "msr",    "cr",    "lr",   "ctr",   "xer", "fpscr",   "vr0",
	  "vr1",   "vr2",   "vr3",   "vr4",   "vr5",   "vr6",   "vr7",   "vr8",
	  "vr9",  "vr10",  "vr11",  "vr12",  "vr13",  "vr14",  "vr15",  "vr16",
	 "vr17",  "vr18",  "vr19",  "vr20",  "vr21",  "vr22",  "vr23",  "vr24",
	 "vr25",  "vr26",  "vr27",  "vr28",  "vr29",  "vr30",  "vr31",  "vscr",
	 "vrsave"
};

MPC7447AIss::MPC7447AIss(uint32_t ident) :
	unisim::kernel::service::Object(MPC7447AIss::mkname(ident).c_str()),
	unisim::component::cxx::processor::powerpc::CPU<MPC7447AIssConfig>::CPU(MPC7447AIss::mkname(ident).c_str()),
	soclib::common::Iss::Iss(MPC7447AIss::mkname(ident), ident),
	data_cache_access_free_list(0),
	load_store_access_free_list(0)
{
	unsigned int i;
	typedef unisim::util::debug::Register *PRegister;
	debug_register_lookup = new PRegister[sizeof(debug_register_map) / sizeof(debug_register_map[0])];
	for(i = 0; i < sizeof(debug_register_map) / sizeof(debug_register_map[0]); i++)
	{
		debug_register_lookup[i] = inherited::GetRegister(debug_register_map[i]);
	}
}

MPC7447AIss::~MPC7447AIss()
{
	if(debug_register_lookup)
	{
		delete[] debug_register_lookup;
	}
}

void MPC7447AIss::reset()
{
	if(debug) std::cerr << __FUNCTION__ << ": reseting" << std::endl;
	inherited::Reset();
}

uint32_t MPC7447AIss::isBusy()
{
	return false;
}

void MPC7447AIss::step()
{
	if(debug) std::cerr << __FUNCTION__ << ": stepping" << std::endl;
	inherited::StepOneInstruction();
	inherited::OnBusCycle();
}

void MPC7447AIss::nullStep( uint32_t time_passed)
{
	if(debug) std::cerr << __FUNCTION__ << ": null step of " << time_passed << " cycles" << std::endl;
	if(time_passed)
	{
		do
		{
			inherited::OnBusCycle();
		} while(--time_passed);
	}
}

void MPC7447AIss::getInstructionRequest(bool &req, uint32_t &addr) const
{
	if(inherited::NeedFillingPrefetchBuffer())
	{
		req = true;
		addr = inherited::GetCIA();
		if(debug) std::cerr << __FUNCTION__ << ": instruction request at 0x" << std::hex << addr << std::dec << std::endl;
	}
	else
	{
		if(debug) std::cerr << __FUNCTION__ << ": no instruction request" << std::endl;
		req = false;
	}
}

void MPC7447AIss::setInstruction(bool error, uint32_t val)
{
	if(error)
	{
		if(debug) std::cerr << __FUNCTION__ << ": WARNING! transfer error acknowledge detected" << std::endl;
		inherited::ReqTEA();
		return;
	}
	if(debug) std::cerr << __FUNCTION__ << ": Filling prefetch buffer (instruction 0x" << std::hex << val << std::dec << ")" << std::endl;
	inherited::FillPrefetchBuffer(val);
}

void MPC7447AIss::getDataRequest(bool &req, enum DataAccessType &type, uint32_t &addr, uint32_t &data) const
{
	if(debug) std::cerr << __FUNCTION__ << std::endl;
	if(data_cache_access_queue.Empty())
	{
		if(debug) std::cerr << __FUNCTION__ << ": no data request" << std::endl;
		req = false;
		return;
	}

	const DataCacheAccess *data_cache_access = data_cache_access_queue.ConstFront();
	
	req = true;
	addr = data_cache_access->addr;
	type = data_cache_access->type;

	if(data_cache_access->load_store_access->type & unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::STORE)
	{
		switch(data_cache_access->size)
		{
			case 1:
				data = (uint32_t) data_cache_access->data[0];
				break;
			case 2:
				data = (uint32_t) *(uint16_t *) data_cache_access->data;
				break;
			case 4:
				data = *(uint32_t *) data_cache_access->data;
				break;
		}
	}

	if(debug) std::cerr << __FUNCTION__ << ": data request at 0x" << std::hex << addr << std::dec << " of size " << data_cache_access->size << std::endl;
}

void MPC7447AIss::setDataResponse(bool error, uint32_t rdata)
{
	if(data_cache_access_queue.Empty())
	{
		if(debug) std::cerr << __FUNCTION__ << ": ERROR! data cache access queue is empty." << std::endl;
		return;
	}

	DataCacheAccess *data_cache_access = data_cache_access_queue.Front();
	data_cache_access_queue.Pop();

	if(error)
	{
		if(debug) std::cerr << __FUNCTION__ << ": WARNING! transfer error acknowledge detected" << std::endl;
		inherited::ReqTEA();
		return;
	}

	if(debug) std::cerr << __FUNCTION__ << ": data response at 0x" << std::hex << data_cache_access->addr << std::dec << " of size " << data_cache_access->size << std::endl;

	LoadStoreAccess *load_store_access = data_cache_access->load_store_access;

	if(load_store_access->type & unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::LOAD)
	{
		switch(data_cache_access->size)
		{
			case 1:
				data_cache_access->data[0] = (uint8_t) rdata;
				break;
			case 2:
				{
					uint16_t rdata16 = (uint16_t) rdata;
					memcpy(data_cache_access->data, &rdata16, data_cache_access->size);
				}
				break;
			case 4:
				memcpy(data_cache_access->data, &rdata, data_cache_access->size);
				break;
		}
	}

	data_cache_access->valid = true;

	unsigned int num_data_cache_accesses = load_store_access->data_cache_access_queue.Size();
	unsigned int i;

	for(i = 0; i < num_data_cache_accesses; i++)
	{
		if(!data_cache_access->valid) return;
		if(load_store_access->type & unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::LOAD)
		{
			memcpy(load_store_access->data + data_cache_access->offset, data_cache_access->data, data_cache_access->size);
		}
	}

	if(load_store_access->type & unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::LOAD)
	{
		switch(load_store_access->type)
		{
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT8_LOAD:
				{
					uint8_t value = *(uint8_t *) load_store_access->data;
					inherited::SetGPR(load_store_access->reg_num, (uint32_t) value); // 8-bit to 32-bit zero extension
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT16_LOAD:
				{
					uint16_t value = *(uint16_t *) load_store_access->data;
					inherited::SetGPR(load_store_access->reg_num, (uint32_t) unisim::util::endian::BigEndian2Host(value)); // 16-bit to 32-bit zero extension
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::SINT16_LOAD:
				{
					uint16_t value = *(uint16_t *) load_store_access->data;
					inherited::SetGPR(load_store_access->reg_num, (uint32_t) (int16_t) unisim::util::endian::BigEndian2Host(value)); // 16-bit to 32-bit sign extension
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT32_LOAD:
				{
					uint32_t value = *(uint32_t *) load_store_access->data;
					inherited::SetGPR(load_store_access->reg_num, unisim::util::endian::BigEndian2Host(value));
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::FP32_LOAD:
				{
					uint32_t value = *(uint32_t *) load_store_access->data;
					unisim::component::cxx::processor::powerpc::Flags flags;
					flags.setRoundingMode(RN_ZERO);
					inherited::SetFPR(load_store_access->reg_num, unisim::component::cxx::processor::powerpc::SoftDouble(unisim::component::cxx::processor::powerpc::SoftFloat(unisim::util::endian::BigEndian2Host(value)), flags));
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::FP64_LOAD:
				{
					uint64_t value = *(uint64_t *) load_store_access->data;
					inherited::SetFPR(load_store_access->reg_num, unisim::component::cxx::processor::powerpc::SoftDouble(unisim::util::endian::BigEndian2Host(value)));
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT16_LOAD_BYTE_REVERSE:
				{
					uint16_t value = *(uint16_t *) load_store_access->data;
					inherited::SetGPR(load_store_access->reg_num, (uint32_t) unisim::util::endian::LittleEndian2Host(value)); // reverse bytes and 16-bit to 32-bit zero extension
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT32_LOAD_BYTE_REVERSE:
				{
					uint32_t value = *(uint32_t *) load_store_access->data;
					inherited::SetGPR(load_store_access->reg_num, (uint32_t) unisim::util::endian::LittleEndian2Host(value)); // reverse bytes
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT_LOAD_MSB:
				switch(load_store_access->size)
				{
					case 1:
					{
						uint8_t value = *(uint8_t *) load_store_access->data;
						inherited::SetGPR(load_store_access->reg_num, (uint32_t) value << 24);
						break;
					}
			
					case 2:
					{
						uint16_t value = *(uint16_t *) load_store_access->data;
						inherited::SetGPR(load_store_access->reg_num, (uint32_t) unisim::util::endian::BigEndian2Host(value) << 16);
						break;
					}
			
					case 3:
					{
						uint8_t *buffer = (uint8_t *) load_store_access->data;
						inherited::SetGPR(load_store_access->reg_num, ((uint32_t) buffer[0] << 24) | ((uint32_t) buffer[1] << 16) | ((uint32_t) buffer[2] << 8));
						break;
					}
			
					case 4:
					{
						uint32_t value = *(uint32_t *) load_store_access->data;
						inherited::SetGPR(load_store_access->reg_num, unisim::util::endian::BigEndian2Host(value));
						break;
					}
				}
				break;
			default:
				if(debug) std::cerr << "WARNING! unhandled load/store type in setDataResponse" << std::endl;
		}
	}

	load_store_access_queue.Pop();
	FreeLoadStoreAccess(load_store_access);
}

void MPC7447AIss::setWriteBerr()
{
	if(debug) std::cerr << __FUNCTION__ << ": WARNING! machine check exception detected" << std::endl;
	inherited::ReqMCP();
}

void MPC7447AIss::setIrq(uint32_t irq)
{
	if(!irq) return;

	if(irq & (1 << IRQ_HARD_RESET))
	{
		inherited::ReqHardReset();
	}
	if(irq & (1 << IRQ_SOFT_RESET))
	{
		inherited::ReqSoftReset();
	}
	if(irq & (1 << IRQ_MACHINE_CHECK_EXCEPTION))
	{
		inherited::ReqMCP();
	}
	if(irq & (1 << IRQ_EXTERNAL_INTERRUPT))
	{
		inherited::ReqExternalInterrupt();
	}
	if(irq & (1 << IRQ_SYSTEM_MANAGEMENT_INTERRUPT))
	{
		inherited::ReqSMI();
	}
	if(irq & (1 << IRQ_TRANSFER_ERROR_ACKNOWLEDGE))
	{
		inherited::ReqTEA();
	}
}

unsigned int MPC7447AIss::getDebugRegisterCount() const
{
	return sizeof(debug_register_map) / sizeof(debug_register_map[0]);
}

uint32_t MPC7447AIss::getDebugRegisterValue(unsigned int reg) const
{
	unisim::util::debug::Register *debug_reg = debug_register_lookup[reg];

	switch(debug_reg->GetSize())
	{
		case 1:
			{
				uint8_t value;
				debug_reg->GetValue(&value);
				return (uint32_t) value;
			}
			break;
		case 2:
			{
				uint16_t value;
				debug_reg->GetValue(&value);
				return (uint32_t) value;
			}
			break;
		case 4:
			{
				uint32_t value;
				debug_reg->GetValue(&value);
				return value;
			}
			break;
		default:
			if(debug) std::cerr << "WARNING! FIX the GDB server to handle 64-bit floating point numbers and 128-bit vector registers" << std::endl;
			return 0xffffffffUL;
	}
}

void MPC7447AIss::setDebugRegisterValue(unsigned int reg, uint32_t value)
{
	unisim::util::debug::Register *debug_reg = debug_register_lookup[reg];

	switch(debug_reg->GetSize())
	{
		case 1:
			{
				uint8_t value8 = value;
				debug_reg->SetValue(&value8);
			}
			break;
		case 2:
			{
				uint16_t value16 = value;
				debug_reg->SetValue(&value16);
			}
			break;
		case 4:
			{
				debug_reg->SetValue(&value);
			}
			break;
		default:
			if(debug) std::cerr << "WARNING! FIX the GDB server to handle 64-bit floating point numbers and 128-bit vector registers" << std::endl;
	}
}

size_t MPC7447AIss::getDebugRegisterSize(unsigned int reg) const
{
	return debug_register_lookup[reg]->GetSize();
}

uint32_t MPC7447AIss::getDebugPC() const
{
	return inherited::GetCIA();
}

void MPC7447AIss::setDebugPC(uint32_t pc)
{
	inherited::SetCIA(pc);
	inherited::SetNIA(pc);
}

// void MPC7447AIss::setICacheInfo( size_t line_size, size_t assoc, size_t n_lines )
// {
// }
// 
// void MPC7447AIss::setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines )
// {
// }
// 
// bool MPC7447AIss::exceptionBypassed( uint32_t cause )
// {
// }
// 
// int MPC7447AIss::cpuCauseToSignal( uint32_t cause ) const
// {
// }


// 	bool data_req;
// 	DataAccessType data_access_type;
// 	uint32_t data_addr;
// 	uint32_t data;

MPC7447AIss::LoadStoreAccess *MPC7447AIss::AllocateLoadStoreAccess()
{
	if(!load_store_access_free_list)
	{
		load_store_access_free_list = new LoadStoreAccess();
	}

	LoadStoreAccess *load_store_access = load_store_access_free_list;
	load_store_access_free_list = load_store_access->next_free;
	load_store_access->next_free = 0;
	return load_store_access;
}

void MPC7447AIss::FreeLoadStoreAccess(LoadStoreAccess *load_store_access)
{
	if(!load_store_access->data_cache_access_queue.Empty())
	{
		do
		{
			DataCacheAccess *data_cache_access = load_store_access->data_cache_access_queue.Front();
			FreeDataCacheAccess(data_cache_access);
			load_store_access->data_cache_access_queue.Pop();
		} while(!load_store_access->data_cache_access_queue.Empty());
	}

	load_store_access->next_free = load_store_access_free_list;
	load_store_access_free_list = load_store_access;
}

MPC7447AIss::DataCacheAccess *MPC7447AIss::AllocateDataCacheAccess()
{
	if(!data_cache_access_free_list)
	{
		data_cache_access_free_list = new DataCacheAccess();
	}

	DataCacheAccess *data_cache_access = data_cache_access_free_list;
	data_cache_access_free_list = data_cache_access->next_free;
	data_cache_access->next_free = 0;
	return data_cache_access;
}

void MPC7447AIss::FreeDataCacheAccess(DataCacheAccess *data_cache_access)
{
	data_cache_access->next_free = data_cache_access_free_list;
	data_cache_access_free_list = data_cache_access;
}

void MPC7447AIss::GenLoadStore(unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::Type type, unsigned int reg_num, inherited::address_t munged_ea, uint32_t size)
{
	if(debug) std::cerr << __FUNCTION__ << std::endl;

	LoadStoreAccess *load_store_access = AllocateLoadStoreAccess();
	load_store_access->type = type;
	load_store_access->reg_num = reg_num;
	load_store_access->munged_ea = munged_ea;
	load_store_access->size = size;

	if(load_store_access->type & unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::STORE)
	{
		switch(load_store_access->type)
		{
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT8_STORE:
				{
					uint8_t value = inherited::GetGPR(load_store_access->reg_num);
					memcpy(load_store_access->data, &value, sizeof(value));
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT16_STORE:
				{
					uint16_t value = unisim::util::endian::Host2BigEndian((uint16_t) inherited::GetGPR(load_store_access->reg_num));
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT32_STORE:
				{
					uint32_t value = unisim::util::endian::Host2BigEndian((uint32_t) inherited::GetGPR(load_store_access->reg_num));
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::FP32_STORE:
				{
					unisim::component::cxx::processor::powerpc::Flags flags;
					flags.setRoundingMode(RN_ZERO);
					uint32_t value = unisim::util::endian::Host2BigEndian(unisim::component::cxx::processor::powerpc::SoftFloat(inherited::GetFPR(load_store_access->reg_num), flags).queryValue());
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::FP64_STORE:
				{
					uint64_t value = unisim::util::endian::Host2BigEndian(inherited::GetFPR(load_store_access->reg_num).queryValue());
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::FP_STORE_LSW:
				{
					uint32_t value = unisim::util::endian::Host2BigEndian((uint32_t) inherited::GetFPR(load_store_access->reg_num).queryValue());
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT16_STORE_BYTE_REVERSE:
				{
					uint16_t value = unisim::util::endian::Host2LittleEndian((uint16_t) inherited::GetGPR(load_store_access->reg_num));
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT32_STORE_BYTE_REVERSE:
				{
					uint32_t value = unisim::util::endian::Host2LittleEndian((uint32_t) inherited::GetGPR(load_store_access->reg_num));
					memcpy(load_store_access->data, &value, load_store_access->size);
				}
				break;
			case unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::INT_STORE_MSB:
				switch(load_store_access->size)
				{
					case 1:
						{
							uint8_t value = inherited::GetGPR(load_store_access->reg_num) >> 24;
							memcpy(load_store_access->data, &value, load_store_access->size);
						}
						break;
					case 2:
						{
							uint16_t value = unisim::util::endian::Host2BigEndian((uint16_t)(inherited::GetGPR(load_store_access->reg_num) >> 16));
							memcpy(load_store_access->data, &value, load_store_access->size);
							break;
						}
			
					case 3:
						{
							uint32_t value = inherited::GetGPR(load_store_access->reg_num);
							uint8_t buffer[3];
							buffer[0] = value >> 24;
							buffer[1] = value >> 16;
							buffer[2] = value >> 8;
							memcpy(load_store_access->data, buffer, load_store_access->size);
							break;
						}
			
					case 4:
						{
							uint32_t value = unisim::util::endian::Host2BigEndian(inherited::GetGPR(load_store_access->reg_num));
							memcpy(load_store_access->data, &value, load_store_access->size);
							break;
						}
				}
				break;
		}
	}

	if(debug) std::cerr << "load_store_access_queue.Push(): before load_store_access_queue.Size() = " << load_store_access_queue.Size() << std::endl;
	load_store_access_queue.Push(load_store_access);

	uint32_t addr = munged_ea;
	uint32_t offset = 0;
	while(size > 0)
	{
		uint32_t size_to_fsb_boundary = size < SOCLIB_FSB_WIDTH ? size : SOCLIB_FSB_WIDTH;
		uint32_t data_cache_access_size;

		switch(size_to_fsb_boundary)
		{
			case 1:
				data_cache_access_size = 1;
				break;
			case 2:
			case 3:
				if(addr & 1)
				{
					data_cache_access_size = 1;
				}
				else
				{
					data_cache_access_size = 2;
				}
				break;
			case 4:
				if(addr & 3)
				{
					if(addr & 1)
					{
						data_cache_access_size = 1;
					}
					else
					{
						data_cache_access_size = 2;
					}
				}
				else
				{
					data_cache_access_size = 4;
				}
				break;
		}

		DataCacheAccess *data_cache_access = AllocateDataCacheAccess();

		data_cache_access->addr = addr;
		data_cache_access->offset = offset;
		data_cache_access->size = data_cache_access_size;
		data_cache_access->load_store_access = load_store_access;

		if(type & unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::STORE)
		{
			switch(data_cache_access_size)
			{
				case 1: data_cache_access->type = WRITE_BYTE; break;
				case 2: data_cache_access->type = WRITE_HALF; break;
				case 4: data_cache_access->type = WRITE_WORD; break;
			}

			memcpy(data_cache_access->data, load_store_access->data + offset, data_cache_access_size);
		}
		else
		{
			switch(data_cache_access_size)
			{
				case 1: data_cache_access->type = READ_BYTE; break;
				case 2: data_cache_access->type = READ_HALF; break;
				case 4: data_cache_access->type = READ_WORD; break;
			}
		}

		if(debug) std::cerr << "load_store_access->data_cache_access_queue.Push(): before load_store_access->data_cache_access_queue.Size() = " << load_store_access->data_cache_access_queue.Size() << std::endl;
		load_store_access->data_cache_access_queue.Push(data_cache_access);
		if(debug) std::cerr << "data_cache_access_queue.Push(): before data_cache_access_queue.Size() = " << data_cache_access_queue.Size() << std::endl;
		data_cache_access_queue.Push(data_cache_access);

		addr += data_cache_access_size;
		offset += data_cache_access_size;
		size -= data_cache_access_size;
	}
}


} // end of namespace common
} // end of namespace soclib
