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

/*
 * Functionnal description:
 * The tree files mips_iss.h, mips_iss.cc, & mips_instructions.cc
 * define an Instruction Set Simulator for the MIPS R3000 processor.
 * The same ISS has been wrapped to build CABA, TLMT, and PV simulation
 * models,  using appropriate wrappers.
 *
 * - One instruction is executed in one "step".
 *   (In case of a timed model one step corresponds to one cycle).
 * - The MIPS R3000 delayed branchs are supported: The ISS internal
 *   state contains two program counters (r_pc & r_npc), in order
 *   to describe the delayed branch behaviour.
 * - The MIPS R3000 delayed load behaviour : in case of RaW dependency
 *   between two successive instructions, the processor is dtalled
 *   during one step.
 * - The MIPS R3000 delayed multiply and divide behaviour is NOT
 *   supported : the mul and div instructions are supposed to be
 *   executed in one step.
 */

#ifndef _SOCLIB_MIPS_ISS_H_
#define _SOCLIB_MIPS_ISS_H_

#include <systemc.h>
#include "common/iss/iss.h"
#include "common/endian.h"
#include "common/register.h"

#define MIPS_DEBUG 0

namespace soclib { namespace common {

class MipsIss
    : public soclib::common::Iss
{
public:
    static const int n_irq = 6;

private:
    enum Addresses {
        EXCEPT_ADDRESS  = 0x80000080,
        RESET_ADDRESS   = 0xbfc00000
    };

    enum Cp0Reg {
        INDEX = 0,
        BAR = 8,
        COUNT = 9,
        STATUS = 12,
        CAUSE = 13,
        EPC = 14,
        IDENT = 15,
    };

    enum ExceptCause {
        X_INT,      // Interrupt
        X_MOD,      // TLB Modification
        X_TLBL,     // TLB Load error
        X_TLBS,     // TLB Store error
        X_ADEL,     // Address error (load or fetch)
        X_ADES,     // Address error (store)
        X_IBE,      // Ins bus error
        X_DBE,      // Data bus error (load/store)
        X_SYS,      // Syscall
        X_BP,       // Break point
        X_RI,       // Reserved
        X_CPU,      // Coproc unusable
        X_OV,       // Overflow
        X_TR,       // Trap
        X_reserved,     // Reserved
        X_FPE,      // Floating point
    };

    // member variables (internal registers)

    uint32_t    r_gp[32];       // General Registers
    uint32_t    r_hi;           // Multiply result (MSB bits)
    uint32_t    r_lo;           // Multiply result (LSB bits)
    uint32_t    r_cp0[32];      // Coprocessor registers

    typedef union {
        struct {
            union {
                PACKED_BITFIELD(
                    uint32_t op:6,
                    uint32_t imd:26
                    ) j;
                PACKED_BITFIELD(
                    uint32_t op:6,
                    uint32_t rs:5,
                    uint32_t rt:5,
                    uint32_t imd:16
                    ) i;
                PACKED_BITFIELD(
                    uint32_t op:6,
                    uint32_t rs:5,
                    uint32_t rt:5,
                    uint32_t rd:5,
                    uint32_t sh:5,
                    uint32_t func:6
                    ) r;
            } __attribute__((packed));
        } __attribute__((packed));
        uint32_t ins;
    } ins_t;

    // member variables used for communication between
    // member functions (they are not registers)

    ins_t       m_ins;
    uint32_t    m_exception;
    uint32_t    m_rs;
    uint32_t    m_rt;
    uint32_t    m_branch_address;
    bool        m_branch_taken;

public:
    MipsIss(uint32_t ident);
    ~MipsIss();

    void step();
    void reset();

private:
    inline void print_registers()
    {
        for (int i=0;i<8;i++)
        {
            for (int j=0;j<4;j++)
                printf("R%2.2d=%8.8x ",i*4+j,(int)r_gp[i*4+j]);
            printf("\n");
        }
        printf("pc=%8.8x ",r_pc);
    }

    void run();

    inline bool isInUserMode() const
    {
        return (uint32_t)r_cp0[STATUS] & 0x2;
    }

    inline bool isHighPC() const
    {
        return (uint32_t)r_pc & 0x80000000;
    }

    inline bool isPrivDataAddr( uint32_t addr) const
    {
        return addr & 0x80000000;
    }

    typedef void (MipsIss::*func_t)();

    static func_t const opcod_table[64];
    static func_t const special_table[64];

    void op_special();
    void op_bcond();
    void op_j();
    void op_jal();
    void op_beq();
    void op_bne();
    void op_blez();
    void op_bgtz();
    void op_addi();
    void op_addiu();
    void op_slti();
    void op_sltiu();
    void op_andi();
    void op_ori();
    void op_xori();
    void op_lui();
    void op_copro();
    void op_ill();
    void op_lb();
    void op_lh();
    void op_lw();
    void op_lbu();
    void op_lhu();
    void op_sb();
    void op_sh();
    void op_sw();
    void op_cache();

    void special_sll();
    void special_srl();
    void special_sra();
    void special_sllv();
    void special_srlv();
    void special_srav();
    void special_jr();
    void special_jalr();
    void special_sysc();
    void special_brek();
    void special_mfhi();
    void special_mthi();
    void special_mflo();
    void special_mtlo();
    void special_mult();
    void special_multu();
    void special_div();
    void special_divu();
    void special_add();
    void special_addu();
    void special_sub();
    void special_subu();
    void special_and();
    void special_or();
    void special_xor();
    void special_nor();
    void special_slt();
    void special_sltu();
    void special_ill();

    typedef enum {
        USE_NONE = 0,
        USE_T    = 1,
        USE_S    = 2,
        USE_ST   = 3,
        USE_SPECIAL = 4,
    } use_t;

    use_t curInstructionUsesRegs();

    static use_t const use_table[64];
    static use_t const use_special_table[64];
};

}}

#endif // _SOCLIB_MIPS_ISS_H_

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
