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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2007
 *
 * Maintainers: nipo
 *
 * $Id$
 *
 * History:
 * - 2007-09-19
 *   Nicolas Pouillon: Fix overflow
 *
 * - 2007-06-15
 *   Nicolas Pouillon, Alain Greiner: Model created
 */

#include "mips.h"
#include "base_module.h"
#include "arithmetics.h"

#include <strings.h>

namespace soclib { namespace common {

#define COPROC_REGNUM(no, sel) (((no)<<3)+sel)

enum Cp0Reg {
    INDEX = COPROC_REGNUM(0,0),
    BAR = COPROC_REGNUM(8,0),
    COUNT = COPROC_REGNUM(9,0),
    STATUS = COPROC_REGNUM(12,0),
    CAUSE = COPROC_REGNUM(13,0),
    EPC = COPROC_REGNUM(14,0),
    IDENT = COPROC_REGNUM(15,1),
    CONFIG_1 = COPROC_REGNUM(16,1),

    // Implementation dependant,
    // count of non-frozen cycles
    EXEC_CYCLES = COPROC_REGNUM(9,6),
};

namespace {
// Avoid duplication of source code, this kind of op
// is easy to bug, and should be easy to debug 
static inline uint32_t sll( uint32_t reg, uint32_t sh )
{
    return reg << sh;
}
static inline uint32_t srl( uint32_t reg, uint32_t sh )
{
    return reg >> sh;
}
static inline uint32_t sra( uint32_t reg, uint32_t sh )
{
    if ( (int32_t)reg < 0 )
        return (reg >> sh) | (~((1<<(32-sh))-1));
    else
        return reg >> sh;
}
}

void MipsIss::do_load( uint32_t address, enum DataAccessType type, bool unsigned_, int shift )
{
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    switch (type) {
    case READ_BYTE:
    case LINE_INVAL:
        break;
    case READ_HALF:
        if ( address & 1 ) {
            m_exception = X_ADEL;
            return;
        }
        break;
    default:
        if ( address & 3 ) {
            m_exception = X_ADEL;
            return;
        }
        break;
    }
    r_mem_req = true;
    r_mem_type = type;
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;
    r_mem_unsigned = unsigned_;
    r_mem_shift = shift;
#if MIPS_DEBUG
    std::cout
        << m_name << std::hex
        << " load @" << address
        << " (" << dataAccessTypeName(type) << ")"
        << " -> r" << std::dec << m_ins.i.rt
        << std::endl;
#endif    
}

void MipsIss::do_store( uint32_t address, enum DataAccessType type, uint32_t data )
{
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADES;
        return;
    }
    switch (type) {
    case WRITE_BYTE:
        break;
    case WRITE_HALF:
        if ( address & 1 ) {
            m_exception = X_ADEL;
            return;
        }
        break;
    default:
        if ( address & 3 ) {
            m_exception = X_ADEL;
            return;
        }
        break;
    }
    r_mem_req = true;
    r_mem_type = type;
    r_mem_addr = address;
    r_mem_wdata = data;
#if MIPS_DEBUG
    std::cout
        << m_name << std::hex
        << " store @" << address
        << ": " << data
        << " (" << dataAccessTypeName(type) << ")"
        << std::endl;
#endif    
}

void MipsIss::op_bcond()
{
    bool taken;

    taken = (int32_t)m_rs < 0;
    taken ^= (bool)(m_ins.i.rt & 1);

    // and link ?
    if (m_ins.i.rt & 0x10)
        r_gp[31] = r_pc+8;

    if (taken) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

uint32_t MipsIss::cp0Get( uint32_t reg, uint32_t sel ) const
{
    switch(COPROC_REGNUM(reg,sel)) {
    case INDEX:
        return m_ident;
    case BAR:
        return r_bar;
    case COUNT:
        return r_count;
    case STATUS:
        return r_status.whole;
    case CAUSE:
        return r_cause.whole;
    case EPC:
        return r_epc;
    case IDENT:
        return 0x80000000|m_ident;
    case EXEC_CYCLES:
        return m_exec_cycles;
    case CONFIG_1:
        return 0x80000000
            | ((uint32_log2(m_icache_line_size)-1)<<19)
            | ((uint32_log2(m_dcache_line_size)-1)<<10);
    default:
        return 0;
    }
}

void MipsIss::cp0Set( uint32_t reg, uint32_t sel, uint32_t val )
{
    switch(COPROC_REGNUM(reg, sel)) {
    case STATUS:
        r_status.whole = val;
        return;
    default:
        return;
    }
}

// **Start**

void MipsIss::op_j()
{
    m_next_pc = (r_pc&0xf0000000) | (m_ins.j.imd * 4);
}

void MipsIss::op_jal()
{
    r_gp[31] = r_pc+8;
    m_next_pc = (r_pc&0xf0000000) | (m_ins.j.imd * 4);
}

void MipsIss::op_beq()
{
    if ( m_rs == m_rt ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

void MipsIss::op_bne()
{
    if ( m_rs != m_rt ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

void MipsIss::op_blez()
{
    if ( (int32_t)m_rs <= 0 ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

void MipsIss::op_bgtz()
{
    if ( (int32_t)m_rs > 0 ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

void MipsIss::op_addi()
{
    uint64_t tmp = (uint64_t)m_rs + (uint64_t)sign_ext16(m_ins.i.imd);
    if ( overflow( m_rs, sign_ext16(m_ins.i.imd), 0 ) )
        m_exception = X_OV;
    else
        r_gp[m_ins.i.rt] = tmp;
}

void MipsIss::op_addiu()
{
    r_gp[m_ins.i.rt] = m_rs + sign_ext16(m_ins.i.imd);
}

void MipsIss::op_slti()
{
    r_gp[m_ins.i.rt] = (bool)
        ((int32_t)m_rs < sign_ext16(m_ins.i.imd));
}

void MipsIss::op_sltiu()
{
    r_gp[m_ins.i.rt] = (bool)
        ((uint32_t)m_rs < (uint32_t)sign_ext16(m_ins.i.imd));
}

void MipsIss::op_andi()
{
    r_gp[m_ins.i.rt] = m_rs & m_ins.i.imd;
}

void MipsIss::op_ori()
{
    r_gp[m_ins.i.rt] = m_rs | m_ins.i.imd;
}

void MipsIss::op_xori()
{
    r_gp[m_ins.i.rt] = m_rs ^ m_ins.i.imd;
}

void MipsIss::op_lui()
{
    r_gp[m_ins.i.rt] = m_ins.i.imd << 16;
}

enum {
    MFC0 = 0,
    MFC1 = 1,
    MFC2 = 2,
    MTC0 = 4,
    MTC1 = 5,
    MTC2 = 6,
    RFE = 16,
};

void MipsIss::op_copro()
{
    if (isInUserMode()) {
        m_exception = X_CPU;
        return;
    }
    switch (m_ins.coproc.action) {
    case MTC0:
        cp0Set( m_ins.coproc.rd, m_ins.coproc.sel, m_rt );
        break;
    case MFC0:
        r_gp[m_ins.coproc.rt] = cp0Get( m_ins.coproc.rd, m_ins.coproc.sel );
        break;
    case RFE:
        r_status.kuc = r_status.kup;
        r_status.iec = r_status.iep;
        r_status.kup = r_status.kuo;
        r_status.iep = r_status.ieo;
        break;
    default: // Not handled, so raise an exception
        op_ill();
    }
}

void MipsIss::op_ill()
{
    m_exception = X_RI;
}

void MipsIss::op_lb()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_load( address, READ_BYTE, false);
}

void MipsIss::op_ll()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_load( address, READ_LINKED, false);
}

void MipsIss::op_lh()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_load( address, READ_HALF, false);
}

void MipsIss::op_lw()
{
    if ( m_ins.i.rt ) {
        uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
        do_load( address, READ_WORD, false);
    } else {
        uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
        SOCLIB_WARNING(
            "If you intend to flush cache reading to $0,\n"
            "this is a hack, go get a processor aware of caches");
        do_load( address, LINE_INVAL, false);
    }
}

void MipsIss::op_lwl()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    uint32_t w = address&3;
    do_load( address&~3, READ_WORD, false,
             m_little_endian
             ? (3-w)
             : w
             );
}

void MipsIss::op_lwr()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    uint32_t w = address&3;
    do_load( address&~3, READ_WORD, false,
             m_little_endian
             ? -w
             : -(3-w)
             );
}

#define CACHE_OP(what, cache) ((what)<<2+(cache))
enum {
    ICACHE,
    DCACHE,
    TCACHE,
    SCACHE,
};

enum {
    INDEX_INVAL,
    LOAD_TAG,
    STORE_TAG,
    DEP,
    HIT_INVAL,
    FILL,
    HIT_WB,
    FETCH_AND_LOCK,
};

void MipsIss::op_cache()
{
    switch (m_ins.i.rt) {
    case CACHE_OP(HIT_INVAL,DCACHE): {
        uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
        do_load( address, LINE_INVAL, false);
    }
    }
}

void MipsIss::op_lbu()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_load( address, READ_BYTE, true);
}

void MipsIss::op_lhu()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_load( address, READ_HALF, true);
}

void MipsIss::op_sb()
{
    uint32_t tmp = m_rt&0xff;
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_store(address, WRITE_BYTE, tmp|(tmp << 8)|(tmp << 16)|(tmp << 24));
}

void MipsIss::op_sc()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_store(address, STORE_COND, m_rt);
    r_mem_dest = m_ins.i.rt;
}

void MipsIss::op_sh()
{
    uint32_t tmp = m_rt&0xffff;
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_store(address, WRITE_HALF, tmp|(tmp << 16));
}

void MipsIss::op_sw()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    do_store(address, WRITE_WORD, m_rt);
}

void MipsIss::op_swl()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if ( (address & 3) == (m_little_endian ? 3 : 0) ) {
        op_sw();
        return;
    }
    std::cout << name() << " Unimplemented opcod swl for address=" << address << " and le=" << m_little_endian << "!" << std::endl;
    m_exception = X_RI;
}

void MipsIss::op_swr()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if ( (address & 3) == (m_little_endian ? 0 : 3) ) {
        op_sw();
        return;
    }
    std::cout << name() << " Unimplemented opcod swr for address=" << address << " and le=" << m_little_endian << "!" << std::endl;
    m_exception = X_RI;
}

void MipsIss::special_sll()
{
    r_gp[m_ins.r.rd] = sll(m_rt, m_ins.r.sh);
}

void MipsIss::special_srl()
{
    r_gp[m_ins.r.rd] = srl(m_rt, m_ins.r.sh);
}

void MipsIss::special_sra()
{
    r_gp[m_ins.r.rd] = sra(m_rt, m_ins.r.sh);
}

void MipsIss::special_sllv()
{
    r_gp[m_ins.r.rd] = sll(m_rt, m_rs&0x1f );
}

void MipsIss::special_srlv()
{
    r_gp[m_ins.r.rd] = srl(m_rt, m_rs&0x1f );
}

void MipsIss::special_srav()
{
    r_gp[m_ins.r.rd] = sra(m_rt, m_rs&0x1f );
}

void MipsIss::special_jr()
{
    if (isPrivDataAddr(m_rs) && isInUserMode()) {
        m_exception = X_ADEL;
        return;
    }
    m_next_pc = m_rs;
}

void MipsIss::special_jalr()
{
    if (isPrivDataAddr(m_rs) && isInUserMode()) {
        m_exception = X_ADEL;
        return;
    }
    r_gp[m_ins.r.rd] = r_pc+8;
    m_next_pc = m_rs;
}

void MipsIss::special_sysc()
{
    m_exception = X_SYS;
}

void MipsIss::special_brek()
{
    m_exception = X_BP;
}

void MipsIss::special_mfhi()
{
    r_gp[m_ins.r.rd] = r_hi;
}

void MipsIss::special_movn()
{
    if ( m_rt != 0 )
        r_gp[m_ins.r.rd] = m_rs;
}

void MipsIss::special_movz()
{
    if ( m_rt == 0 )
        r_gp[m_ins.r.rd] = m_rs;
}

void MipsIss::special_mthi()
{
    r_hi = m_rs;
}

void MipsIss::special_mflo()
{
    r_gp[m_ins.r.rd] = r_lo;
}

void MipsIss::special_mtlo()
{
    r_lo = m_rs;
}

void MipsIss::special_mult()
{
    int64_t a = (int32_t)m_rs;
    int64_t b = (int32_t)m_rt;
    int64_t res = a*b;
    r_hi = res>>32;
    r_lo = res;
    if (m_rt)
        setInsDelay( 3 );
}

void MipsIss::special_multu()
{
    uint64_t a = m_rs;
    uint64_t b = m_rt;
    uint64_t res = a*b;
    r_hi = res>>32;
    r_lo = res;
    if (m_rt)
        setInsDelay( 3 );
}

void MipsIss::special_div()
{
    if ( ! m_rt ) {
        r_hi = random();
        r_lo = random();
        return;
    }
    r_hi = (int32_t)m_rs % (int32_t)m_rt;
    r_lo = (int32_t)m_rs / (int32_t)m_rt;
    if (m_rt)
        setInsDelay( __builtin_clz(m_rt)+1 );
}

void MipsIss::special_divu()
{
    if ( ! m_rt ) {
        r_hi = random();
        r_lo = random();
        return;
    }
    r_hi = m_rs % m_rt;
    r_lo = m_rs / m_rt;
    if (m_rt)
        setInsDelay( __builtin_clz(m_rt)+1 );
}

void MipsIss::special_add()
{
    uint64_t tmp = (uint64_t)m_rs + (uint64_t)m_rt;
    if ( overflow( m_rs, m_rt, 0 ) )
        m_exception = X_OV;
    else
        r_gp[m_ins.r.rd] = tmp;
}

void MipsIss::special_addu()
{
    r_gp[m_ins.r.rd] = m_rs + m_rt;
}

void MipsIss::special_sub()
{
    uint64_t tmp = (uint64_t)m_rs - (uint64_t)m_rt;
    if ( overflow( ~m_rt, m_rs, 1 ) )
        m_exception = X_OV;
    else
        r_gp[m_ins.r.rd] = tmp;
}

void MipsIss::special_subu()
{
    r_gp[m_ins.r.rd] = m_rs - m_rt;
}

void MipsIss::special_and()
{
    r_gp[m_ins.r.rd] = m_rs & m_rt;
}

void MipsIss::special_or()
{
    r_gp[m_ins.r.rd] = m_rs | m_rt;
}

void MipsIss::special_xor()
{
    r_gp[m_ins.r.rd] = m_rs ^ m_rt;
}

void MipsIss::special_nor()
{
    r_gp[m_ins.r.rd] = ~(m_rs | m_rt);
}

void MipsIss::special_slt()
{
    r_gp[m_ins.r.rd] = (bool)((int32_t)m_rs < (int32_t)m_rt);
}

void MipsIss::special_sltu()
{
    r_gp[m_ins.r.rd] = (bool)(m_rs < m_rt);
}

void MipsIss::special_tlt()
{
    if ((int32_t)m_rs < (int32_t)m_rt)
        m_exception = X_TR;
}

void MipsIss::special_tltu()
{
    if (m_rs < m_rt)
        m_exception = X_TR;
}

void MipsIss::special_tge()
{
    if ((int32_t)m_rs >= (int32_t)m_rt)
        m_exception = X_TR;
}

void MipsIss::special_tgeu()
{
    if (m_rs >= m_rt)
        m_exception = X_TR;
}

void MipsIss::special_teq()
{
    if (m_rs == m_rt)
        m_exception = X_TR;
}

void MipsIss::special_tne()
{
    if (m_rs != m_rt)
        m_exception = X_TR;
}

void MipsIss::special_ill()
{
    m_exception = X_RI;
}

#define op(x) &MipsIss::special_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)

MipsIss::func_t const MipsIss::special_table[] = {
        op4(  sll,  ill,  srl,  sra),
        op4( sllv,  ill, srlv, srav),

        op4(   jr, jalr, movz, movn),
        op4( sysc, brek,  ill,  ill),

        op4( mfhi, mthi, mflo, mtlo),
        op4(  ill,  ill,  ill,  ill),

        op4( mult,multu,  div, divu),
        op4(  ill,  ill,  ill,  ill),

        op4(  add, addu,  sub, subu),
        op4(  and,   or,  xor,  nor),

        op4(  ill,  ill,  slt, sltu),
        op4(  ill,  ill,  ill,  ill),

        op4(  tge, tgeu,  tlt, tltu),
        op4(  teq,  ill,  tne,  ill),

        op4(  ill,  ill,  ill,  ill),
        op4(  ill,  ill,  ill,  ill),
};

#undef op
#undef op4

void MipsIss::op_special2()
{
    switch ( m_ins.r.func ) {
    case 2: // MUL
        r_gp[m_ins.r.rd] = m_rs*m_rt;
        if (m_rt)
            setInsDelay( 3 );
        break;
    default:
        op_ill();
    }
}

void MipsIss::op_special()
{
    func_t func = special_table[m_ins.r.func];
    (this->*func)();
}

#define op(x) &MipsIss::op_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)

MipsIss::func_t const MipsIss::opcod_table[]= {
    op4(special, bcond,    j,   jal),
    op4(    beq,   bne, blez,  bgtz),

    op4(   addi, addiu, slti, sltiu),
    op4(   andi,   ori, xori,   lui),

    op4(  copro,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(special2,  ill,  ill,   ill),

    op4(     lb,    lh,  lwl,    lw),
    op4(    lbu,   lhu,  lwr,   ill),

    op4(     sb,    sh,  swl,    sw),
    op4(    ill,   ill,  swr, cache),

    op4(     ll,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     sc,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};

#undef op
#define op(x) #x

const char *MipsIss::name_table[] = {
    op4(special, bcond,    j,   jal),
    op4(    beq,   bne, blez,  bgtz),

    op4(   addi, addiu, slti, sltiu),
    op4(   andi,   ori, xori,   lui),

    op4(  copro,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(special2,  ill,  ill,   ill),

    op4(     lb,    lh,  lwl,    lw),
    op4(    lbu,   lhu,  lwr,   ill),

    op4(     sb,    sh,  swl,    sw),
    op4(    ill,   ill,  swr, cache),

    op4(     ll,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     sc,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};
#undef op
#undef op4

void MipsIss::run()
{
    func_t func = opcod_table[m_ins.i.op];
    m_rs = r_gp[m_ins.r.rs];
    m_rt = r_gp[m_ins.r.rt];

    if (isHighPC() && isInUserMode()) {
        m_exception = X_SYS;
        return;
    }

    (this->*func)();
}

#define use(x) MipsIss::USE_##x
#define use4(x, y, z, t) use(x), use(y), use(z), use(t)

MipsIss::use_t const MipsIss::use_table[]= {
       use4(SPECIAL,    ST, NONE,  NONE),
       use4(     ST,    ST,    S,     S),

       use4(      S,     S,    S,     S),
       use4(      S,     S,    S,  NONE),

       use4(     ST,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(   NONE,  NONE, NONE,  NONE),
       use4(     ST,  NONE, NONE,  NONE),

       use4(      S,     S,    S,     S),
       use4(      S,     S,    S,  NONE),

       use4(     ST,    ST,   ST,    ST),
       use4(   NONE,  NONE,   ST,    ST),

       use4(      S,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(     ST,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),
};

MipsIss::use_t const MipsIss::use_special_table[] = {
        use4(    T, NONE,    T,    T),
        use4(    T, NONE,    T,    T),

        use4(    S,    S, NONE, NONE),
        use4( NONE, NONE, NONE, NONE),

        use4( NONE,    S, NONE,    S),
        use4( NONE, NONE, NONE, NONE),

        use4(   ST,   ST,   ST,   ST),
        use4( NONE, NONE, NONE, NONE),

        use4(   ST,   ST,   ST,   ST),
        use4(   ST,   ST,   ST,   ST),

        use4( NONE, NONE,   ST,   ST),
        use4( NONE, NONE, NONE, NONE),

        use4( NONE, NONE, NONE, NONE),
        use4( NONE, NONE, NONE, NONE),

        use4( NONE, NONE, NONE, NONE),
        use4( NONE, NONE, NONE, NONE),
};

MipsIss::use_t MipsIss::curInstructionUsesRegs()
{
    use_t use = use_table[m_ins.i.op];
    if ( use == USE_SPECIAL )
        return use_special_table[m_ins.r.func];
    return use;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
