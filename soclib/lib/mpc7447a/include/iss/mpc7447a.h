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

#ifndef __UNISIM_COMPONENT_CXX_PROCESSOR_POWERPC_SOCLIB_HH__
#define __UNISIM_COMPONENT_CXX_PROCESSOR_POWERPC_SOCLIB_HH__

// SocLib ISS <-> Wrapper interface definition
#include "iss.h"

#include "mpc7447a_cpu.h"

namespace soclib {
namespace common {

typedef unisim::component::cxx::processor::powerpc::MPC7447ADebugConfig MPC7447AIssConfig;

class MPC7447AIss :
	public unisim::component::cxx::processor::powerpc::CPU<MPC7447AIssConfig>,
	public soclib::common::Iss
{
public:
	static const int n_irq = 6;
	typedef unisim::component::cxx::processor::powerpc::CPU<MPC7447AIssConfig> inherited;
	static const uint32_t IRQ_HARD_RESET = 0;
	static const uint32_t IRQ_SOFT_RESET = 1;
	static const uint32_t IRQ_MACHINE_CHECK_EXCEPTION = 2;
	static const uint32_t IRQ_EXTERNAL_INTERRUPT = 3;
	static const uint32_t IRQ_SYSTEM_MANAGEMENT_INTERRUPT = 4;
	static const uint32_t IRQ_TRANSFER_ERROR_ACKNOWLEDGE = 5;

	MPC7447AIss(uint32_t ident);
	virtual ~MPC7447AIss();

    virtual void reset();
    virtual uint32_t isBusy();
    virtual void step();
    virtual void nullStep( uint32_t time_passed = 1 );
    virtual void getInstructionRequest(bool &req, uint32_t &addr) const;
	virtual void setInstruction(bool error, uint32_t val);
	virtual void getDataRequest(bool &req, enum DataAccessType &type,
                                uint32_t &addr, uint32_t &data) const;
	virtual void setDataResponse(bool error, uint32_t rdata);
    virtual void setWriteBerr();
	virtual void setIrq(uint32_t irq);

    // processor internal registers access API, used by
    // debugger. Register numbering must match gdb packet order.

    virtual unsigned int getDebugRegisterCount() const;
    virtual uint32_t getDebugRegisterValue(unsigned int reg) const;
    virtual void setDebugRegisterValue(unsigned int reg, uint32_t value);
    virtual size_t getDebugRegisterSize(unsigned int reg) const;
    virtual uint32_t getDebugPC() const;
    virtual void setDebugPC(uint32_t);

	// the following method have default implementations
    //virtual void setICacheInfo( size_t line_size, size_t assoc, size_t n_lines );
    //virtual void setDCacheInfo( size_t line_size, size_t assoc, size_t n_lines );
    //virtual bool exceptionBypassed( uint32_t cause );
    //virtual int cpuCauseToSignal( uint32_t cause ) const;

private:
	static const uint32_t SOCLIB_FSB_WIDTH = 4; // Do not modify this value
	static const bool debug = false;

	static const char *debug_register_map[];
	unisim::util::debug::Register **debug_register_lookup;

	class LoadStoreAccess;

	class DataCacheAccess
	{
	public:
		bool valid;
		uint32_t addr;
		uint32_t offset;
		DataAccessType type;
		uint32_t size;
		uint8_t data[sizeof(uint32_t)];
		LoadStoreAccess *load_store_access;
		DataCacheAccess *next_free;
	};

	class LoadStoreAccess
	{
	public:
		unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::Type type;                                           // type of Load/Store access
		unsigned int reg_num;                                // destination register
		uint32_t munged_ea;                                  // Munged Effective Address
		uint32_t size;                                       // size in byte of the access
		bool valid;                                          // true if data is available (for a load)
		uint8_t data[8];                                     // data to store/loaded data

		class DataCacheAccessQueueConfig
		{
		public:
			static const bool DEBUG = true;
			typedef DataCacheAccess *ELEMENT;
			static const unsigned int SIZE = 8;
		};

		unisim::util::queue::Queue<DataCacheAccessQueueConfig> data_cache_access_queue;

		LoadStoreAccess *next_free;
	};

	class DataCacheAccessQueueConfig
	{
	public:
		static const bool DEBUG = true;
		typedef DataCacheAccess *ELEMENT;
		static const unsigned int SIZE = 8;
	};

	class LoadStoreAccessQueueConfig
	{
	public:
		static const bool DEBUG = true;
		typedef LoadStoreAccess *ELEMENT;
		static const unsigned int SIZE = 8;
	};

	DataCacheAccess *data_cache_access_free_list;
	unisim::util::queue::Queue<DataCacheAccessQueueConfig> data_cache_access_queue;
	LoadStoreAccess *load_store_access_free_list;
	unisim::util::queue::Queue<LoadStoreAccessQueueConfig> load_store_access_queue;

	DataCacheAccess *AllocateDataCacheAccess();
	LoadStoreAccess *AllocateLoadStoreAccess();
	void FreeDataCacheAccess(DataCacheAccess *data_cache_access);
	void FreeLoadStoreAccess(LoadStoreAccess *load_store_access);
	virtual void GenLoadStore(unisim::component::cxx::processor::powerpc::LoadStoreAccess<MPC7447AIssConfig>::Type type, unsigned int reg_num, inherited::address_t munged_ea, uint32_t size);

	static inline std::string mkname(uint32_t no)
	{
		char tmp[32];
		snprintf(tmp, 32, "mpc7447a_iss%d", (int)no);
		return std::string(tmp);
	}
};

} // end of namespace common
} // end of namespace soclib

#endif
