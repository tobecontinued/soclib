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

#include "common/iss/mips.h"
#include "common/base_module.h"
#include "common/arithmetics.h"

namespace soclib { namespace common {

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

template <bool little_endian>
void MipsMetaIss<little_endian>::do_load( enum DataAccessType type, bool unsigned_ )
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    r_mem_req = true;
    r_mem_type = type;
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;
    r_mem_unsigned = unsigned_;
#if MIPS_DEBUG
    std::cout
        << m_name << std::hex
        << " load @" << address
        << " (" << dataAccessTypeName(type) << ")"
        << " -> r" << std::dec << m_ins.i.rt
        << std::endl;
#endif    
}

template <bool little_endian>
void MipsMetaIss<little_endian>::do_store( enum DataAccessType type, uint32_t data )
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADES;
        return;
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

template <bool little_endian>
void MipsMetaIss<little_endian>::op_bcond()
{
    bool taken;

    taken = (int32_t)m_rs < 0;
    taken ^= (bool)(m_ins.i.rt & 1);

    // and link ?
    if (m_ins.i.rt & 0x20)
        r_gp[31] = r_pc+8;

    if (taken) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

template <bool little_endian>
uint32_t MipsMetaIss<little_endian>::cp0Get( uint32_t reg ) const
{
    switch(reg) {
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
    default:
        return 0;
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::cp0Set( uint32_t reg, uint32_t val )
{
    switch(reg) {
    case STATUS:
        r_status.whole = val;
        return;
    default:
        return;
    }
}

// **Start**

template <bool little_endian>
void MipsMetaIss<little_endian>::op_j()
{
    m_next_pc = (r_pc&0xf0000000) | (m_ins.j.imd * 4);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_jal()
{
    r_gp[31] = r_pc+8;
    m_next_pc = (r_pc&0xf0000000) | (m_ins.j.imd * 4);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_beq()
{
    if ( m_rs == m_rt ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_bne()
{
    if ( m_rs != m_rt ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_blez()
{
    if ( (int32_t)m_rs <= 0 ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_bgtz()
{
    if ( (int32_t)m_rs > 0 ) {
        m_next_pc = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_addi()
{
    uint64_t tmp = (uint64_t)m_rs + (uint64_t)sign_ext16(m_ins.i.imd);
    if ( overflow( m_rs, sign_ext16(m_ins.i.imd), 0 ) )
        m_exception = X_OV;
    else
        r_gp[m_ins.i.rt] = tmp;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_addiu()
{
    r_gp[m_ins.i.rt] = m_rs + sign_ext16(m_ins.i.imd);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_slti()
{
    r_gp[m_ins.i.rt] = (bool)
        ((int32_t)m_rs < sign_ext16(m_ins.i.imd));
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_sltiu()
{
    r_gp[m_ins.i.rt] = (bool)
        ((uint32_t)m_rs < (uint32_t)sign_ext16(m_ins.i.imd));
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_andi()
{
    r_gp[m_ins.i.rt] = m_rs & m_ins.i.imd;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_ori()
{
    r_gp[m_ins.i.rt] = m_rs | m_ins.i.imd;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_xori()
{
    r_gp[m_ins.i.rt] = m_rs ^ m_ins.i.imd;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_lui()
{
    r_gp[m_ins.i.rt] = m_ins.i.imd << 16;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_copro()
{
    if (isInUserMode()) {
        m_exception = X_CPU;
        return;
    }
    switch (m_ins.r.rs) {
    case 4: // mtc0
        cp0Set( m_ins.r.rd, m_rt );
        break;
    case 0: // mfc0
        r_gp[m_ins.r.rt] = cp0Get( m_ins.r.rd );
        break;
    case 16: // rfe
        r_status.kuc = r_status.kup;
        r_status.iec = r_status.iep;
        r_status.kup = r_status.kuo;
        r_status.iep = r_status.ieo;
        break;
    default: // Not handled, so raise an exception
        op_ill();
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_ill()
{
    m_exception = X_RI;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_lb()
{
    do_load(READ_BYTE, false);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_ll()
{
    do_load(READ_LINKED, false);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_lh()
{
    do_load(READ_HALF, false);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_lw()
{
    if ( m_ins.i.rt )
        do_load(READ_WORD, false);
    else {
        SOCLIB_WARNING(
            "If you intend to flush cache reading to $0,\n"
            "this is a hack, go get a processor aware of caches");
        do_load(LINE_INVAL, false);
    }
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_lbu()
{
    do_load(READ_BYTE, true);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_lhu()
{
    do_load(READ_HALF, true);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_sb()
{
    uint32_t tmp = m_rt&0xff;
    do_store(WRITE_BYTE, tmp|(tmp << 8)|(tmp << 16)|(tmp << 24));
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_sh()
{
    uint32_t tmp = m_rt&0xffff;
    do_store(WRITE_HALF, tmp|(tmp << 16));
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_sw()
{
    do_store(WRITE_WORD, m_rt);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::op_sc()
{
    do_store(STORE_COND, m_rt);
    r_mem_dest = m_ins.i.rt;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_sll()
{
    r_gp[m_ins.r.rd] = sll(m_rt, m_ins.r.sh);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_srl()
{
    r_gp[m_ins.r.rd] = srl(m_rt, m_ins.r.sh);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_sra()
{
    r_gp[m_ins.r.rd] = sra(m_rt, m_ins.r.sh);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_sllv()
{
    r_gp[m_ins.r.rd] = sll(m_rt, m_rs&0x1f );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_srlv()
{
    r_gp[m_ins.r.rd] = srl(m_rt, m_rs&0x1f );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_srav()
{
    r_gp[m_ins.r.rd] = sra(m_rt, m_rs&0x1f );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_jr()
{
    if (isPrivDataAddr(m_rs) && isInUserMode()) {
        m_exception = X_ADEL;
        return;
    }
    m_next_pc = m_rs;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_jalr()
{
    if (isPrivDataAddr(m_rs) && isInUserMode()) {
        m_exception = X_ADEL;
        return;
    }
    r_gp[m_ins.r.rd] = r_pc+8;
    m_next_pc = m_rs;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_sysc()
{
    m_exception = X_SYS;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_brek()
{
    m_exception = X_BP;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_mfhi()
{
    r_gp[m_ins.r.rd] = r_hi;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_mthi()
{
    r_hi = m_rs;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_mflo()
{
    r_gp[m_ins.r.rd] = r_lo;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_mtlo()
{
    r_lo = m_rs;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_mult()
{
    int64_t a = (int32_t)m_rs;
    int64_t b = (int32_t)m_rt;
    int64_t res = a*b;
    r_hi = res>>32;
    r_lo = res;
    setInsDelay( 6 );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_multu()
{
    uint64_t a = m_rs;
    uint64_t b = m_rt;
    uint64_t res = a*b;
    r_hi = res>>32;
    r_lo = res;
    setInsDelay( 6 );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_div()
{
    if ( ! m_rt ) {
        r_hi = random();
        r_lo = random();
        return;
    }
    r_hi = (int32_t)m_rs % (int32_t)m_rt;
    r_lo = (int32_t)m_rs / (int32_t)m_rt;
    setInsDelay( 31 );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_divu()
{
    if ( ! m_rt ) {
        r_hi = random();
        r_lo = random();
        return;
    }
    r_hi = m_rs % m_rt;
    r_lo = m_rs / m_rt;
    setInsDelay( 31 );
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_add()
{
    uint64_t tmp = (uint64_t)m_rs + (uint64_t)m_rt;
    if ( overflow( m_rs, m_rt, 0 ) )
        m_exception = X_OV;
    else
        r_gp[m_ins.r.rd] = tmp;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_addu()
{
    r_gp[m_ins.r.rd] = m_rs + m_rt;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_sub()
{
    uint64_t tmp = (uint64_t)m_rs - (uint64_t)m_rt;
    if ( overflow( ~m_rt, m_rs, 1 ) )
        m_exception = X_OV;
    else
        r_gp[m_ins.r.rd] = tmp;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_subu()
{
    r_gp[m_ins.r.rd] = m_rs - m_rt;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_and()
{
    r_gp[m_ins.r.rd] = m_rs & m_rt;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_or()
{
    r_gp[m_ins.r.rd] = m_rs | m_rt;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_xor()
{
    r_gp[m_ins.r.rd] = m_rs ^ m_rt;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_nor()
{
    r_gp[m_ins.r.rd] = ~(m_rs | m_rt);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_slt()
{
    r_gp[m_ins.r.rd] = (bool)((int32_t)m_rs < (int32_t)m_rt);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_sltu()
{
    r_gp[m_ins.r.rd] = (bool)(m_rs < m_rt);
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_tlt()
{
    if ((int32_t)m_rs < (int32_t)m_rt)
        m_exception = X_TR;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_tltu()
{
    if (m_rs < m_rt)
        m_exception = X_TR;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_tge()
{
    if ((int32_t)m_rs >= (int32_t)m_rt)
        m_exception = X_TR;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_tgeu()
{
    if (m_rs >= m_rt)
        m_exception = X_TR;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_teq()
{
    if (m_rs == m_rt)
        m_exception = X_TR;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_tne()
{
    if (m_rs != m_rt)
        m_exception = X_TR;
}

template <bool little_endian>
void MipsMetaIss<little_endian>::special_ill()
{
    m_exception = X_RI;
}

#define op(x) &MipsMetaIss<little_endian>::special_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)

template <bool little_endian>
typename MipsMetaIss<little_endian>::func_t const MipsMetaIss<little_endian>::special_table[] = {
        op4(  sll,  ill,  srl,  sra),
        op4( sllv,  ill, srlv, srav),

        op4(   jr, jalr,  ill,  ill),
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

template <bool little_endian>
void MipsMetaIss<little_endian>::op_special()
{
    func_t func = special_table[m_ins.r.func];
    (this->*func)();
}

#define op(x) &MipsMetaIss<little_endian>::op_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)

template <bool little_endian>
typename MipsMetaIss<little_endian>::func_t const MipsMetaIss<little_endian>::opcod_table[]= {
    op4(special, bcond,    j,   jal),
    op4(    beq,   bne, blez,  bgtz),

    op4(   addi, addiu, slti, sltiu),
    op4(   andi,   ori, xori,   lui),

    op4(  copro,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     lb,    lh,  ill,    lw),
    op4(    lbu,   lhu,  ill,   ill),

    op4(     sb,    sh,  ill,    sw),
    op4(    ill,   ill,  ill,   ill),

    op4(     ll,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     sc,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};

#undef op
#define op(x) #x

template <bool little_endian>
const char *MipsMetaIss<little_endian>::name_table[] = {
    op4(special, bcond,    j,   jal),
    op4(    beq,   bne, blez,  bgtz),

    op4(   addi, addiu, slti, sltiu),
    op4(   andi,   ori, xori,   lui),

    op4(  copro,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     lb,    lh,  ill,    lw),
    op4(    lbu,   lhu,  ill,   ill),

    op4(     sb,    sh,  ill,    sw),
    op4(    ill,   ill,  ill,   ill),

    op4(     ll,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(     sc,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};
#undef op
#undef op4

template <bool little_endian>
void MipsMetaIss<little_endian>::run()
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

#define use(x) MipsMetaIss<little_endian>::USE_##x
#define use4(x, y, z, t) use(x), use(y), use(z), use(t)

template <bool little_endian>
typename MipsMetaIss<little_endian>::use_t const MipsMetaIss<little_endian>::use_table[]= {
       use4(SPECIAL,    ST, NONE,  NONE),
       use4(     ST,    ST,    S,     S),

       use4(      S,     S,    S,     S),
       use4(      S,     S,    S,  NONE),

       use4(     ST,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(   NONE,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(      S,     S, NONE,     S),
       use4(      S,     S, NONE,  NONE),

       use4(     ST,    ST, NONE,    ST),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(      S,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(     ST,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),
};

template <bool little_endian>
typename MipsMetaIss<little_endian>::use_t const MipsMetaIss<little_endian>::use_special_table[] = {
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

template <bool little_endian>
typename MipsMetaIss<little_endian>::use_t MipsMetaIss<little_endian>::curInstructionUsesRegs()
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
