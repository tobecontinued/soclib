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
 * Copyright (c) TelecomParisTECH
 *         Tarik Graba <tarik.graba@telecom-paristech.fr>, 2009
 *
 * Based on sparcv8 and mips32 code
 *         Alexis Polti <polti@telecom-paristech.fr>, 2008
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2007
 *
 * Maintainers: tarik.graba@telecom-paristech.fr
 *
 * $Id$
 *
 * History:
 * - 2009-07-08
 *   Tarik Graba : the iss is now sensitive to high level irqs
 * - 2009-02-15
 *   Tarik Graba : Forked mips32 and sparcv8 to begin lm32
 */

#include "lm32.h"

namespace soclib { namespace common {

    void LM32Iss::dump( ) const
    {
        dump_pc (m_name);
        dump_regs("");
    }

    void LM32Iss::dump_pc( const std::string &msg) const
    {
        std::cout
            << msg << std::endl
            << std::hex << std::showbase
            << "\tPC=" << std::setw(10) << std::setfill('0') << std::internal 
            << std::hex << std::showbase << r_pc
            << " / NPC=" << std::setw(10) << std::setfill('0') << std::internal 
            << std::hex << std::showbase << r_npc
            << " / next_pc=" << std::setw(10) << std::setfill('0') << std::internal 
            << std::hex << std::showbase << m_next_pc << std::endl
            << "\tIns=" << std::setw(10) << std::setfill('0') << std::internal 
            << std::hex << std::showbase << m_inst.ins
            << " (" << get_ins_name() << ")" << std::endl ;

    }

    void LM32Iss::dump_regs( const std::string &msg) const
    {
    std::cout << msg << std::endl;
#define DUMP_REG(n, prefix, val)                                    \
        std::cout <<  prefix << std::setw(2) << std::setfill('0')   \
        << std::dec << n << ": "                                    \
        << std::setw(10) << std::setfill('0') << std::internal      \
        << std::hex << std::showbase << val <<"\t"

#define DUMP_LINE(n)  DUMP_REG(n  ,"r",r_gp[n])   ;\
                      DUMP_REG(n+1,"r",r_gp[n+1]) ;\
                      DUMP_REG(n+2,"r",r_gp[n+2]) ;\
                      std::cout << std::endl

            DUMP_LINE(0);
            DUMP_LINE(3);
            DUMP_LINE(6);
            DUMP_LINE(9);
            DUMP_LINE(12);
            DUMP_LINE(15);
            DUMP_LINE(18);
            DUMP_LINE(21);
            DUMP_REG(24,"r",r_gp[24]);
            DUMP_REG(25,"r",r_gp[25]);
            std::cout << std::endl;

            DUMP_REG(26,"(gp) r",r_gp[26]);
            DUMP_REG(27,"(fp) r",r_gp[27]);
            std::cout << std::endl;
            DUMP_REG(28,"(sp) r",r_gp[28]);
            DUMP_REG(29,"(ra) r",r_gp[29]);
            std::cout << std::endl;
            DUMP_REG(30,"(ea) r",r_gp[30]);
            DUMP_REG(31,"(ba) r",r_gp[31]);
            std::cout << std::endl;

#undef DUMP_LINE
#undef DUMP_REG
    }

    std::string LM32Iss::GetExceptioName (int type) const
    {
        switch(type){
#define rstr(x) case x: return #x
            rstr( X_RESET            );
            rstr( X_BREAK_POINT      );
            rstr( X_INST_BUS_ERROR   );
            rstr( X_WATCH_POINT      );
            rstr( X_DATA_BUS_ERROR   );
            rstr( X_DIVISION_BY_ZERO );
            rstr( X_INTERRUPT        );
            rstr( X_SYSTEM_CALL      );
#undef rstr
            default: return "Unkown exception";
        }
    }

    unsigned int LM32Iss::debugGetRegisterCount() const
    {
        return 32 + 5;
    }

    Iss2::debug_register_t LM32Iss::debugGetRegisterValue(unsigned int reg) const
    {
        switch (reg)
        {
            case 0:
                return 0;
            case 1 ... 31:
                return r_gp[reg];
            case 32:
                return r_pc;
            case 33:
                return m_exception_cause;
            case 34:
                return r_EBA; 
            case 35:
                return r_DEBA; 
            case 36:
                return r_IE.whole; 
            default:
                return 0;
        }
    }

    size_t LM32Iss::debugGetRegisterSize(unsigned int reg) const
    {
        return 32;
    }

    void LM32Iss::debugSetRegisterValue(unsigned int reg,  debug_register_t value)
    {
        switch (reg)
        {
            case 1 ... 31:
                r_gp[reg] = value;
                break;
            case 32:
                r_pc = value;
                r_npc = value+4;
                break;
            case 33:
                m_exception_cause = (except_t)value;
                break;
            case 34:
                r_EBA = value;
                break;
            case 35:
                r_DEBA = value;
                break;
            case 36:
                r_IE.whole = value;
                break;
            default:
                break;
        }
    }

    Iss2::addr_t LM32Iss::debugGetPC() const
    {
        return r_pc;
    }

    void LM32Iss::debugSetPC(addr_t pc)
    {
        r_pc = pc;
        r_npc = pc+4;
    }

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

