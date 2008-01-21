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
#include <signal.h>

namespace soclib { namespace common {

using namespace sc_core;

class Iss
{
public:
	enum DataAccessType {
		READ_WORD,
        READ_HALF,
        READ_BYTE,
		LINE_INVAL,
		WRITE_WORD,
		WRITE_HALF,
		WRITE_BYTE,
		STORE_COND,
		READ_LINKED,
	};

    static inline const char* dataAccessTypeName( enum DataAccessType e )
    {
        static const char *const type_names[15] =
            { "READ_WORD", "READ_HALF", "READ_BYTE", "LINE_INVAL", "WRITE_WORD",
              "WRITE_HALF", "WRITE_BYTE", "STORE_COND", "READ_LINKED" };
        if ( e > READ_LINKED )
            return "Invalid";
        return type_names[e];
    }

protected:

	const uint32_t m_ident;
	const std::string m_name;

public:
    virtual ~Iss() {}

    inline const std::string & name() const
    {
        return m_name;
    }

	Iss( const std::string &name, uint32_t ident )
		: m_ident(ident),
		  m_name(name)
	{
	}

    // ISS <-> Wrapper API

    virtual void reset() = 0;

    virtual uint32_t isBusy() = 0;
    virtual void step() = 0;
    virtual void nullStep( uint32_t time_passed = 1 ) = 0;

    virtual void getInstructionRequest(bool &req, uint32_t &addr) const = 0;
	virtual void setInstruction(bool error, uint32_t val) = 0;

	virtual void getDataRequest(bool &req, enum DataAccessType &type,
                                uint32_t &addr, uint32_t &data) const = 0;
	virtual void setDataResponse(bool error, uint32_t rdata) = 0;
    virtual void setWriteBerr() = 0;

	virtual void setIrq(uint32_t irq) = 0;

    // processor internal registers access API, used by
    // debugger. Register numbering must match gdb packet order.

    virtual unsigned int getDebugRegisterCount() const = 0;
    virtual uint32_t getDebugRegisterValue(unsigned int reg) const = 0;
    virtual void setDebugRegisterValue(unsigned int reg, uint32_t value) = 0;
    virtual size_t getDebugRegisterSize(unsigned int reg) const = 0;

    virtual uint32_t getDebugPC() const = 0;
    virtual void setDebugPC(uint32_t) = 0;

protected:

    virtual bool exceptionBypassed( uint32_t cause )
    {
        return false;
    }

    virtual int cpuCauseToSignal( uint32_t cause ) const
    {
        return 5;       // GDB SIGTRAP
    }

    static inline bool addressNotAligned( uint32_t address, DataAccessType type )
    {
        switch (type) {
        case LINE_INVAL:
		case WRITE_BYTE:
        case READ_BYTE:
            return false;
        case READ_HALF:
		case WRITE_HALF:
            return (address&1);
        case READ_LINKED:
        case READ_WORD:
		case WRITE_WORD:
		case STORE_COND:
            return (address&3);
        }
        assert(0&&"This is impossible");
        return false;
    }

    static inline bool isReadAccess( DataAccessType type )
    {
        switch (type) {
        case READ_LINKED:
        case READ_WORD:
        case READ_HALF:
        case READ_BYTE:
		case STORE_COND:
            return true;
        case LINE_INVAL:
		case WRITE_WORD:
		case WRITE_HALF:
		case WRITE_BYTE:
            return false;
        }
        assert(0&&"This is impossible");
        return false;
    }

    static inline bool isWriteAccess( DataAccessType type )
    {
        switch (type) {
        case READ_LINKED:
        case READ_WORD:
        case READ_HALF:
        case READ_BYTE:
        case LINE_INVAL:
            return false;
		case WRITE_WORD:
		case WRITE_HALF:
		case WRITE_BYTE:
		case STORE_COND:
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
