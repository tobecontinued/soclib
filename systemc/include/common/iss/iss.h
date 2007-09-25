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

#include <systemc.h>
#include "common/endian.h"
#include "common/register.h"

namespace soclib { namespace common {

class Iss
{
public:
	enum DataAccessType {
		MEM_NONE,
		MEM_LB,
		MEM_LBU,
		MEM_LH,
        MEM_LHBR, /* Load half byte-swapped, PPC */
        MEM_LWR, /* Load word byte-swapped, PPC */
		MEM_LHU,
		MEM_LW,
		MEM_SB,
		MEM_SH,
		MEM_SW,
		MEM_INVAL
	};

protected:
	uint32_t 	r_pc;			// Program Counter
	uint32_t 	r_npc;			// Next Program Counter

	uint32_t 	r_mem_type;  		// Data Cache access type
	uint32_t 	r_mem_addr;  		// Data Cache address
	uint32_t 	r_mem_wdata;  		// Data Cache data value (write)
	uint32_t	r_mem_dest;  		// Data Cache destination register (read)
	bool		r_dbe;			// Asynchronous Data Bus Error (write)

	uint32_t   	m_instruction;
	uint32_t	m_rdata;
	uint32_t 	m_irq;
	bool		m_ibe;
	bool		m_dbe;

	const uint32_t m_ident;
	const std::string m_name;

public:
	Iss( const std::string &name, uint32_t ident )
		: m_ident(ident),
		  m_name(name)
	{
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_pc);
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_npc);
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_dbe);
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_mem_type);
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_mem_addr);
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_mem_dest);
		SOCLIB_REG_RENAME_NAME(m_name.c_str(), r_mem_wdata);
	}

    inline bool addressNotAligned( uint32_t address, DataAccessType type )
    {
        switch (type) {
        case MEM_NONE:
        case MEM_INVAL:
            return false;
        case MEM_LB:
        case MEM_LBU:
        case MEM_SB:
            return false;
        case MEM_LH:
        case MEM_LHBR:
        case MEM_LHU:
        case MEM_SH:
            return (address&1);
        case MEM_LWR:
        case MEM_LW:
        case MEM_SW:
            return (address&3);
        }
    }

	virtual ~Iss() {}

	virtual void step() = 0;
	virtual void reset() = 0;

	inline void getInstructionRequest(uint32_t &address)
	{
		address = r_pc;
	}

	inline void getDataRequest(uint32_t &type, uint32_t &address, uint32_t &wdata)
	{
		address = r_mem_addr;
		wdata = r_mem_wdata;
		type = r_mem_type;
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

	inline void setInstruction(bool error, uint32_t val)
	{
		m_ibe = error;
		m_instruction = val;
	}

	inline void setIrq(uint32_t irq)
	{
		m_irq = irq;
	}

protected:
	virtual void run() = 0;
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
