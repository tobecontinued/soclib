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
#ifndef _SOCLIB_ISS_H_
#define _SOCLIB_ISS_H_

#include <systemc>
#include <cassert>
#include "common/endian.h"
#include "common/register.h"

namespace soclib { namespace common {

using namespace sc_core;

class Iss
{
public:
	enum DataAccessType {
		MEM_NONE,
		MEM_LB,
		MEM_LBU,
		MEM_LH,
        MEM_LHBR, /* Load half byte-swapped, PPC */
        MEM_LWBR, /* Load word byte-swapped, PPC */
		MEM_LHU,
		MEM_LW,
		MEM_SB,
		MEM_SH,
		MEM_SW,
		MEM_LL,
		MEM_SC,
		MEM_SWAP,
		MEM_INVAL
	};

    static inline const char* dataAccessTypeName( enum DataAccessType e )
    {
        static const char *const type_names[15] =
            {"NONE", "LB", "LBU", "LH", "LHBR",
             "LWBR", "LHU", "LW", "SB", "SH",
             "SW", "LL", "SC", "SWAP", "INVAL" };
        if ( e > MEM_INVAL )
            return "Invalid";
        return type_names[e];
    }

protected:
	uint32_t 	r_pc;			// Program Counter

	enum DataAccessType 	r_mem_type;  		// Data Cache access type
	uint32_t 	r_mem_addr;  		// Data Cache address
	uint32_t 	r_mem_wdata;  		// Data Cache data value (write)
	uint32_t	r_mem_dest;  		// Data Cache destination register (read)
	bool		r_dbe;			// Asynchronous Data Bus Error (write)

	uint32_t	m_rdata;
	uint32_t 	m_irq;
	bool		m_ibe;
	bool		m_dbe;

	const uint32_t m_ident;
	const std::string m_name;

private:
    // Instruction latency simulation
    uint32_t m_ins_delay;

public:
    virtual ~Iss() {}

    void step();
    void nullStep();

    void reset();
	void setInstruction(bool error, uint32_t val);

	Iss( const std::string &name, uint32_t ident )
		: m_ident(ident),
		  m_name(name)
	{
	}

    inline void setInsDelay( uint32_t delay )
    {
        assert( delay > 0 );
        m_ins_delay = delay-1;
    }

    inline bool isBusy()
    {
        return m_ins_delay;
    }

	inline void getInstructionRequest(bool &req, uint32_t &address) const
	{
        req = true;
		address = r_pc;
	}

	inline void getDataRequest(
        enum DataAccessType &type,
        uint32_t &address,
        uint32_t &wdata) const
	{
		address = r_mem_addr;
		wdata = r_mem_wdata;
		type = r_mem_type;
	}

	inline void clearDataRequest()
	{
		r_mem_addr = 0;
		r_mem_wdata = 0;
		r_mem_type = MEM_NONE;
        r_mem_dest = 0;
	}

	inline void setWriteBerr()
	{
		r_dbe = true;
	}

	inline void setRdata(bool error, uint32_t rdata)
	{
		m_dbe = error;
		m_rdata = rdata;
	}

	inline void setIrq(uint32_t irq)
	{
		m_irq = irq;
	}

    // processor internal registers access API, used by
    // debugger. Register numbering must match gdb packet order.

    virtual inline unsigned int get_register_count() const
    {
        return 0;
    }

    virtual inline uint32_t get_register_value(unsigned int reg) const
    {
        return 0;
    }

    virtual inline void set_register_value(unsigned int reg, uint32_t value)
    {
    }

    virtual inline size_t get_register_size(unsigned int reg) const
    {
        return 0;
    }

protected:

    void doneNullStep()
    {
        if ( m_ins_delay )
            --m_ins_delay;
    }

    void reset( uint32_t reset_addr )
    {
        r_pc = reset_addr;
        r_dbe = false;
        m_ibe = false;
        m_dbe = false;
        r_mem_type = MEM_NONE;
        m_ins_delay = 0;
    }

    static inline bool addressNotAligned( uint32_t address, DataAccessType type )
    {
        switch (type) {
        case MEM_NONE:
        case MEM_INVAL:
        case MEM_LB:
        case MEM_LBU:
        case MEM_SB:
            return false;
        case MEM_LH:
        case MEM_LHBR:
        case MEM_LHU:
        case MEM_SH:
            return (address&1);
        case MEM_LWBR:
        case MEM_LW:
        case MEM_SW:
        case MEM_LL:
        case MEM_SC:
        case MEM_SWAP:
            return (address&3);
        }
        assert(0&&"This is impossible");
        return false;
    }

    static inline bool isReadAccess( DataAccessType type )
    {
        switch (type) {
        case MEM_LB:
        case MEM_LBU:
        case MEM_LH:
        case MEM_LHBR:
        case MEM_LHU:
        case MEM_LWBR:
        case MEM_LW:
        case MEM_LL:
        case MEM_SC:
        case MEM_SWAP:
            return true;
        case MEM_NONE:
        case MEM_INVAL:
        case MEM_SB:
        case MEM_SH:
        case MEM_SW:
            return false;
        }
        assert(0&&"This is impossible");
        return false;
    }

    static inline bool isWriteAccess( DataAccessType type )
    {
        switch (type) {
        case MEM_LB:
        case MEM_LBU:
        case MEM_LH:
        case MEM_LHBR:
        case MEM_LHU:
        case MEM_LWBR:
        case MEM_LW:
        case MEM_LL:
        case MEM_NONE:
        case MEM_INVAL:
            return false;
        case MEM_SB:
        case MEM_SH:
        case MEM_SW:
        case MEM_SC:
        case MEM_SWAP:
            return true;
        }
        assert(0&&"This is impossible");
        return false;
    }
};

}}

#endif // _SOCLIB_ISS_H_

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
