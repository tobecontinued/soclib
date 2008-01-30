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
 *
 * Maintainers: nipo
 *
 * $Id$
 */

#ifndef _SOCLIB_PPC405_ISS_H_
#define _SOCLIB_PPC405_ISS_H_

#include "iss.h"
#include "static_assert.h"
#include "soclib_endian.h"
#include "register.h"

#define PPC405_DEBUG 0

namespace soclib { namespace common {

#define PPC_PVR(own, pcf, cas, pcl, aid) \
(((own&0xfff)<<20)|((pcf&0xf)<<16)|((cas&0x3f)<<10)|((pcl&0xf)<<6)|((aid&0x3f)))

#define PPC_SPLIT_FIELD(x) (((((unsigned)x)>>5)|(((unsigned)x)<<5))&0x3ff)

class Ppc405Iss
	: public soclib::common::Iss
{
public:
	static const int n_irq = 2;
    enum {
        IRQ_CRITICAL_INPUT = 0,
        IRQ_EXTERNAL = 1
    };

private:
    enum SpecialAddresses {
        RESET_ADDR = 0xfffffffc,
    };

    enum ExceptSyndrome {
        ESR_MCI = 1<<0,
        ESR_PIL = 1<<4,
        ESR_PPR = 1<<5,
        ESR_PTR = 1<<6,
        ESR_PEU = 1<<7,
        ESR_DST = 1<<8,
        ESR_DIZ = 1<<9,
        ESR_PFP = 1<<12,
        ESR_PAP = 1<<13,
        ESR_U0F = 1<<16,
    };

    enum ExceptCause {
        EXCEPT_NONE,
        EXCEPT_CRITICAL,
        EXCEPT_WATCHDOG,
        EXCEPT_DEBUG,
        EXCEPT_MACHINE_CHECK,
        EXCEPT_INSTRUCTION_STORAGE,
        EXCEPT_PROGRAM,
        EXCEPT_DATA_STORAGE,
        EXCEPT_DATA_TLB_MISS,
        EXCEPT_ALIGNMENT,
        EXCEPT_EXTERNAL,
        EXCEPT_SYSCALL,
        EXCEPT_PI_TIMER,
        EXCEPT_FI_TIMER,
        EXCEPT_INSTRUCTION_TLB_MISS,
    };

    static const uint32_t except_addresses[];

    enum Dcrn {
        DCR_PROCNUM,
        DCR_CRITICAL,
        DCR_EXTERNAL,
        DCR_EXEC_CYCLES,
        DCR_MAX,
    };

    enum Sprf {
        SPR_XER      = PPC_SPLIT_FIELD(0x001), // 1
        SPR_LR       = PPC_SPLIT_FIELD(0x008), // 8
        SPR_CTR      = PPC_SPLIT_FIELD(0x009), // 9
        SPR_SRR0     = PPC_SPLIT_FIELD(0x01A), // 26
        SPR_SRR1     = PPC_SPLIT_FIELD(0x01B), // 27
        SPR_USPRG0   = PPC_SPLIT_FIELD(0x100), // 256
        SPR_SPRG4_RO = PPC_SPLIT_FIELD(0x104), // 260
        SPR_SPRG5_RO = PPC_SPLIT_FIELD(0x105), // 261
        SPR_SPRG6_RO = PPC_SPLIT_FIELD(0x106), // 262
        SPR_SPRG7_RO = PPC_SPLIT_FIELD(0x107), // 263
        SPR_SPRG0_RW = PPC_SPLIT_FIELD(0x110), // 272  Privileged
        SPR_SPRG1_RW = PPC_SPLIT_FIELD(0x111), // 273  Privileged
        SPR_SPRG2_RW = PPC_SPLIT_FIELD(0x112), // 274  Privileged
        SPR_SPRG3_RW = PPC_SPLIT_FIELD(0x113), // 275  Privileged
        SPR_SPRG4_RW = PPC_SPLIT_FIELD(0x114), // 276  Privileged
        SPR_SPRG5_RW = PPC_SPLIT_FIELD(0x115), // 277  Privileged
        SPR_SPRG6_RW = PPC_SPLIT_FIELD(0x116), // 278  Privileged
        SPR_SPRG7_RW = PPC_SPLIT_FIELD(0x117), // 279  Privileged
        SPR_TBL      = PPC_SPLIT_FIELD(0x11C), // 284  Privileged
        SPR_TBU      = PPC_SPLIT_FIELD(0x11D), // 285  Privileged
        SPR_PVR      = PPC_SPLIT_FIELD(0x11F), // 287  Privileged
        SPR_ZPR      = PPC_SPLIT_FIELD(0x3B0), // 944  Privileged
        SPR_PID      = PPC_SPLIT_FIELD(0x3B1), // 945  Privileged
        SPR_CCR0     = PPC_SPLIT_FIELD(0x3B3), // 947  Privileged
        SPR_IAC3     = PPC_SPLIT_FIELD(0x3B4), // 948  Privileged
        SPR_IAC4     = PPC_SPLIT_FIELD(0x3B5), // 949  Privileged
        SPR_DVC1     = PPC_SPLIT_FIELD(0x3B6), // 950  Privileged
        SPR_DVC2     = PPC_SPLIT_FIELD(0x3B7), // 951  Privileged
        SPR_SGR      = PPC_SPLIT_FIELD(0x3B9), // 953  Privileged
        SPR_DCWR     = PPC_SPLIT_FIELD(0x3BA), // 954  Privileged
        SPR_SLER     = PPC_SPLIT_FIELD(0x3BB), // 955  Privileged
        SPR_SU0R     = PPC_SPLIT_FIELD(0x3BC), // 956  Privileged
        SPR_DBCR1    = PPC_SPLIT_FIELD(0x3BD), // 957  Privileged
        SPR_ICDBDR   = PPC_SPLIT_FIELD(0x3D3), // 979  Privileged
        SPR_ESR      = PPC_SPLIT_FIELD(0x3D4), // 980  Privileged
        SPR_DEAR     = PPC_SPLIT_FIELD(0x3D5), // 981  Privileged
        SPR_EVPR     = PPC_SPLIT_FIELD(0x3D6), // 982  Privileged
        SPR_TSR      = PPC_SPLIT_FIELD(0x3D8), // 984  Privileged
        SPR_TCR      = PPC_SPLIT_FIELD(0x3DA), // 986  Privileged
        SPR_PIT      = PPC_SPLIT_FIELD(0x3DB), // 987  Privileged
        SPR_SRR2     = PPC_SPLIT_FIELD(0x3DE), // 990  Privileged
        SPR_SRR3     = PPC_SPLIT_FIELD(0x3DF), // 991  Privileged
        SPR_DBSR     = PPC_SPLIT_FIELD(0x3F0), // 1008 Privileged
        SPR_DBCR0    = PPC_SPLIT_FIELD(0x3F2), // 1010 Privileged
        SPR_IAC1     = PPC_SPLIT_FIELD(0x3F4), // 1012 Privileged
        SPR_IAC2     = PPC_SPLIT_FIELD(0x3F5), // 1013 Privileged
        SPR_DAC1     = PPC_SPLIT_FIELD(0x3F6), // 1014 Privileged
        SPR_DAC2     = PPC_SPLIT_FIELD(0x3F7), // 1015 Privileged
        SPR_DCCR     = PPC_SPLIT_FIELD(0x3FA), // 1018 Privileged
        SPR_ICCR     = PPC_SPLIT_FIELD(0x3FB), // 1019 Privileged

        SPR_PRIV_MASK= PPC_SPLIT_FIELD(0x010),
    };

    enum CompareResults {
        CMP_LT = 8,
        CMP_GT = 4,
        CMP_EQ = 2,
        CMP_SO = 1,
    };

    enum TrapConditions {
        TRAP_LT = 1,
        TRAP_GT = 2,
        TRAP_EQ = 4,
        TRAP_LTU = 8,
        TRAP_GTU = 16,
    };

    static const uint32_t pvr = PPC_PVR(0x50c, 1, 0, 1, 0x3f);

	typedef union {
		struct {
			union {
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t bo:5,
					uint32_t bi:5,
					uint32_t bd:14,
					uint32_t aa:1,
					uint32_t lk:1
					) b;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rd:5,
					uint32_t ra:5,
					uint32_t imm:16
					) d;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t li:24,
					uint32_t aa:1,
					uint32_t lk:1
					) i;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rs:5,
					uint32_t ra:5,
					uint32_t sh:5,
					uint32_t mb:5,
					uint32_t me:5,
					uint32_t rc:1
					) m;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rs:5,
					uint32_t ra:5,
					uint32_t rb:5,
					uint32_t func:10,
					uint32_t rc:1
					) x;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rs:5,
					uint32_t opt:10,
					uint32_t func:10,
					uint32_t unused0:1
					) xfx;
				PACKED_BITFIELD(
					uint32_t op:6,
					uint32_t rd:5,
					uint32_t ra:5,
					uint32_t rb:5,
					uint32_t oe:1,
					uint32_t func:9,
					uint32_t rc:1
					) xo;
			} __attribute__((packed));
        } __attribute__((packed));
        uint32_t ins;
	} ins_t;

    typedef REG32_BITFIELD(
        uint32_t so:1,
        uint32_t ov:1,
        uint32_t ca:1,
        uint32_t reserved:22,
        uint32_t tbc:7
        ) xer_t;

    typedef REG32_BITFIELD(
        uint32_t reserved0:6,
        uint32_t ap:1,
        uint32_t reserved1:5,
        uint32_t ape:1,
        uint32_t we:1,
        uint32_t ce:1,
        uint32_t reserved2:1,
        uint32_t ee:1,
        uint32_t pr:1,
        uint32_t fp:1,
        uint32_t me:1,
        uint32_t fe0:1,
        uint32_t dwe:1,
        uint32_t de:1,
        uint32_t fe1:1,
        uint32_t reserved3:2,
        uint32_t ir:1,
        uint32_t dr:1,
        uint32_t reserved4:4,
        ) msr_t;

    static_assert(sizeof(msr_t) == 4);

	// member variables used for communication between
	// member functions (they are not registers)

	uint32_t 	r_pc;

    bool        r_mem_req;
    bool        r_mem_unsigned;
    bool        r_mem_reversed;
	enum DataAccessType 	r_mem_type;
	uint32_t 	r_mem_addr;
	uint32_t 	r_mem_wdata;
	uint32_t	r_mem_dest;
	bool		r_dbe;
	bool		m_ibe;
	bool		m_dbe;
	uint32_t 	m_irq;

    uint32_t m_ins_delay;

	ins_t m_ins;
	enum ExceptCause m_exception;
    uint32_t m_next_pc;

    uint32_t    m_exec_cycles;

    // Registers
	uint32_t r_gp[32];

    // Ppc special registers
    uint32_t r_dcr[DCR_MAX];
    uint32_t r_ctr;
    uint32_t r_lr;
    uint32_t r_sprg[8];
    uint64_t r_tb;
    xer_t r_xer;
    uint32_t r_cr;
    msr_t r_msr;

    // Exception registers
    uint32_t r_srr[4];
    uint32_t r_evpr;
    uint32_t r_esr;
    uint32_t r_dear;

public:

    int cpuCauseToSignal( uint32_t cause )
    {
        switch (cause)
            {
            case EXCEPT_PROGRAM:
                return 4;       // GDB SIGILL
            case EXCEPT_DEBUG:
                return 5;       // GDB SIGTRAP
            case EXCEPT_ALIGNMENT:
                return 10;      // GDB SIGBUS
            default:
                return 11;      // GDB SIGSEGV
            }
    }

	Ppc405Iss(uint32_t ident);

	void reset();

    inline uint32_t isBusy()
    {
        return m_ins_delay;
    }

	void step();
    void nullStep( uint32_t time_passed = 1 )
    {
        if ( m_ins_delay ) {
            if ( m_ins_delay > time_passed )
                m_ins_delay -= time_passed;
            else
                m_ins_delay = 0;
        }
        r_tb++;
    }

	inline void getInstructionRequest(bool &req, uint32_t &address) const
	{
        req = true;
		address = r_pc;
	}

	inline void setInstruction(bool error, uint32_t val)
	{
		m_ibe = error;
		m_ins.ins = soclib::endian::uint32_swap(val);
	}

	inline void getDataRequest(bool &valid, enum DataAccessType &type,
                               uint32_t &address, uint32_t &wdata) const
	{
        valid = r_mem_req;
		address = r_mem_addr;
		wdata = soclib::endian::uint32_swap(r_mem_wdata);
		type = r_mem_type;
	}

	void setDataResponse(bool error, uint32_t rdata);

	inline void setWriteBerr()
	{
		r_dbe = true;
	}

	inline void setIrq(uint32_t irq)
	{
		m_irq = irq;
	}

    // processor internal registers access API, used by
    // debugger. PPC regs order is 32 32bits GPs, 32 64bits FPs, pc, ps, cnd, lr, cnt, xer, mq, fpscr

    inline unsigned int getDebugRegisterCount() const
    {
        return 32 + 32 + 8;
    }

    uint32_t getDebugRegisterValue(unsigned int reg) const;
    size_t getDebugRegisterSize(unsigned int reg) const;
    void setDebugRegisterValue(unsigned int reg, uint32_t value);

    inline uint32_t getDebugPC() const
    {
        return r_pc;
    }

    inline void setDebugPC(uint32_t pc)
    {
        r_pc = pc;
    }

protected:
    void exceptionProcess( uint32_t cause );

private:
	void run();

    inline void setInsDelay( uint32_t delay )
    {
        assert( delay > 0 );
        m_ins_delay = delay-1;
    }

	typedef void (Ppc405Iss::*func_t)();

	static func_t const op_op19_table[14];
	static func_t const op_op31_table[342];
	static func_t const op_op4_table[42];
	static func_t const run_table[64];

    inline bool privsCheck()
    {
        if ( r_msr.pr )
            m_exception = EXCEPT_PROGRAM;
        return !r_msr.pr;
    }

    void trap( uint32_t, uint32_t, uint32_t );

    inline void caSet( bool ca )
    {
        r_xer.ca = ca;
    }

    inline bool caGet() const
    {
        return r_xer.ca;
    }

    inline void crSetSigned( int cr_no, int32_t a, int32_t b )
    {
        int cr = 0;
        if ( a < b ) cr |= CMP_LT;
        if ( a > b ) cr |= CMP_GT;
        if ( a == b ) cr |= CMP_EQ;
        if ( r_xer.so ) cr |= CMP_SO;
        crSet( cr_no, cr );
    }

    inline void crSetUnsigned( int cr_no, uint32_t a, uint32_t b )
    {
        int cr = 0;
        if ( a < b ) cr |= CMP_LT;
        if ( a > b ) cr |= CMP_GT;
        if ( a == b ) cr |= CMP_EQ;
        if ( r_xer.so ) cr |= CMP_SO;
        crSet( cr_no, cr );
    }

    inline uint32_t crGet( int no ) const
    {
        return (r_cr>>((7-no)*4))&0xf;
    }

    inline void crSet( int no, uint32_t cr )
    {
        uint32_t mask = 0xf<<((7-no)*4);
        r_cr = mask&(cr<<((7-no)*4)) | r_cr&~mask;
    }

    inline void ovSet( bool ov )
    {
        r_xer.ov = ov;
        r_xer.so |= ov;
    }

    void sprfSet( enum Sprf, uint32_t );
    uint32_t sprfGet( enum Sprf );

    inline void mem_load_imm( DataAccessType type, bool update, bool, bool );
    inline void mem_load_indexed( DataAccessType type, bool update, bool, bool );
    inline void mem_store_imm( DataAccessType type, bool update, uint32_t data );
    inline void mem_store_indexed( DataAccessType type, bool update, uint32_t data );
    inline void do_add( uint32_t opl, uint32_t opr, uint32_t ca, bool need_ca );
    inline uint32_t do_addi( uint32_t opl, uint32_t opr, uint32_t ca, bool need_ca );
    inline void branch_cond( uint32_t next_pc_if_taken );

    void dump() const;

#include "ppc405_ops.inc"

};

}}

#endif // _SOCLIB_PPC405_ISS_H_

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
