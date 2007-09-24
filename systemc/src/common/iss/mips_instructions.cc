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
 * - 2007-09-19
 *   Nicolas Pouillon: Fix overflow
 *
 * - 2007-06-15
 *   Nicolas Pouillon, Alain Greiner: Model created
 */

#include "common/iss/mips.h"

namespace {

static inline bool overflow( uint32_t a, uint32_t b, uint32_t c )
{
    return ((b^(a+b+c))&~(a^b))>>31;
}

}

namespace soclib { namespace common {

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
    op4(    ill,   ill,  ill,   ill),

    op4(     lb,    lh,  ill,    lw),
    op4(    lbu,   lhu,  ill,   ill),

    op4(     sb,    sh,  ill,    sw),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};

#undef op
#undef op4

#if MIPS_DEBUG
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)
#define op(x) #x

static const char *name_table[] = {
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

    op4(    ill,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),

    op4(    ill,   ill,  ill,   ill),
    op4(    ill,   ill,  ill,   ill),
};
#undef op
#undef op4
#endif

void MipsIss::run()
{
    func_t func = opcod_table[m_ins.i.op];
    m_rs = r_gp[m_ins.r.rs];
    m_rt = r_gp[m_ins.r.rt];
    m_branch_taken = false;

#if MIPS_DEBUG
    std::cout
        << std::hex << std::showbase
        << m_name << std::endl
        << " PC: " << r_pc
        << " Ins: " << m_ins.ins << std::endl
        << " op:  " << m_ins.i.op << " (" << name_table[m_ins.i.op] << ")" << std::endl
        << std::dec
        << " i rs: " << m_ins.i.rs
        << " rt: "<<m_ins.i.rt
        << " i: "<<std::hex << m_ins.i.imd
        << std::endl << std::dec
        << " r rs: " << m_ins.r.rs
        << " rt: "<<m_ins.r.rt
        << " rd: "<<m_ins.r.rd
        << " sh: "<<m_ins.r.sh << std::hex
        << " func: "<<m_ins.r.func
        << std::endl
        << " V rs: " << m_rs
        << " rt: "<<m_rt
        << std::endl;
#endif

    if (isHighPC() && isInUserMode()) {
        m_exception = X_SYS;
        return;
    }

    // We write the default value in this register
    // It will be overwritten by load & store instructions
    r_mem_type = MEM_NONE;
    (this->*func)();
}

static inline int32_t sign_ext16(uint16_t i)
{
    return (int16_t)i;
}

void MipsIss::op_bcond()
{
    bool taken;

    taken = (int32_t)m_rs < 0;
    taken ^= (bool)(m_ins.i.rt & 1);

    // and link ?
    if (m_ins.i.rt & 0x20)
        r_gp[31] = r_pc+8;

    if (taken) {
        m_branch_address = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
        m_branch_taken = true;
    }
}

void MipsIss::op_j()
{
    m_branch_address = (r_pc&0xf0000000) | (m_ins.j.imd * 4);
    m_branch_taken = true;
}

void MipsIss::op_jal()
{
    r_gp[31] = r_pc+8;
    m_branch_address = (r_pc&0xf0000000) | (m_ins.j.imd * 4);
    m_branch_taken = true;
}

void MipsIss::op_beq()
{
    if ( m_rs == m_rt ) {
        m_branch_address = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
        m_branch_taken = true;
    }
}

void MipsIss::op_bne()
{
    if ( m_rs != m_rt ) {
        m_branch_address = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
        m_branch_taken = true;
    }
}

void MipsIss::op_blez()
{
    if ( (int32_t)m_rs <= 0 ) {
        m_branch_address = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
        m_branch_taken = true;
    }
}

void MipsIss::op_bgtz()
{
    if ( (int32_t)m_rs > 0 ) {
        m_branch_address = sign_ext16(m_ins.i.imd)*4 + r_pc + 4;
        m_branch_taken = true;
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

void MipsIss::op_copro()
{
    if (isInUserMode()) {
        m_exception = X_CPU;
        return;
    }
    switch (m_ins.r.rs) {
    case 4: // mtc0
        r_cp0[m_ins.r.rd] = m_rt;
        break;
    case 0: // mfc0
        r_gp[m_ins.r.rt] = r_cp0[m_ins.r.rd];
        break;
    case 16: // rfe
        r_cp0[STATUS] = (r_cp0[STATUS]&(~0xf)) | ((r_cp0[STATUS]>>2)&0xf);
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
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    r_mem_type = MEM_LB;
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;

}

void MipsIss::op_lh()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    r_mem_type = MEM_LH;
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;
}

void MipsIss::op_lw()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    if ( m_ins.i.rt )
        r_mem_type = MEM_LW;
    else {
        static bool warned = false;
        if ( !warned ) {
            std::cout
                << "If you intend to flush cache reading to $0," << std::endl
                << "this is a hack, go get a processor aware of caches" << std::endl;
            warned = true;
        }
        r_mem_type = MEM_INVAL;
    }
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;
}

void MipsIss::op_lbu()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    r_mem_type = MEM_LBU;
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;
}

void MipsIss::op_lhu()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADEL;
        return;
    }
    r_mem_type = MEM_LHU;
    r_mem_addr = address;
    r_mem_dest = m_ins.i.rt;
}

void MipsIss::op_sb()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADES;
        return;
    }
    uint8_t tmp = m_rt;
    r_mem_type = MEM_SB;
    r_mem_addr = address;
    r_mem_wdata = tmp|(tmp << 8)|(tmp << 16)|(tmp << 24);
}

void MipsIss::op_sh()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADES;
        return;
    }
    uint16_t tmp = m_rt;
    r_mem_type = MEM_SH;
    r_mem_addr = address;
    r_mem_wdata = tmp | (tmp << 16);
}

void MipsIss::op_sw()
{
    uint32_t address =  m_rs + sign_ext16(m_ins.i.imd);
    if (isInUserMode() && isPrivDataAddr(address)) {
        m_exception = X_ADES;
        return;
    }
    r_mem_type = MEM_SW;
    r_mem_addr = address;
    r_mem_wdata = m_rt;
}

// Avoid duplication of source code, this kind of op
// is easy to bug, and should be easy to debug 
static inline uint32_t sll( uint32_t reg, uint32_t sh )
{
    return reg << sh;
}
static inline uint32_t srl( uint32_t reg, uint32_t sh )
{
    return (reg >> sh) & ((1<<(32-sh))-1);
}
static inline uint32_t sra( uint32_t reg, uint32_t sh )
{
    if ( (int32_t)reg < 0 )
        return (reg >> sh) | (~((1<<(32-sh))-1));
    else
        return (reg >> sh) & ((1<<(32-sh))-1);
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
    m_branch_address = m_rs;
    m_branch_taken = true;
}

void MipsIss::special_jalr()
{
    if (isPrivDataAddr(m_rs) && isInUserMode()) {
        m_exception = X_ADEL;
        return;
    }
    r_gp[m_ins.r.rd] = r_pc+8;
    m_branch_address = m_rs;
    m_branch_taken = true;
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
}

void MipsIss::special_multu()
{
    uint64_t a = m_rs;
    uint64_t b = m_rt;
    uint64_t res = a*b;
    r_hi = res>>32;
    r_lo = res;
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

void MipsIss::special_ill()
{
    m_exception = X_RI;
}

#define op(x) &MipsIss::special_##x
#define op4(x, y, z, t) op(x), op(y), op(z), op(t)

MipsIss::func_t const MipsIss::special_table[] = {
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

        op4(  ill,  ill,  ill,  ill),
        op4(  ill,  ill,  ill,  ill),

        op4(  ill,  ill,  ill,  ill),
        op4(  ill,  ill,  ill,  ill),
};

#undef op
#undef op4

void MipsIss::op_special()
{
    func_t func = special_table[m_ins.r.func];
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
       use4(   NONE,  NONE, NONE,  NONE),

       use4(      S,     S, NONE,     S),
       use4(      S,     S, NONE,  NONE),

       use4(     ST,    ST, NONE,    ST),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(   NONE,  NONE, NONE,  NONE),
       use4(   NONE,  NONE, NONE,  NONE),

       use4(   NONE,  NONE, NONE,  NONE),
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
