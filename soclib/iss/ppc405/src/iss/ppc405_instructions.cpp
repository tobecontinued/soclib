/* -*- c++ -*-
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
 *
 * Maintainers: nipo
 *
 * $Id$
 */

#include <stdint.h>
#include "base_module.h"
#include "ppc405.h"
#include "soclib_endian.h"
#include "arithmetics.h"

namespace soclib { namespace common {

namespace {

inline uint32_t mask_gen( uint32_t mb, uint32_t me )
{
	me = (me+1)%32;
	uint32_t ml = (uint32_t)-1>>me;
	uint32_t mr = (uint32_t)-1>>mb;
	uint32_t m = ml^mr;
	return (!me || mb>me) ? ~m  :m;
}
static inline uint32_t rotl( uint32_t data, uint32_t sh )
{
    sh &= 0x1f;
    return (data << sh) | (data >> (32-sh));
}

enum {
    BO0 = 0x10,
    BO1 = 0x8,
    BO2 = 0x4,
    BO3 = 0x2,
    BO4 = 0x1,
};

}

uint32_t Ppc405Iss::sprfGet( enum Sprf sprf )
{
    if ( r_msr.pr && sprf&SPR_PRIV_MASK ) {
        m_exception = EXCEPT_PROGRAM;
        r_esr = ESR_PPR;
        return 0;
    }
    switch (sprf) {
    case SPR_XER:
        return r_xer.whole;
    case SPR_LR:
        return r_lr;
    case SPR_CTR:
        return r_ctr;
    case SPR_SRR0:
    case SPR_SRR1:
        return r_srr[PPC_SPLIT_FIELD(sprf) - 0x01a /*SPR_SRR0*/];
    case SPR_USPRG0:
    case SPR_SPRG4_RO:
    case SPR_SPRG5_RO:
    case SPR_SPRG6_RO:
    case SPR_SPRG7_RO:
        return r_sprg[PPC_SPLIT_FIELD(sprf) - 0x100 /*SPR_USPRG0*/];
    case SPR_SPRG0_RW:
    case SPR_SPRG1_RW:
    case SPR_SPRG2_RW:
    case SPR_SPRG3_RW:
    case SPR_SPRG4_RW:
    case SPR_SPRG5_RW:
    case SPR_SPRG6_RW:
    case SPR_SPRG7_RW:
        return r_sprg[PPC_SPLIT_FIELD(sprf) - 0x110 /*SPR_SPRG0_RW*/];
    case SPR_TBL:
        return r_tb;
    case SPR_TBU:
        return r_tb>>32;
    case SPR_PVR:
        return pvr;
    case SPR_ESR:
        return r_esr;
    case SPR_DEAR:
        return r_dear;
    case SPR_EVPR:
        return r_evpr;
    case SPR_SRR2:
    case SPR_SRR3:
        return r_srr[PPC_SPLIT_FIELD(sprf) - 0x3de + 2 /*SPR_SRR2+2*/];
    default:
        m_exception = EXCEPT_PROGRAM;
        r_esr = ESR_PEU;
        return 0;
    }
}

void Ppc405Iss::sprfSet( enum Sprf sprf, uint32_t val )
{
    if ( r_msr.pr && sprf&SPR_PRIV_MASK ) {
        m_exception = EXCEPT_PROGRAM;
        r_esr = ESR_PPR;
        return;
    }
    switch (sprf) {
    case SPR_XER:
        r_xer.whole = val;
        break;
    case SPR_LR:
        r_lr = val;
        break;
    case SPR_CTR:
        r_ctr = val;
        break;
    case SPR_SRR0:
    case SPR_SRR1:
        r_srr[PPC_SPLIT_FIELD(sprf) - 0x1a /*SPR_SRR0*/] = val;
        break;
    case SPR_SPRG0_RW:
    case SPR_SPRG1_RW:
    case SPR_SPRG2_RW:
    case SPR_SPRG3_RW:
    case SPR_SPRG4_RW:
    case SPR_SPRG5_RW:
    case SPR_SPRG6_RW:
    case SPR_SPRG7_RW:
        r_sprg[PPC_SPLIT_FIELD(sprf) - 0x110 /*SPR_SPRG0_RW*/] = val;
        break;
    case SPR_TBL:
        r_tb = (r_tb & 0xffffffff00000000LL) | val;
        break;
    case SPR_TBU:
        r_tb = (r_tb & 0xffffffff) | ((uint64_t)val<<32);
        break;
    case SPR_ESR:
        r_esr = val;
        break;
    case SPR_DEAR:
        r_dear = val;
        break;
    case SPR_EVPR:
        r_evpr = val & 0xffff0000;
        break;
    case SPR_SRR2:
    case SPR_SRR3:
        r_srr[PPC_SPLIT_FIELD(sprf) - 0x3de + 2 /*SPR_SRR2+2*/] = val;
        break;
    default:
        m_exception = EXCEPT_PROGRAM;
        r_esr = ESR_PEU;
    }
}

void Ppc405Iss::trap( uint32_t to, uint32_t a, uint32_t b )
{
    if ( (to & TRAP_LT && (int32_t)a < (int32_t)b) ||
         (to & TRAP_GT && (int32_t)a > (int32_t)b) ||
         (to & TRAP_EQ && a == b) ||
         (to & TRAP_LTU && a < b) ||
         (to & TRAP_GTU && a > b) ) {
        m_exception = EXCEPT_DEBUG;
        r_esr = ESR_PTR;
    }
}

void Ppc405Iss::mem_load_imm( DataOperationType type, uint32_t nb, bool update, bool reversed, bool unsigned_ )
{
    uint32_t base = (m_ins.d.ra || update) ? r_gp[m_ins.d.ra] : 0;
    uint32_t address = base + sign_ext(m_ins.d.imm, 16);
    if ( update )
        r_gp[m_ins.d.ra] = address;
    if ( type != XTN_READ && (address % nb) != 0 )
        {
            m_exception = EXCEPT_ALIGNMENT;
            return;
        }
    m_dreq.valid = true;
    m_dreq.be = ((1<<nb)-1) << (address % 4);
    m_dreq.type = type;
    if ( type == XTN_READ )
        m_dreq.addr = nb*4;
    else
        m_dreq.addr = address;
    r_mem_dest = m_ins.d.rd;
    r_mem_reversed = reversed;
    r_mem_unsigned = unsigned_;
#if SOCLIB_MODULE_DEBUG
    std::cout
        << m_name << ": "
        << __FUNCTION__ << ": " << m_dreq
        << "->" << r_mem_dest << " Rev:" << r_mem_reversed << " U:" << r_mem_unsigned
        << std::endl;
#endif
}

void Ppc405Iss::mem_load_indexed( DataOperationType type, uint32_t nb, bool update, bool reversed, bool unsigned_ )
{
    uint32_t base = (m_ins.x.ra || update) ? r_gp[m_ins.x.ra] : 0;
    uint32_t address = base + r_gp[m_ins.x.rb];
    if ( update )
        r_gp[m_ins.d.ra] = address;
    if ( type != XTN_READ && (address % nb) != 0 )
        {
            m_exception = EXCEPT_ALIGNMENT;
            return;
        }
    m_dreq.valid = true;
    m_dreq.be = ((1<<nb)-1) << (address % 4);
    m_dreq.type = type;
    if ( type == XTN_READ )
        m_dreq.addr = nb*4;
    else
        m_dreq.addr = address;
    r_mem_dest = m_ins.d.rd;
    r_mem_reversed = reversed;
    r_mem_unsigned = unsigned_;
#if SOCLIB_MODULE_DEBUG
    std::cout
        << m_name << ": "
        << __FUNCTION__ << ": " << m_dreq
        << "->" << r_mem_dest << " Rev:" << r_mem_reversed << " U:" << r_mem_unsigned
        << std::endl;
#endif
}

void Ppc405Iss::mem_store_imm( DataOperationType type, uint32_t nb, bool update, uint32_t data )
{
    uint32_t base = (m_ins.d.ra || update) ? r_gp[m_ins.d.ra] : 0;
    uint32_t address = base + sign_ext(m_ins.d.imm, 16);
    if ( update )
        r_gp[m_ins.d.ra] = address;
    if ( type != XTN_READ && (address % nb) != 0 )
        {
            m_exception = EXCEPT_ALIGNMENT;
            return;
        }

    data <<= 8 * (4 - nb);
    data = soclib::endian::uint32_swap(data);
    data <<= 8 * (address % 4);

    m_dreq.valid = true;
    m_dreq.be = ((1<<nb)-1) << (address % 4);
    m_dreq.type = type;
    if ( type == XTN_WRITE )
        m_dreq.addr = nb*4;
    else
        m_dreq.addr = address;
    m_dreq.wdata = data;
    r_mem_dest = 0;
    r_mem_reversed = false;
    r_mem_unsigned = false;
#if SOCLIB_MODULE_DEBUG
    std::cout
        << m_name << ": "
        << __FUNCTION__ << ": " << m_dreq
        << "->" << r_mem_dest << " Rev:" << r_mem_reversed << " U:" << r_mem_unsigned
        << std::endl;
#endif
}

void Ppc405Iss::mem_store_indexed( DataOperationType type, uint32_t nb, bool update, uint32_t data )
{
    uint32_t base = (m_ins.x.ra || update) ? r_gp[m_ins.x.ra] : 0;
    uint32_t address = base + r_gp[m_ins.x.rb];
    if ( update )
        r_gp[m_ins.d.ra] = address;
    if ( type != XTN_READ && (address % nb) != 0 )
        {
            m_exception = EXCEPT_ALIGNMENT;
            return;
        }

    data <<= 8 * (4 - nb);
    data = soclib::endian::uint32_swap(data);
    data <<= 8 * (address % 4);

    m_dreq.valid = true;
    m_dreq.be = ((1<<nb)-1) << (address % 4);
    m_dreq.type = type;
    if ( type == XTN_WRITE )
        m_dreq.addr = nb*4;
    else
        m_dreq.addr = address;
    m_dreq.wdata = data;
    r_mem_dest = 0;
    r_mem_reversed = false;
    r_mem_unsigned = false;
#if SOCLIB_MODULE_DEBUG
    std::cout
        << m_name << ": "
        << __FUNCTION__ << ": " << m_dreq
        << "->" << r_mem_dest << " Rev:" << r_mem_reversed << " U:" << r_mem_unsigned
        << std::endl;
#endif
}

void Ppc405Iss::do_add( uint32_t opl, uint32_t opr, uint32_t ca, bool need_ca )
{
    bool cout, vout;
    uint32_t tmp = add_cv(opl, opr, ca, cout, vout);
    r_gp[m_ins.xo.rd] = tmp;

    if ( need_ca )
        caSet( cout );
    if ( m_ins.xo.oe )
        ovSet( vout );
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
}

uint32_t Ppc405Iss::do_addi( uint32_t opl, uint32_t opr, uint32_t ca, bool need_ca )
{
    bool cout, vout;
    uint32_t tmp = add_cv(opl, opr, ca, cout, vout);
    r_gp[m_ins.d.rd] = tmp;
    if ( need_ca )
        caSet( cout );
    return tmp;
}

void Ppc405Iss::branch_cond( uint32_t next_pc_if_taken )
{
    bool ctr_cond_met = true;
    if ( !(m_ins.b.bo & BO2) ) {
        uint32_t next_ctr = r_ctr - 1;
        r_ctr = next_ctr;
        ctr_cond_met = !!next_ctr ^ !!(m_ins.b.bo & BO3);
    }
    bool cr_cond_met =
        !!(m_ins.b.bo & BO0) ||
        (crBitGet(m_ins.b.bi) == !!(m_ins.b.bo & BO1));

#if SOCLIB_MODULE_DEBUG
    std::cout << m_name << " bcond"
              << ", CTR " << ((m_ins.b.bo & BO2) ? "" : "--")
              << " " << ((m_ins.b.bo & BO3) ? "== 0" : "!= 0");
    if ( !(m_ins.b.bo & BO0) )
        std::cout
            << ", CR ? (bo1)" << !!(m_ins.b.bo & BO1)
            << " == (cr" << m_ins.b.bi << ")" << crBitGet(m_ins.b.bi)
            << " cr: " << std::hex << r_cr;
    std::cout
        << ", PRED: " << !!(m_ins.b.bo & BO4)
        << " ctr: " << ctr_cond_met
        << " cr: " << cr_cond_met
        << " result: " << (ctr_cond_met && cr_cond_met)
        << std::endl;
#endif

    if ( ctr_cond_met && cr_cond_met ) {
        m_next_pc = next_pc_if_taken;
    }
    if ( m_ins.b.lk )
        r_lr = r_pc + 4;
}

// **Start**

void Ppc405Iss::op_add()
{
    do_add( r_gp[m_ins.xo.ra], r_gp[m_ins.xo.rb], 0, false );
}

void Ppc405Iss::op_addc()
{
    do_add( r_gp[m_ins.xo.ra], r_gp[m_ins.xo.rb], 0, true );
}

void Ppc405Iss::op_adde()
{
    do_add( r_gp[m_ins.xo.ra], r_gp[m_ins.xo.rb], caGet(), true );
}

void Ppc405Iss::op_addi()
{
    uint32_t base = m_ins.d.ra ? r_gp[m_ins.d.ra] : 0;
    do_addi( base, sign_ext(m_ins.d.imm, 16), 0, false );
}

void Ppc405Iss::op_addic()
{
    do_addi( r_gp[m_ins.d.ra], sign_ext(m_ins.d.imm, 16), 0, true );
}

void Ppc405Iss::op_addic_()
{
    uint32_t tmp = do_addi( r_gp[m_ins.d.ra], sign_ext(m_ins.d.imm, 16), 0, true );
    crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_addis()
{
    uint32_t base = m_ins.d.ra ? r_gp[m_ins.d.ra] : 0;
    do_addi( base, m_ins.d.imm<<16, 0, false );
}

void Ppc405Iss::op_addme()
{
    do_add( r_gp[m_ins.xo.ra], (uint32_t)-1, caGet(), true );
}

void Ppc405Iss::op_addze()
{
    do_add( r_gp[m_ins.xo.ra], 0, caGet(), true );
}

void Ppc405Iss::op_and()
{
    uint32_t tmp = r_gp[m_ins.x.rs] & r_gp[m_ins.x.rb];
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_andc()
{
    uint32_t tmp = r_gp[m_ins.x.rs] & ~r_gp[m_ins.x.rb];
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_andi()
{
    uint32_t tmp = r_gp[m_ins.d.rd] & m_ins.d.imm;
    r_gp[m_ins.d.ra] = tmp;
    crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_andis()
{
    uint32_t tmp = r_gp[m_ins.d.rd] & (m_ins.d.imm<<16);
    r_gp[m_ins.d.ra] = tmp;
    crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_b()
{
	uint32_t base = m_ins.i.aa ? 0 : r_pc;
    if ( m_ins.i.lk )
        r_lr = r_pc + 4;
    m_next_pc = base + sign_ext(m_ins.i.li<<2, 26);
}

void Ppc405Iss::op_bc()
{
    int32_t base = m_ins.b.aa ? 0 : r_pc;
    branch_cond( base + sign_ext(m_ins.i.li<<2, 16) );
}

void Ppc405Iss::op_bcctr()
{
    branch_cond( (r_ctr-!(m_ins.b.bo & BO2))&~0x3 );
}

void Ppc405Iss::op_bclr()
{
    branch_cond( r_lr );
}

void Ppc405Iss::op_cmp()
{
    crSetSigned( m_ins.x.rs>>2, r_gp[m_ins.x.ra], r_gp[m_ins.x.rb] );
}

void Ppc405Iss::op_cmpi()
{
    crSetSigned( m_ins.d.rd>>2, r_gp[m_ins.d.ra], sign_ext(m_ins.d.imm, 16) );
}

void Ppc405Iss::op_cmpl()
{
    crSetUnsigned( m_ins.x.rs>>2, r_gp[m_ins.x.ra], r_gp[m_ins.x.rb] );
}

void Ppc405Iss::op_cmpli()
{
    crSetUnsigned( m_ins.d.rd>>2, r_gp[m_ins.d.ra], m_ins.d.imm );
}

void Ppc405Iss::op_cntlzw()
{
    uint32_t rs = r_gp[m_ins.x.rs];
    int i = rs ? soclib::common::clz<uint32_t>(rs) : 32;
    r_gp[m_ins.x.ra] = i;
    if ( m_ins.x.rc )
        crSetSigned( 0, i, 0 );
}

void Ppc405Iss::op_crand()
{
	crBitSet( m_ins.x.rs, crBitGet( m_ins.x.ra ) && crBitGet( m_ins.x.rb ) );
}

void Ppc405Iss::op_crandc()
{
	crBitSet( m_ins.x.rs, crBitGet( m_ins.x.ra ) && !crBitGet( m_ins.x.rb ) );
}

void Ppc405Iss::op_creqv()
{
	crBitSet( m_ins.x.rs, crBitGet( m_ins.x.ra ) == crBitGet( m_ins.x.rb ) );
}

void Ppc405Iss::op_crnand()
{
	crBitSet( m_ins.x.rs, !(crBitGet( m_ins.x.ra ) && crBitGet( m_ins.x.rb )) );
}

void Ppc405Iss::op_crnor()
{
	crBitSet( m_ins.x.rs, !(crBitGet( m_ins.x.ra ) || crBitGet( m_ins.x.rb )) );
}

void Ppc405Iss::op_cror()
{
	crBitSet( m_ins.x.rs, crBitGet( m_ins.x.ra ) || crBitGet( m_ins.x.rb ) );
}

void Ppc405Iss::op_crorc()
{
	crBitSet( m_ins.x.rs, crBitGet( m_ins.x.ra ) || !crBitGet( m_ins.x.rb ) );
}

void Ppc405Iss::op_crxor()
{
	crBitSet( m_ins.x.rs, crBitGet( m_ins.x.ra ) != crBitGet( m_ins.x.rb ) );
}

void Ppc405Iss::op_dcba()
{
    // No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dcbf()
{
    // No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dcbi()
{
    mem_load_indexed( XTN_WRITE, XTN_DCACHE_INVAL, false, false, false );
}

void Ppc405Iss::op_dcbst()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dcbt()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dcbtst()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dcbz()
{
    // Must check for alignment when implementing
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dccci()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_dcread()
{
    // Must check for alignment when implementing
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_divw()
{
    int32_t a = r_gp[m_ins.xo.ra];
    int32_t b = r_gp[m_ins.xo.rb];
    int32_t tmp;
    bool ov;
    if ( !b || (b==-1 && a == (int32_t)0x80000000) ) {
        tmp = 0;
        ov = true;
    } else {
        tmp = a/b;
        ov = false;
    }
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.oe )
        ovSet( ov );
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 31 );
}

void Ppc405Iss::op_divwu()
{
    uint32_t a = r_gp[m_ins.xo.ra];
    uint32_t b = r_gp[m_ins.xo.rb];
    uint32_t tmp;
    bool ov;
    if ( !b ) {
        tmp = 0;
        ov = true;
    } else {
        tmp = a/b;
        ov = false;
    }
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.oe )
        ovSet( ov );
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 31 );
}

void Ppc405Iss::op_eieio()
{
	SOCLIB_WARNING("EIOIO Not implementable");
}

void Ppc405Iss::op_eqv()
{
    uint32_t tmp = ~(r_gp[m_ins.x.rs] ^ r_gp[m_ins.x.rb]);
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_extsb()
{
    uint32_t tmp = sign_ext(r_gp[m_ins.x.rs], 8);
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_extsh()
{
    uint32_t tmp = sign_ext(r_gp[m_ins.x.rs], 16);
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_icbi()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_icbt()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_iccci()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_icread()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_ill()
{
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PIL;
}

void Ppc405Iss::op_isync()
{
	// No cache support
    m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_lbz()
{
    mem_load_imm( DATA_READ, 1, false, false, true );
}

void Ppc405Iss::op_lbzu()
{
    mem_load_imm( DATA_READ, 1, true, false, true );
}

void Ppc405Iss::op_lbzux()
{
    mem_load_indexed( DATA_READ, 1, true, false, true );
}

void Ppc405Iss::op_lbzx()
{
    mem_load_indexed( DATA_READ, 1, false, false, true );
}

void Ppc405Iss::op_lha()
{
    mem_load_imm( DATA_READ, 2, false, false, false );
}

void Ppc405Iss::op_lhau()
{
    mem_load_imm( DATA_READ, 2, true, false, false );
}

void Ppc405Iss::op_lhaux()
{
    mem_load_indexed( DATA_READ, 2, true, false, false );
}

void Ppc405Iss::op_lhax()
{
    mem_load_indexed( DATA_READ, 2, false, false, false );
}

void Ppc405Iss::op_lhbrx()
{
    mem_load_indexed( DATA_READ, 2, false, true, false );
}

void Ppc405Iss::op_lhz()
{
    mem_load_imm( DATA_READ, 2, false, false, true );
}

void Ppc405Iss::op_lhzu()
{
    mem_load_imm( DATA_READ, 2, true, false, true );
}

void Ppc405Iss::op_lhzux()
{
    mem_load_indexed( DATA_READ, 2, true, false, true );
}

void Ppc405Iss::op_lhzx()
{
    mem_load_indexed( DATA_READ, 2, false, false, true );
}

void Ppc405Iss::op_lmw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_lswi()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_lswx()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_lwarx()
{
    mem_load_indexed( DATA_LL, 4, false, false, false );
}

void Ppc405Iss::op_lwbrx()
{
    mem_load_indexed( DATA_READ, 4, false, true, false );
}

void Ppc405Iss::op_lwz()
{
    mem_load_imm( DATA_READ, 4, false, false, true );
}

void Ppc405Iss::op_lwzu()
{
    mem_load_imm( DATA_READ, 4, true, false, true );
}

void Ppc405Iss::op_lwzux()
{
    mem_load_indexed( DATA_READ, 4, true, false, true );
}

void Ppc405Iss::op_lwzx()
{
    mem_load_indexed( DATA_READ, 4, false, false, true );
}

void Ppc405Iss::op_macchw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_macchws()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_macchwsu()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_macchwu()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_machhw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_machhws()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_machhwsu()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_machhwu()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_maclhw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_maclhws()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_maclhwsu()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_maclhwu()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_mcrf()
{
	crSet( m_ins.x.rs>>2, crGet( m_ins.x.ra>>2 ) );
}

void Ppc405Iss::op_mcrxr()
{
	crSet( m_ins.x.rs>>2, (r_xer.whole >> 28)&0xf );
    r_xer.whole &= ~(0xf << 28);
}

void Ppc405Iss::op_mfcr()
{
	r_gp[m_ins.x.rs] = r_cr;
}

void Ppc405Iss::op_mfdcr()
{
	if ( privsCheck() ) {
        uint32_t dcrn = PPC_SPLIT_FIELD(m_ins.xfx.opt);
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << "Accessing DCR " << std::hex << dcrn << std::endl;
#endif
        if ( dcrn >= DCR_MAX ) {
            m_exception = EXCEPT_PROGRAM;
            r_esr = ESR_PEU;
        }
        r_gp[m_ins.xfx.rs] = r_dcr[dcrn];
    }
}

void Ppc405Iss::op_mfmsr()
{
	if ( privsCheck() ) {
        r_gp[m_ins.x.rs] = r_msr.whole;
    }
}

void Ppc405Iss::op_mfspr()
{
    r_gp[m_ins.xfx.rs] = sprfGet( (enum Sprf)m_ins.xfx.opt );
}

void Ppc405Iss::op_mftb()
{
    uint32_t tbrn = m_ins.xfx.opt;
    enum Sprf sprf = (enum Sprf)0;
    switch (tbrn) {
    case PPC_SPLIT_FIELD(268):
        sprf = SPR_TBL;
        break;
    case PPC_SPLIT_FIELD(269):
        sprf = SPR_TBU;
        break;
    }
    r_gp[m_ins.xfx.rs] = sprfGet( sprf );
}

void Ppc405Iss::op_mtcrf()
{
    uint32_t mask = 0;
    uint32_t crm = m_ins.xfx.opt >> 1;
    for ( uint8_t mask_temp= 0x80; mask_temp; mask_temp >>= 1 ) {
        mask <<= 4;
        if ( crm & mask_temp )
            mask |= 0xf;
    }
#if SOCLIB_MODULE_DEBUG
    std::cout << m_name << " mtcrf " << std::hex << m_ins.xfx.opt << " " << mask << ", " << r_gp[m_ins.xfx.rs] << std::endl;
#endif
	r_cr = (r_gp[m_ins.xfx.rs] & mask) | (r_cr & ~mask);
}

void Ppc405Iss::op_mtdcr()
{
	if ( privsCheck() ) {
        uint32_t dcrn = PPC_SPLIT_FIELD(m_ins.xfx.opt);
        if ( dcrn >= DCR_MAX ) {
            m_exception = EXCEPT_PROGRAM;
            r_esr = ESR_PEU;
            return;
        }
        r_dcr[dcrn] = r_gp[m_ins.xfx.rs];
    }
}

void Ppc405Iss::op_mtmsr()
{
	if ( privsCheck() ) {
        r_msr.whole = r_gp[m_ins.x.rs];
    }
}

void Ppc405Iss::op_mtspr()
{
    sprfSet( (enum Sprf)m_ins.xfx.opt, r_gp[m_ins.xfx.rs] );
}

void Ppc405Iss::op_mulchw()
{
    int32_t a = sign_ext(r_gp[m_ins.xo.ra], 16);
    int32_t b = sign_ext(r_gp[m_ins.xo.rb] >> 16, 16);
    int32_t tmp = a * b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mulchwu()
{
    uint32_t a = 0xffff & r_gp[m_ins.xo.ra];
    uint32_t b = 0xffff & (r_gp[m_ins.xo.rb] >> 16);
    uint32_t tmp = a * b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mulhhw()
{
    int32_t a = sign_ext(r_gp[m_ins.xo.ra] >> 16, 16);
    int32_t b = sign_ext(r_gp[m_ins.xo.rb] >> 16, 16);
    int32_t tmp = a * b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mulhhwu()
{
    uint32_t a = 0xffff & (r_gp[m_ins.xo.ra] >> 16);
    uint32_t b = 0xffff & (r_gp[m_ins.xo.rb] >> 16);
    uint32_t tmp = a * b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mulhw()
{
    int64_t a = (int32_t)r_gp[m_ins.xo.ra];
    int64_t b = (int32_t)r_gp[m_ins.xo.rb];
    int64_t tmp = (a*b)>>32;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mulhwu()
{
    uint64_t a = r_gp[m_ins.xo.ra];
    uint64_t b = r_gp[m_ins.xo.rb];
    uint64_t tmp = (a*b)>>32;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mullhw()
{
    int16_t a = r_gp[m_ins.xo.ra];
    int16_t b = r_gp[m_ins.xo.rb];
    int32_t tmp = (int32_t)a * (int32_t)b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mullhwu()
{
    uint16_t a = r_gp[m_ins.xo.ra];
    uint16_t b = r_gp[m_ins.xo.rb];
    uint32_t tmp = (uint32_t)a * (uint32_t)b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_mulli()
{
    int32_t a = r_gp[m_ins.d.ra];
    int32_t b = sign_ext(m_ins.d.imm, 16);
    r_gp[m_ins.d.rd] = (uint32_t)(a*b);
    setInsDelay( 2 );
}

void Ppc405Iss::op_mullw()
{
    int64_t a = (int32_t)r_gp[m_ins.xo.ra];
    int64_t b = (int32_t)r_gp[m_ins.xo.rb];
    int64_t tmp = a*b;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.oe )
        ovSet( !!((uint64_t)tmp>>32) );
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
    setInsDelay( 2 );
}

void Ppc405Iss::op_nand()
{
    uint32_t tmp = ~(r_gp[m_ins.x.rs] & r_gp[m_ins.x.rb]);
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_neg()
{
    uint32_t tmp = r_gp[m_ins.xo.ra];
    bool ov = (tmp == 0x80000000);
    tmp = -tmp;
    r_gp[m_ins.xo.rd] = tmp;
    if ( m_ins.xo.oe )
        ovSet( ov );
    if ( m_ins.xo.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_nmacchw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_nmacchws()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_nmachhw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_nmachhws()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_nmaclhw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_nmaclhws()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert( 0 && "TODO" );
}

void Ppc405Iss::op_nor()
{
    uint32_t tmp = ~(r_gp[m_ins.x.rs] | r_gp[m_ins.x.rb]);
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

/* disabled
void Ppc405Iss::op_op19()
{
	
}
*/

/* disabled
void Ppc405Iss::op_op31()
{
	
}
*/

/* disabled
void Ppc405Iss::op_op4()
{
	
}
*/

void Ppc405Iss::op_or()
{
    uint32_t tmp = r_gp[m_ins.x.rs] | r_gp[m_ins.x.rb];
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_orc()
{
    uint32_t tmp = r_gp[m_ins.x.rs] | ~r_gp[m_ins.x.rb];
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_ori()
{
    uint32_t tmp = r_gp[m_ins.d.rd] | m_ins.d.imm;
    r_gp[m_ins.d.ra] = tmp;
}

void Ppc405Iss::op_oris()
{
	uint32_t tmp = r_gp[m_ins.d.rd] | (m_ins.d.imm<<16);
    r_gp[m_ins.d.ra] = tmp;
}

void Ppc405Iss::op_rfci()
{
	if ( privsCheck() ) {
        r_msr.whole = r_srr[3];
        m_next_pc = r_srr[2];
    }
}

void Ppc405Iss::op_rfi()
{
	if ( privsCheck() ) {
        r_msr.whole = r_srr[1];
        m_next_pc = r_srr[0];
    }
}

void Ppc405Iss::op_rlwimi()
{
	uint32_t m = mask_gen(m_ins.m.mb, m_ins.m.me);
    uint32_t r = rotl(r_gp[m_ins.m.rs], m_ins.m.sh);
    uint32_t tmp = (r&m)|(r_gp[m_ins.m.ra]&~m);
    r_gp[m_ins.m.ra] = tmp;
    if ( m_ins.m.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_rlwinm()
{
	uint32_t m = mask_gen(m_ins.m.mb, m_ins.m.me);
    uint32_t r = rotl(r_gp[m_ins.m.rs], m_ins.m.sh);
    uint32_t tmp = (r&m);
    r_gp[m_ins.m.ra] = tmp;
    if ( m_ins.m.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_rlwnm()
{
	uint32_t m = mask_gen(m_ins.m.mb, m_ins.m.me);
    uint32_t r = rotl(r_gp[m_ins.m.rs], r_gp[m_ins.m.sh]&0x1f);
    uint32_t tmp = (r&m);
    r_gp[m_ins.m.ra] = tmp;
    if ( m_ins.m.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_sc()
{
	m_exception = EXCEPT_SYSCALL;
}

void Ppc405Iss::op_slw()
{
	uint32_t n = r_gp[m_ins.x.rb]&0x1f;
    uint32_t tmp = r_gp[m_ins.x.rs] << n;
    if ( r_gp[m_ins.x.rb] & 0x20 )
        tmp = 0;
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_sraw()
{
	uint32_t n = r_gp[m_ins.x.rb]&0x1f;
    int32_t a = r_gp[m_ins.x.rs];
    int32_t tmp;
    if ( r_gp[m_ins.x.rb] & 0x20 ) {
        tmp = 0 - (a < 0);
        caSet( a<0 );
    } else {
        tmp = a >> n;;
        caSet( a<0 ? !!(a&((1<<n)-1)) : 0 );
    }
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_srawi()
{
	uint32_t n = m_ins.x.rb;
    int32_t a = r_gp[m_ins.x.rs];
    int32_t tmp = a >> n;
    r_gp[m_ins.x.ra] = tmp;
    caSet( a<0 ? !!(a&((1<<n)-1)) : 0 );
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_srw()
{
	uint32_t n = r_gp[m_ins.x.rb]&0x1f;
    uint32_t tmp = r_gp[m_ins.x.rs] >> n;
    if ( r_gp[m_ins.x.rb] & 0x20 )
        tmp = 0;
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_stb()
{
    mem_store_imm( DATA_WRITE, 1, false, r_gp[m_ins.d.rd] );
}

void Ppc405Iss::op_stbu()
{
	mem_store_imm( DATA_WRITE, 1, true, r_gp[m_ins.d.rd] );
}

void Ppc405Iss::op_stbux()
{
	mem_store_indexed( DATA_WRITE, 1, true, r_gp[m_ins.x.rs] );
}

void Ppc405Iss::op_stbx()
{
	mem_store_indexed( DATA_WRITE, 1, false, r_gp[m_ins.x.rs] );
}

void Ppc405Iss::op_sth()
{
	mem_store_imm( DATA_WRITE, 2, false, r_gp[m_ins.d.rd] );
}

void Ppc405Iss::op_sthbrx()
{
	mem_store_indexed( DATA_WRITE, 2, false, soclib::endian::uint16_swap(r_gp[m_ins.x.rs]) );
}

void Ppc405Iss::op_sthu()
{
	mem_store_imm( DATA_WRITE, 2, true, r_gp[m_ins.d.rd] );
}

void Ppc405Iss::op_sthux()
{
	mem_store_indexed( DATA_WRITE, 2, true, r_gp[m_ins.x.rs] );
}

void Ppc405Iss::op_sthx()
{
	mem_store_indexed( DATA_WRITE, 2, false, r_gp[m_ins.x.rs] );
}

void Ppc405Iss::op_stmw()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert(0 && "TODO");
}

void Ppc405Iss::op_stswi()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert(0 && "TODO");
}

void Ppc405Iss::op_stswx()
{
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
	// assert(0 && "TODO");
}

void Ppc405Iss::op_stw()
{
	mem_store_imm( DATA_WRITE, 4, false, r_gp[m_ins.d.rd] );
}

void Ppc405Iss::op_stwbrx()
{
    mem_store_indexed( DATA_WRITE, 4, false, soclib::endian::uint32_swap(r_gp[m_ins.x.rs]) );
}

void Ppc405Iss::op_stwcx()
{
    mem_store_indexed( DATA_SC, 4, false, r_gp[m_ins.x.rs] );
//    r_mem_dest = m_ins.x.rs;
}

void Ppc405Iss::op_stwu()
{
	mem_store_imm( DATA_WRITE, 4, true, r_gp[m_ins.d.rd] );
}

void Ppc405Iss::op_stwux()
{
    mem_store_indexed( DATA_WRITE, 4, true, r_gp[m_ins.x.rs] );
}

void Ppc405Iss::op_stwx()
{
    mem_store_indexed( DATA_WRITE, 4, false, r_gp[m_ins.x.rs] );
}

void Ppc405Iss::op_subf()
{
    do_add( ~r_gp[m_ins.xo.ra], r_gp[m_ins.xo.rb], 1, false );
}

void Ppc405Iss::op_subfc()
{
    do_add( ~r_gp[m_ins.xo.ra], r_gp[m_ins.xo.rb], 1, true );
}

void Ppc405Iss::op_subfe()
{
    do_add( ~r_gp[m_ins.xo.ra], r_gp[m_ins.xo.rb], caGet(), true );
}

void Ppc405Iss::op_subfic()
{
    do_addi( ~r_gp[m_ins.d.ra], sign_ext(m_ins.d.imm, 16), 1, true );
}

void Ppc405Iss::op_subfme()
{
    do_add( ~r_gp[m_ins.xo.ra], (uint32_t)-1, caGet(), true );
}

void Ppc405Iss::op_subfze()
{
    do_add( ~r_gp[m_ins.xo.ra], 0, caGet(), true );
}

void Ppc405Iss::op_sync()
{
    // No cache support
	m_exception = EXCEPT_PROGRAM;
    r_esr = ESR_PEU;
}

void Ppc405Iss::op_tw()
{
	trap( m_ins.x.rs, r_gp[m_ins.x.ra], r_gp[m_ins.x.rb] );
}

void Ppc405Iss::op_twi()
{
	trap( m_ins.d.rd, r_gp[m_ins.d.ra], sign_ext(m_ins.d.imm, 16) );
}

void Ppc405Iss::op_wrtee()
{
	if ( privsCheck() ) {
        r_msr.ee = !!(r_gp[m_ins.x.rs] & (1<<15));
    }
}

void Ppc405Iss::op_wrteei()
{
    if ( privsCheck() ) {
        r_msr.ee = !!(m_ins.ins & (1<<15));
    }
}

void Ppc405Iss::op_xor()
{
	uint32_t tmp = r_gp[m_ins.x.rs] ^ r_gp[m_ins.x.rb];
    r_gp[m_ins.x.ra] = tmp;
    if ( m_ins.x.rc )
        crSetSigned( 0, tmp, 0 );
}

void Ppc405Iss::op_xori()
{
	uint32_t tmp = r_gp[m_ins.d.rd] ^ m_ins.d.imm;
    r_gp[m_ins.d.ra] = tmp;
}

void Ppc405Iss::op_xoris()
{
	uint32_t tmp = r_gp[m_ins.d.rd] ^ (m_ins.d.imm<<16);
    r_gp[m_ins.d.ra] = tmp; 
}

// **End**

const uint32_t Ppc405Iss::except_addresses[] = {
    /* EXCEPT_NONE                 */ 0x0,
    /* EXCEPT_CRITICAL             */ 0x0100,
    /* EXCEPT_WATCHDOG             */ 0x1020,
    /* EXCEPT_DEBUG                */ 0x2000,
    /* EXCEPT_MACHINE_CHECK        */ 0x0200,
    /* EXCEPT_INSTRUCTION_STORAGE  */ 0x0400,
    /* EXCEPT_PROGRAM              */ 0x0700,
    /* EXCEPT_DATA_STORAGE         */ 0x0300,
    /* EXCEPT_DATA_TLB_MISS        */ 0x1100,
    /* EXCEPT_ALIGNMENT            */ 0x0600,
    /* EXCEPT_EXTERNAL             */ 0x0500,
    /* EXCEPT_SYSCALL              */ 0x0c00,
    /* EXCEPT_PI_TIMER             */ 0x1000,
    /* EXCEPT_FI_TIMER             */ 0x1010,
    /* EXCEPT_INSTRUCTION_TLB_MISS */ 0x1200,
};

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
