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
 * - 2009-02-15
 *   Tarik Graba : Forked mips32 and sparcv8 to begin lm32
 */

#include "lm32.h"

namespace soclib { namespace common {

    uint32_t LM32Iss::get_absolute_dest_reg(ins_t ins) const
    {
        switch(LM32Iss::OpcodesTable [ins.J.op].instformat) {
            case JI:
                return 0;
            case RI:
                return ins.I.rX;
            case RR:
                return ins.R.rX;
            case CR:
                return ins.C.rR;
        }
        return 0;
    }

#define OPTABLE(x,y) {&LM32Iss::OP_LM32_##x, #x, y}

    LM32Iss::LM32_op_entry const LM32Iss::OpcodesTable [64]= {
        OPTABLE(srui   , RI ), OPTABLE(nori  , RI ),
        OPTABLE(muli   , RI ), OPTABLE(sh    , RR ),
        OPTABLE(lb     , RR ), OPTABLE(sri   , RI ),  
        OPTABLE(xori   , RI ), OPTABLE(lh    , RR ),
        OPTABLE(andi   , RI ), OPTABLE(xnori , RI ),  
        OPTABLE(lw     , RR ), OPTABLE(lhu   , RR ),
        OPTABLE(sb     , RR ), OPTABLE(addi  , RI ),  
        OPTABLE(ori    , RI ), OPTABLE(sli   , RI ),
        OPTABLE(lbu    , RR ), OPTABLE(be    , RR ),
        OPTABLE(bg     , RI ), OPTABLE(bge   , RI ),  
        OPTABLE(bgeu   , RI ), OPTABLE(bgu   , RI ),
        OPTABLE(sw     , RR ), OPTABLE(bne   , RR ),
        OPTABLE(andhi  , RI ), OPTABLE(cmpei , RI ),
        OPTABLE(cmpgi  , RI ), OPTABLE(cmpgei, RI ),
        OPTABLE(cmpgeui, RI ), OPTABLE(cmpgui, RI ),
        OPTABLE(orhi   , RI ), OPTABLE(cmpnei, RI ),
        OPTABLE(sru    , RR ), OPTABLE(nor   , RR ),
        OPTABLE(mul    , RR ), OPTABLE(divu  , RR ),
        OPTABLE(rcsr   , CR ), OPTABLE(sr    , RR ),
        OPTABLE(xor    , RR ), OPTABLE(div   , RR ),
        OPTABLE(and    , RR ), OPTABLE(xnor  , RR ),
        OPTABLE(reser  , JI ), OPTABLE(raise , JI ),
        OPTABLE(sextb  , RR ), OPTABLE(add   , RR ),
        OPTABLE(or     , RR ), OPTABLE(sl    , RR ),
        OPTABLE(b      , RR ), OPTABLE(modu  , RR ),
        OPTABLE(sub    , RR ), OPTABLE(reser , JI ),
        OPTABLE(wcsr   , CR ), OPTABLE(mod   , RR ),
        OPTABLE(call   , RR ), OPTABLE(sexth , RR ),
        OPTABLE(bi     , JI ), OPTABLE(cmpe  , RR ),
        OPTABLE(cmpg   , RR ), OPTABLE(cmpge , RR ),
        OPTABLE(cmpgeu , RR ), OPTABLE(cmpgu , RR ),
        OPTABLE(calli  , JI ), OPTABLE(cmpne , RR ),
    };

#undef OPTABLE

    std::string LM32Iss::get_ins_name( void ) const
    {
        return this->OpcodesTable[m_inst.J.op].name;
    }

    // Run the instruction 
    void LM32Iss::run() {
        // m_inst.J.op contains the opcode
        // The opcode is the same field for all instruction types
        void (LM32Iss::*func)() = LM32Iss::OpcodesTable [m_inst.J.op].func;
        (this->*func)();
    }

    // The instructions
#define LM32_function(x) void LM32Iss::OP_LM32_##x()

    LM32_function(raise) {
        //Soft exception fonction
        setInsDelay(4);
        if ((m_inst.ins & 0x7) == 0x7) {
            m_exception = true; 
            m_exception_cause = X_SYSTEM_CALL ; // scall instruction
        }
        else if((m_inst.ins & 0x3) == 0x2) {
            m_exception = true; 
            m_exception_cause = X_BREAK_POINT ; // break instruction
        }
        else {
            std::cout   << name() 
                << " This raise exception is not implemented !!" 
                << std::endl;
            exit (-1);
        }
    }
    LM32_function(reser ) {
        std::cout << name() << "ERROR: LM32 Iss gets reserved opcode !!" << std::endl;
        exit (-1);
    }

    //!Instruction srui behavior method.
    LM32_function( srui ){// shift rghit unsigned immediate
        r_gp[m_inst.I.rX] = (unsigned)r_gp[m_inst.I.rY] >> (m_inst.I.imd & 0x1F);
    }

    //!Instruction nori behavior method.
    LM32_function( nori ){// not or immediate
        r_gp[m_inst.I.rX] = ~(r_gp[m_inst.I.rY] | m_inst.I.imd);
    }

    //!Instruction muli behavior method.
    LM32_function( muli ){// immediate multiplication
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] * m_inst.I.imd;
    }

    //!Instruction sri behavior method.
    LM32_function( sri ){// shift right immediate
        r_gp[m_inst.I.rX] = (signed)r_gp[m_inst.I.rY] >> (m_inst.I.imd & 0x1F);
    }

    //!Instruction xori behavior method.
    LM32_function( xori ){// exclusive or immediate
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] ^ m_inst.I.imd;
    }

    //!Instruction andi behavior method.
    LM32_function( andi ){// and immediate
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] & m_inst.I.imd;
    }

    //!Instruction xnori behavior method.
    LM32_function( xnori ){// not exclusive or immediate
        r_gp[m_inst.I.rX] = ~(r_gp[m_inst.I.rY] ^ m_inst.I.imd);
    }

    //!Instruction addi behavior method.
    LM32_function( addi ){// add immediate
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] + sign_ext16(m_inst.I.imd);
    }

    //!Instruction ori behavior method.
    LM32_function( ori ){// or immediate
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] | m_inst.I.imd;
    }

    //!Instruction sli behavior method.
    LM32_function( sli ){// shift left immediate
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] << (m_inst.I.imd & 0x1F);
    }

    //!Instruction be behavior method.
    LM32_function( be ){// branch if equal
        if (r_gp[m_inst.I.rY] == r_gp[m_inst.I.rX])
        {
            m_cancel_next_ins = true; // To override r_npc
            m_next_pc = r_pc+ (sign_ext16(m_inst.I.imd)<<2);
            setInsDelay(4);
        }
    }

    //!Instruction bg behavior method.
    LM32_function( bg ){// branch if greater
        if ((signed)r_gp[m_inst.I.rY] > (signed)r_gp[m_inst.I.rX])
        {
            m_cancel_next_ins = true; // To override r_npc
            m_next_pc = r_pc+ (sign_ext16(m_inst.I.imd)<<2);
            setInsDelay(4);
        }
    }

    //!Instruction bge behavior method.
    LM32_function( bge ){// branch if greater or equal
        if ((signed)r_gp[m_inst.I.rY] >= (signed)r_gp[m_inst.I.rX])
        {
            m_cancel_next_ins = true; // To override r_npc
            m_next_pc = r_pc+ (sign_ext16(m_inst.I.imd)<<2);
            setInsDelay(4);
        }
    }

    //!Instruction bgeu behavior method.
    LM32_function( bgeu ){// branch if greater or equal unsigned
        if ((unsigned)r_gp[m_inst.I.rY] >= (unsigned)r_gp[m_inst.I.rX])
        {
            m_cancel_next_ins = true; // To override r_npc
            m_next_pc = r_pc+ (sign_ext16(m_inst.I.imd)<<2);
            setInsDelay(4);
        }
    }

    //!Instruction bgu behavior method.
    LM32_function( bgu ){// branch if greater unsigned
        if ((unsigned)r_gp[m_inst.I.rY] > (unsigned)r_gp[m_inst.I.rX])
        {
            m_cancel_next_ins = true; // To override r_npc
            m_next_pc = r_pc+ (sign_ext16(m_inst.I.imd)<<2);
            setInsDelay(4);
        }
    }

    //!Instruction bne behavior method.
    LM32_function( bne ){// branch if not equal
        if (r_gp[m_inst.I.rY] != r_gp[m_inst.I.rX])
        {
            m_cancel_next_ins = true; // To override r_npc
            m_next_pc = r_pc+ (sign_ext16(m_inst.I.imd)<<2);
            setInsDelay(4);
        }
    }

    //!Instruction andhi behavior method.
    LM32_function( andhi ){// and immediate with high 16bits
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] & (m_inst.I.imd << 16);
    }

    //!Instruction cmpei behavior method.
    LM32_function( cmpei ){// compare if equal immediate
        r_gp[m_inst.I.rX] = ((unsigned) r_gp[m_inst.I.rY] == (unsigned) sign_ext16(m_inst.I.imd));
    }

    //!Instruction cmpgi behavior method.
    LM32_function( cmpgi ){// compare if greater  immediate
        r_gp[m_inst.I.rX] = (signed)r_gp[m_inst.I.rY] >  (signed)sign_ext16(m_inst.I.imd);
    }

    //!Instruction cmpgei behavior method.
    LM32_function( cmpgei ){// compare if greater or equal immediate
        r_gp[m_inst.I.rX] = (signed)r_gp[m_inst.I.rY] >= (signed)sign_ext16(m_inst.I.imd);
    }

    //!Instruction cmpgeui behavior method.
    LM32_function( cmpgeui ){// compare if greater or equal immediate unsigned
        r_gp[m_inst.I.rX] = (unsigned)r_gp[m_inst.I.rY] >= (unsigned)m_inst.I.imd;
    }

    //!Instruction cmpgui behavior method.
    LM32_function( cmpgui ){// compare if greater immediate unsigned
        r_gp[m_inst.I.rX] = (unsigned)r_gp[m_inst.I.rY] > (unsigned)m_inst.I.imd;
    }

    //!Instruction orhi behavior method.
    LM32_function( orhi ){// or immediate high 16bits
        r_gp[m_inst.I.rX] = r_gp[m_inst.I.rY] | (m_inst.I.imd << 16);
    }

    //!Instruction cmpnei behavior method.
    LM32_function( cmpnei ){// compare if not equal immediate
        r_gp[m_inst.I.rX] = ((unsigned)r_gp[m_inst.I.rY] != (unsigned)sign_ext16(m_inst.I.imd));
    }

    //!Instruction sru behavior method.
    LM32_function( sru ){// shift right unsigned
        r_gp[m_inst.R.rX] = (unsigned)r_gp[m_inst.R.rY] >> (r_gp[m_inst.R.rZ] & 0x1F);
    }

    //!Instruction nor behavior method.
    LM32_function( nor ){// not or
        r_gp[m_inst.R.rX] = ~(r_gp[m_inst.R.rY] | r_gp[m_inst.R.rZ]);
    }

    //!Instruction mul behavior method.
    LM32_function( mul ){// multiplication
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] * r_gp[m_inst.R.rZ];
    }

    //!Instruction divu behavior method.
    LM32_function( divu ){// unsigned integer division
        if (r_gp[m_inst.R.rZ] == 0){
            m_exception = true;
            m_exception_cause = X_DIVISION_BY_ZERO; // division by 0
        }
        else {
            r_gp[m_inst.R.rX] = (unsigned)r_gp[m_inst.R.rY] / (unsigned)r_gp[m_inst.R.rZ];
            setInsDelay(34);
        }
    }

    //!Instruction sr behavior method.
    LM32_function( sr ){// shift right signed
        r_gp[m_inst.R.rX] = (signed)r_gp[m_inst.R.rY] >> (r_gp[m_inst.R.rZ] & 0x1F);
    }

    //!Instruction xor behavior method.
    LM32_function( xor ){// exclusive or
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] ^ r_gp[m_inst.R.rZ];
    }

    //!Instruction div behavior method.
    LM32_function( div ){// integer division
        if (r_gp[m_inst.R.rZ] == 0){
            m_exception = true;
            m_exception_cause = X_DIVISION_BY_ZERO; // division by 0
        }
        else {
            r_gp[m_inst.R.rX] = (signed)r_gp[m_inst.R.rY] / (signed)r_gp[m_inst.R.rZ];
            setInsDelay(34);
        }
    }

    //!Instruction and behavior method.
    LM32_function( and ){
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] & r_gp[m_inst.R.rZ];
    }

    //!Instruction xnor behavior method.
    LM32_function( xnor ){// not exclusive or
        r_gp[m_inst.R.rX] = ~(r_gp[m_inst.R.rY] ^ r_gp[m_inst.R.rZ]);
    }

    //!Instruction sextb behavior method.
    LM32_function( sextb ){// sign extension of a byte
        r_gp[m_inst.R.rX] = ((signed)(r_gp[m_inst.R.rY]<<24))>>24;
    }

    //!Instruction add behavior method.
    LM32_function( add ){// addition
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] + r_gp[m_inst.R.rZ];
    }

    //!Instruction or behavior method.
    LM32_function( or ){// or
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] | r_gp[m_inst.R.rZ];
    }

    //!Instruction sl behavior method.
    LM32_function( sl ){// shift left
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] << (r_gp[m_inst.R.rZ] & 0x1F);
    }

    //!Instruction b behavior method.
    LM32_function( b ){ // branch
        setInsDelay(4);
        m_cancel_next_ins = true; // To override r_npc
        m_next_pc = r_gp[m_inst.R.rY];
        if (m_inst.R.rY == 30)      // eret // return from exception
            r_IE.IE = r_IE.EIE;
        else if (m_inst.R.rY == 31) // bret // return from breakpoint
            r_IE.IE = r_IE.BIE;
    }

    //!Instruction modu behavior method.
    LM32_function( modu ){// unsigned modulo
        if (r_gp[m_inst.R.rZ] == 0){
            m_exception = true;
            m_exception_cause = X_DIVISION_BY_ZERO; // division by 0
        }
        else {
            r_gp[m_inst.R.rX] = (unsigned)r_gp[m_inst.R.rY] % (unsigned)r_gp[m_inst.R.rZ]; 
            setInsDelay(34);
        }
    }

    //!Instruction sub behavior method.
    LM32_function( sub ){// soustraction
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] - r_gp[m_inst.R.rZ]; 
    }

    //!Instruction mod behavior method.
    LM32_function( mod ){// signed modulo
        if (r_gp[m_inst.R.rZ] == 0){
            m_exception = true;
            m_exception_cause = X_DIVISION_BY_ZERO; // division by 0
        }
        else {
            r_gp[m_inst.R.rX] = (signed)r_gp[m_inst.R.rY] % (signed)r_gp[m_inst.R.rZ]; 
            setInsDelay(34);
        }
    }

    //!Instruction call behavior method.
    LM32_function( call ){// jump to sub routine
        setInsDelay(4);
        r_gp[ra] = r_npc ;// is pc + 4!!// return address
        m_cancel_next_ins = true; // To override r_npc
        m_next_pc = r_pc+ r_gp[m_inst.R.rY];
    }

    //!Instruction calli behavior method.
    LM32_function( calli ){//jump to sub routine immediate
        setInsDelay(4);
        r_gp[ra] = r_npc ; // is pc + 4!!// return address
        m_cancel_next_ins = true; // To override r_npc
        m_next_pc = r_pc + (sign_ext26(m_inst.J.imd)<<2);
    }

    //!Instruction bi behavior method.
    LM32_function( bi ){// branch immediate
        setInsDelay(4);
        m_cancel_next_ins = true; // To override r_npc
        m_next_pc = r_pc + (sign_ext26(m_inst.J.imd)<<2);
    }

    //!Instruction sexth behavior method.
    LM32_function( sexth ){// sign extension of a half
        r_gp[m_inst.R.rX] = ((signed)(r_gp[m_inst.R.rY]<<16))>>16;
    }

    //!Instruction cmpe behavior method.
    LM32_function( cmpe ){// compare if equal
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] == r_gp[m_inst.R.rZ];
    }

    //!Instruction cmpg behavior method.
    LM32_function( cmpg ){// compare if greater signed
        r_gp[m_inst.R.rX] = (signed)r_gp[m_inst.R.rY] > (signed)r_gp[m_inst.R.rZ];
    }

    //!Instruction cmpge behavior method.
    LM32_function( cmpge ){// compare if greater or equal signed
        r_gp[m_inst.R.rX] = (signed)r_gp[m_inst.R.rY] >= (signed)r_gp[m_inst.R.rZ];
    }

    //!Instruction cmpgeu behavior method.
    LM32_function( cmpgeu ){// compare if greater or equal unsigned
        r_gp[m_inst.R.rX] = (unsigned) r_gp[m_inst.R.rY] >= (unsigned) r_gp[m_inst.R.rZ];
    }

    //!Instruction cmpgu behavior method.
    LM32_function( cmpgu ){// compare if greater unsigned
        r_gp[m_inst.R.rX] = (unsigned) r_gp[m_inst.R.rY] > (unsigned) r_gp[m_inst.R.rZ];
    }

    //!Instruction cmpne behavior method.
    LM32_function( cmpne ){// compare if not equal
        r_gp[m_inst.R.rX] = r_gp[m_inst.R.rY] != r_gp[m_inst.R.rZ];
    }

    //!Instruction rcsr behavior method.
    LM32_function( rcsr ){// read control & status register
        switch (m_inst.C.csr){
            case 0x0:  // interrupt enable
                r_gp[m_inst.C.rR] = r_IE.whole ;
                break;
            case 0x1:  // interrupt mask
                r_gp[m_inst.C.rR] = r_IM ;
                break;
            case 0x2:  // interrupt pending
                r_gp[m_inst.C.rR] = r_IP ; 
                break;
            case 0x3:  // inst cache control
                r_gp[m_inst.C.rR] = 0x0; // write only!! no indication about the read value
                break;
            case 0x4:  // data cache control
                r_gp[m_inst.C.rR] = 0x0; // write only!! no indication about the read value
                break;
            case 0x5:  // cycle counter
                r_gp[m_inst.C.rR] = r_CC ;
                break;
            case 0x6:  // conf register 
                r_gp[m_inst.C.rR] = r_CFG.whole ;
                break;
            case 0x7:  // Exception base address
                r_gp[m_inst.C.rR] = r_EBA ;
                break;
            default:
                std::cout   << name()
                    << "Error: Read to Unkown CSR !!"<<std::endl;
                exit (-1);
                break;
        }
    }

    // XTN_CACHE must be XTN_DCACHE_INVAL or XTN_ICACHE_INVAL
#define FLUSH(XTN_CACHE)                                       \
    do {                                                       \
        struct DataRequest null_dreq = ISS_DREQ_INITIALIZER;   \
        m_dreq.req = null_dreq;                                \
        m_dreq.sign_extend = false;                            \
        m_dreq.dest_reg = 0;                                   \
        m_dreq.addr = 0;                                       \
        m_dreq.req.valid = true;                                      \
        m_dreq.req.addr = XTN_CACHE*4;                                  \
        m_dreq.req.wdata = 0;                                         \
        m_dreq.req.be = 0;                                            \
        m_dreq.req.type = XTN_WRITE;                                 \
        m_dreq.req.mode = MODE_USER;                                  \
    } while(0)

    //!Instruction wcsr behavior method.
    LM32_function( wcsr ){// write control & status register
        uint32_t wData = r_gp[m_inst.C.rW];
        switch (m_inst.C.csr){
            case 0x0: // interrupt enable
                r_IE.IE = 0x1 & wData;
                break;
            case 0x1: // interrupt mask
                r_IM = wData;
                break;
            case 0x2: // interrupt pending
                for (int i=0; i<32; i++){
                    // Bits are cleared by writing '1'!!!
                    if (wData>>i)
                        r_IP = r_IP & ~(1<<i);
                }
                break;
            case 0x3:  // inst cache ctrl
                FLUSH(XTN_ICACHE_INVAL);
                r_ICC  = 0x1; //a write invalidate the Icache
                break;
            case 0x4:  // data cache ctrl
                FLUSH(XTN_DCACHE_INVAL);
                r_DCC  = 0x1; //a read invalidate de Dcache
                break;
            case 0x5: // cycle counter 
                //r_CC  = wData; //read only
                break;
            case 0x6: // cfg register
                //r_CFG = wData; //read only
                break;
            case 0x7: // Exception base address
                r_EBA  = wData & 0xFFFFFF00;
                break;
            default:
                std::cout   << name()
                    << "Error: Write to Unkown CSR !!"<<std::endl;
                exit (-1);
                break;
        }
    }
#undef FLUSH
#undef LM32_function
}}
