/*\
 * This file is part of SoCLIB.
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
 * MicroBlaze Instruction Set Simulator, developed for the SoCLib Project
 * Copyright (C) 2007-2008  SLS Group of the TIMA Lab, INPG
 *
 * Contributing authors : Li Bihong,
 *                        Yan Fu,
 *                        Hao Shen <Hao.Shen@imag.fr>,
 *                        Frédéric Pétrot <Frederic.Petrot@imag.fr>
 *
 * Based on the CASS MIPS ISS developed by Frédéric Pétrot and Denis
 * Hommais back in 1996
 * 
 * The MicroBlaze is a big endian machine, beware of bit numbering
 * below since it follows this convention
 * Note: The current Iss is based on the MicroBlaze version available
 * with EDK 8.2. It appears that quite a few new instructions have
 * been added in 9.2 (a MMU has been added, new mults and so on).
 * Modifications will be added if we encounter gcc generated code that
 * uses them.
\*/

#ifndef _SOCLIB_MICROBLAZE_ISS_H_
#define _SOCLIB_MICROBLAZE_ISS_H_

#include "common/iss/iss.h"
#include "common/endian.h"
#include "common/register.h"

#define R_IR_NOP        0x80000000 

/*\
 *  MicroBlaze Processor structure definition
\*/
namespace soclib {
   namespace common {
      class MicroBlazeIss
         : public soclib::common::Iss
      {
      private:
         enum Vectors {
            RESET_VECTOR     = 0x00000000,
            USER_VECTOR      = 0x00000008,
            INTERRUPT_VECTOR = 0x00000010,
            BREAK_VECTOR     = 0x00000010,
            EXCEPTION_VECTOR = 0x00000020
         };
         // Bits 27:31 of ESR, also called Exception Cause
         // Bit 20:26 are called Exception Specific Status
         // w = 0 means hword access, 1 means word access
         // s = 0 means unaligned load, 1 means unaligned store
         // rx contains the gpr index of source (store) or destination (load)
         enum Exception_Cause {
            UNALIGNED_DATA_ACCESS_EXCEPTION = 1,
            ILLEGAL_OPCODE_EXCEPTION        = 2,
            INSTRUCTION_BUS_ERROR_EXCEPTION = 3,
            DATA_BUS_ERROR_EXCEPTION        = 4,
            DIVIDE_BY_ZERO_EXCEPTION        = 5,
            FLOATING_POINT_UNIT_EXCEPTION   = 6
         };
         static const int w = 20, s = 21, rx = 22;

         /*\
          * Possible instruction types and helper struct
         \*/
         enum {TYPEA, TYPEB,  TYPEN};

         typedef struct {
            int opcode;  /* Internal op code. */
            int format;  /* Format type */
         } IFormat;

         /*\
          * Instruction decoding tables
         \*/
         static const IFormat OpcodeTable[];

         /*\
          * Instruction decoding function
         \*/
         static inline void IDecode(uint32_t ins, char *opcode, int *rd, int *ra, int *rb, int *imm)
         {
            const IFormat *Code;

            // Instruction decoding
            Code = &OpcodeTable[(ins >> 26) & 0x3F];
            *opcode = Code->opcode;
            *ra     = (ins >> 16) & 0x1F;
            *rd     = (ins >> 21) & 0x1F;
            if (Code->format == TYPEA) {
               *imm   = ins & 0x07FF;
               *rb    = (ins >> 11) & 0x1F;
            } else if (Code->format == TYPEB) {
               *imm   = ins & 0xFFFF;
            } else { // Reserved instruction
              fprintf(stderr, "Reserved instruction 0x%08x\n", ins); 
            }
         };
       
         // MicroBlaze Registers, all considered unsigned by default
         // (quite important for the comparisons and shifts implementation)
         uint32_t r_gpr[32]; // General Purpose Registers 
         uint32_t r_imm;     // Temporate Register
         uint32_t r_npc ;    // Next Program Counter, r_pc is in Iss
         uint32_t r_msr;     // Machine Status Register
         uint32_t r_ear;     // Exception address Register 
         uint32_t r_esr;     // Exception Status Register   
         uint32_t r_fsr;     // Floating Point Status Register 

         // States required but not visible as registers
         uint32_t m_ir;      // Current instruction
         bool     m_imm ;    // Imm
         bool     m_delay;   // Current instruction is in the delay slot
         bool     m_cancel;  // Cancel instruction in the delay slot
         bool     m_dbe;     // Data bus error
         bool     m_w;       // Unaligned access type
         uint32_t m_rx;      // Register in use when an unaligned access occurs

      public:
         /*\
          * The MicroBlaze has a single irq wire, called interrupt
         \*/
         static const int n_irq = 1;

         /*\
          * Boa                                                    (constrictor)
         \*/
         MicroBlazeIss(uint32_t ident);

         /*\
          * Reset handling
         \*/
         void reset(void)
         {
            Iss::reset(RESET_VECTOR);
            r_npc = RESET_VECTOR + 4;
            r_gpr[0] = 0;
            r_msr    = 0;
            r_ear    = 0;
            r_esr    = 0;
            m_ir     = R_IR_NOP; 
            m_imm    = 0;
            m_delay  = false;
            m_cancel = false;
         };

         /*\
          * Single stepping
         \*/
         void step(void);

         /*\
          * Useless single stepping
         \*/
         inline void nullStep()
         {
         };

         /*\
          * Feeds the Iss with an instruction to execute and an error
          * status
         \*/
         inline void setInstruction(bool error, uint32_t insn)
         {
            m_ibe = error;
            m_ir  = soclib::endian::uint32_swap(insn);
         };

         /*\
          * API for memory access through the Iss
         \*/
         void setRdata(bool error, uint32_t rdata);

#if 0
         // processor internal registers access API, used by
         // debugger. Mips order is 32 general-purpose; sr; lo; hi; bad; cause; pc;

         inline unsigned int get_register_count() const
         {
             return 32 + 6;
         }

         uint32_t get_register_value(unsigned int reg) const;

         inline size_t get_register_size(unsigned int reg) const
         {
             return 32;
         }

          void set_register_value(unsigned int reg, uint32_t value);
#endif

      };
   }
}
#endif // _SOCLIB_MICROBLAZE_ISS_H_
