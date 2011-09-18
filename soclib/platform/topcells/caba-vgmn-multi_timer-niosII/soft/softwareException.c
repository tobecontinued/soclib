/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
******************************************************************************/
	
    /*
     * This is the software exception handler for Nios2.
     */	

     /*
      * Explicitly allow the use of r1 (the assembler temporary register)
      * within this code. This register is normally reserved for the use of
      * the compiler.
      */

__asm__(
    /*    ".section .softwareException, \"xa\" \n"*/
    ".section .excep, \"xa\"                   \n"
    ".globl softwareException           \n"
/*     ".type softwareException, @function \n" */
	
    ".set noat                           \n"
    ".set nobreak                        \n"
"softwareException:                     \n"
   /*
    * Did a trap instruction cause the exception?
    */
    "addi ea, ea, -4                     \n"
    "ldw et,0(ea)                        \n"  /* instruction that caused the exception */
    "xorhi et,et,0x003b                  \n"  /* upper half of trap opcode */ 
    "xori  et,et,0x683a                  \n"  /* lower half of trap opcode */ 
    "beq et,zero,trap_handler            \n"

"#ifndef ALT_NO_INSTRUCTION_EMULATION    \n"

   /* INSTRUCTION EMULATION
    * ---------------------
    *
    * Nios II processors generate exceptions for unimplemented instructions.
    * The routines below emulate these instructions.  Depending on the
    * processor core, the only instructions that might need to be emulated
    * are div, divu, mul, muli, mulxss, mulxsu, and mulxuu.
    *
    * The emulations match the instructions, except for the following
    * limitations:
    *
    * 1) The emulation routines do not emulate the use of the exception
    *    temporary register (et) as a source operand because the exception
    *    handler already has modified it.
    *
    * 2) The routines do not emulate the use of the stack pointer (sp) or the
    *    exception return address register (ea) as a destination because
    *    modifying these registers crashes the exception handler or the
    *    interrupted routine.
    *
    * Detailed Design
    * ---------------
    *
    * The emulation routines expect the contents of integer registers r0-r31
    * to be on the stack at addresses sp, 4(sp), 8(sp), ... 124(sp).  The
    * routines retrieve source operands from the stack and modify the
    * destination register's value on the stack prior to the end of the
    * exception handler.  Then all registers except the destination register
    * are restored to their previous values.
    *
    * The instruction that causes the exception is found at address -4(ea).
    * The instruction's OP and OPX fields identify the operation to be
    * performed.
    *
    * One instruction, muli, is an I-type instruction that is identified by
    * an OP field of 0x24.
    *
    * muli   AAAAA,BBBBB,IIIIIIIIIIIIIIII,-0x24-
    *           27    22                6      0    <-- LSB of field
    *
    * The remaining emulated instructions are R-type and have an OP field
    * of 0x3a.  Their OPX fields identify them.
    *
    * R-type AAAAA,BBBBB,CCCCC,XXXXXX,NNNNN,-0x3a-
    *           27    22    17     11     6      0  <-- LSB of field
    * 
    * 
    * Opcode Encoding.  muli is identified by its OP value.  Then OPX & 0x02
    * is used to differentiate between the division opcodes and the remaining
    * multiplication opcodes.
    *
    * Instruction   OP      OPX    OPX & 0x02
    * -----------   ----    ----   ----------
    * muli          0x24
    * divu          0x3a    0x24         0
    * div           0x3a    0x25         0
    * mul           0x3a    0x27      != 0
    * mulxuu        0x3a    0x07      != 0
    * mulxsu        0x3a    0x17      != 0
    * mulxss        0x3a    0x1f      != 0
    */


   /*
    * Save everything on the stack to make it easy for the emulation routines
    * to retrieve the source register operands.
    */

    "addi sp, sp, -128                   \n"
    "stw zero,  0(sp)                    \n"    /* Save zero on stack to avoid special case for r0. */
    "stw at,    4(sp)                    \n"
    "stw r2,    8(sp)                    \n"
    "stw r3,   12(sp)                    \n"
    "stw r4,   16(sp)                    \n"
    "stw r5,   20(sp)                    \n"
    "stw r6,   24(sp)                    \n"
    "stw r7,   28(sp)                    \n"
    "stw r8,   32(sp)                    \n"
    "stw r9,   36(sp)                    \n"
    "stw r10,  40(sp)                    \n"
    "stw r11,  44(sp)                    \n"
    "stw r12,  48(sp)                    \n"
    "stw r13,  52(sp)                    \n"
    "stw r14,  56(sp)                    \n"
    "stw r15,  60(sp)                    \n"
    "stw r16,  64(sp)                    \n"
    "stw r17,  68(sp)                    \n"
    "stw r18,  72(sp)                    \n"
    "stw r19,  76(sp)                    \n"
    "stw r20,  80(sp)                    \n"
    "stw r21,  84(sp)                    \n"
    "stw r22,  88(sp)                    \n"
    "stw r23,  92(sp)                    \n"
                     /* Don't bother to save et.  It's already been changed. */
    "stw bt,  100(sp)                    \n"
    "stw gp,  104(sp)                    \n"
    "stw sp,  108(sp)                    \n"
    "stw fp,  112(sp)                    \n"
                     /* Don't bother to save ea.  It's already been changed. */
    "stw ba,  120(sp)                    \n"
    "stw ra,  124(sp)                    \n"


   /*
    * Split the instruction into its fields.  We need 4*A, 4*B, and 4*C as
    * offsets to the stack pointer for access to the stored register values.
    */
    "ldw r2,-4(ea)                       \n"   /* r2 = AAAAA,BBBBB,IIIIIIIIIIIIIIII,PPPPPP */  
    "roli r3,r2,7                        \n"   /* r3 = BBB,IIIIIIIIIIIIIIII,PPPPPP,AAAAA,BB */ 
    "roli r4,r3,3                        \n"   /* r4 = IIIIIIIIIIIIIIII,PPPPPP,AAAAA,BBBBB */  
    "roli r5,r4,2                        \n"   /* r5 = IIIIIIIIIIIIII,PPPPPP,AAAAA,BBBBB,II */ 
    "srai r4,r4,16                       \n"   /* r4 = (sign-extended) IMM16 */ 
    "roli r6,r5,5                        \n"   /* r6 = XXXX,NNNNN,PPPPPP,AAAAA,BBBBB,CCCCC,XX */ 
    "andi r2,r2,0x3f                     \n"   /* r2 = 00000000000000000000000000,PPPPPP */ 
    "andi r3,r3,0x7c                     \n"   /* r3 = 0000000000000000000000000,AAAAA,00 */ 
    "andi r5,r5,0x7c                     \n"   /* r5 = 0000000000000000000000000,BBBBB,00 */ 
    "andi r6,r6,0x7c                     \n"   /* r6 = 0000000000000000000000000,CCCCC,00 */ 

   /* Now
    * r2 = OP
    * r3 = 4*A
    * r4 = IMM16 (sign extended)
    * r5 = 4*B
    * r6 = 4*C
    */


   /*
    *  Prepare for either multiplication or division loop.
    *  They both loop 32 times.
    */
    "movi r14,32                         \n"


   /*
    * Get the operands.
    *
    * It is necessary to check for muli because it uses an I-type instruction
    * format, while the other instructions are have an R-type format.
    */
    "add  r3,r3,sp                       \n"    /* r3 = address of A-operand. */ 
    "ldw  r3,0(r3)                       \n"    /* r3 = A-operand. */ 
    "movi r7,0x24                        \n"    /* muli opcode (I-type instruction format) */ 
    "beq r2,r7,mul_immed                 \n"    /* muli doesn't use the B register as a source */ 

    "add  r5,r5,sp                       \n"    /* r5 = address of B-operand. */ 
    "ldw  r5,0(r5)                       \n"    /* r5 = B-operand. */ 
                                                /* r4 = SSSSSSSSSSSSSSSS,-----IMM16------ */
                                                /* IMM16 not needed, align OPX portion */
                                                /* r4 = SSSSSSSSSSSSSSSS,CCCCC,-OPX--,00000 */
    "srli r4,r4,5                        \n"    /* r4 = 00000,SSSSSSSSSSSSSSSS,CCCCC,-OPX-- */ 
    "andi r4,r4,0x3f                     \n"    /* r4 = 00000000000000000000000000,-OPX-- */ 

   /* Now
    * r2 = OP
    * r3 = src1
    * r5 = src2
    * r4 = OPX (no longer can be muli)
    * r6 = 4*C
    * r14 = loop counter
    */


   /*
    *  Multiply or Divide?
    */
    "andi r7,r4,0x02                     \n"    /* For R-type multiply instructions, OPX & 0x02 != 0 */
    "bne r7,zero,multiply                \n"


   /* DIVISION
    *
    * Divide an unsigned dividend by an unsigned divisor using
    * a shift-and-subtract algorithm.  The example below shows
    * 43 div 7 = 6 for 8-bit integers.  This classic algorithm uses a
    * single register to store both the dividend and the quotient,
    * allowing both values to be shifted with a single instruction.
    *
    *                               remainder dividend:quotient
    *                               --------- -----------------
    *   initialize                   00000000     00101011:
    *   shift                        00000000     0101011:_
    *   remainder >= divisor? no     00000000     0101011:0
    *   shift                        00000000     101011:0_
    *   remainder >= divisor? no     00000000     101011:00
    *   shift                        00000001     01011:00_
    *   remainder >= divisor? no     00000001     01011:000
    *   shift                        00000010     1011:000_
    *   remainder >= divisor? no     00000010     1011:0000
    *   shift                        00000101     011:0000_
    *   remainder >= divisor? no     00000101     011:00000
    *   shift                        00001010     11:00000_
    *   remainder >= divisor? yes    00001010     11:000001
    *       remainder -= divisor   - 00000111
    *                              ----------
    *                                00000011     11:000001
    *   shift                        00000111     1:000001_
    *   remainder >= divisor? yes    00000111     1:0000011
    *       remainder -= divisor   - 00000111
    *                              ----------
    *                                00000000     1:0000011
    *   shift                        00000001     :0000011_
    *   remainder >= divisor? no     00000001     :00000110
    *
    * The quotient is 00000110.
    */

"divide:                                 \n"
   /*
    *  Prepare for division by assuming the result
    *  is unsigned, and storing its "sign" as 0.
    */
    "movi r17,0                          \n"


    /* Which division opcode? */
    "xori r7,r4,0x25                     \n"    /* OPX of div */ 
    "bne r7,zero,unsigned_division       \n"


   /*
    *  OPX is div.  Determine and store the sign of the quotient.
    *  Then take the absolute value of both operands.
    */
    "xor r17,r3,r5                       \n"    /* MSB contains sign of quotient */ 
    "bge r3,zero,dividend_is_nonnegative \n"
    "sub r3,zero,r3                      \n"
"dividend_is_nonnegative:                \n"
    "bge r5,zero,divisor_is_nonnegative  \n"
    "sub r5,zero,r5                      \n"
"divisor_is_nonnegative:                 \n"


"unsigned_division:                      \n"
    /* Initialize the unsigned-division loop. */
    "movi r13,0                          \n"    /* remainder = 0 */ 

   /* Now
    * r3 = dividend : quotient
    * r4 = 0x25 for div, 0x24 for divu
    * r5 = divisor
    * r13 = remainder
    * r14 = loop counter (already initialized to 32)
    * r17 = MSB contains sign of quotient
    */


   /*
    *   for (count = 32; count > 0; --count)
    *   {
    */
"divide_loop:                            \n"

   /*
    *       Division:
    *
    *       (remainder:dividend:quotient) <<= 1;
    */
    "slli r13,r13,1                      \n"
    "cmplt r7,r3,zero                    \n"    /* r7 = MSB of r3 */ 
    "or r13,r13,r7                       \n"
    "slli r3,r3,1                        \n"


   /*
    *       if (remainder >= divisor)
    *       {
    *           set LSB of quotient
    *           remainder -= divisor;
    *       }
    */
    "bltu r13,r5,div_skip                \n"
    "ori r3,r3,1                         \n"
    "sub r13,r13,r5                      \n"
"div_skip:                               \n"

   /*
    *   }
    */
    "subi r14,r14,1                      \n"
    "bne r14,zero,divide_loop            \n"


   /* Now
    * r3 = quotient
    * r4 = 0x25 for div, 0x24 for divu
    * r6 = 4*C
    * r17 = MSB contains sign of quotient
    */

    
   /*
    *  Conditionally negate signed quotient.  If quotient is unsigned,
    *  the sign already is initialized to 0.
    */
    "bge r17,zero,quotient_is_nonnegative\n"
    "sub r3,zero,r3                      \n"    /* -r3 */
"quotient_is_nonnegative:                \n"


   /*
    *  Final quotient is in r3.
    */
    "add r6,r6,sp                        \n"
    "stw r3,0(r6)                        \n"    /* write quotient to stack */
    "br restore_registers                \n"




   /* MULTIPLICATION
    *
    * A "product" is the number that one gets by summing a "multiplicand"
    * several times.  The "multiplier" specifies the number of copies of the
    * multiplicand that are summed.
    *
    * Actual multiplication algorithms don't use repeated addition, however.
    * Shift-and-add algorithms get the same answer as repeated addition, and
    * they are faster.  To compute the lower half of a product (pppp below)
    * one shifts the product left before adding in each of the partial products
    * (a * mmmm) through (d * mmmm).
    *
    * To compute the upper half of a product (PPPP below), one adds in the
    * partial products (d * mmmm) through (a * mmmm), each time following the
    * add by a right shift of the product.
    *
    *     mmmm
    *   * abcd
    *   ------
    *     ####  = d * mmmm
    *    ####   = c * mmmm
    *   ####    = b * mmmm
    *  ####     = a * mmmm
    * --------
    * PPPPpppp
    *
    * The example above shows 4 partial products.  Computing actual Nios II
    * products requires 32 partials.
    *
    * It is possible to compute the result of mulxsu from the result of mulxuu
    * because the only difference between the results of these two opcodes is
    * the value of the partial product associated with the sign bit of rA.
    *
    *   mulxsu = mulxuu - (rA < 0) ? rB : 0;
    *
    * It is possible to compute the result of mulxss from the result of mulxsu
    * because the only difference between the results of these two opcodes is
    * the value of the partial product associated with the sign bit of rB.
    *
    *   mulxss = mulxsu - (rB < 0) ? rA : 0;
    *
    */

"mul_immed:                              \n"
    /* Opcode is muli.  Change it into mul for remainder of algorithm. */
    "mov r6,r5                           \n"    /* Field B is dest register, not field C. */ 
    "mov r5,r4                           \n"    /* Field IMM16 is src2, not field B. */ 
    "movi r4,0x27                        \n"    /* OPX of mul is 0x27 */ 

"multiply:                               \n"
    /* Initialize the multiplication loop. */
    "movi r9,0                           \n"    /* mul_product    = 0 */ 
    "movi r10,0                          \n"    /* mulxuu_product = 0 */ 
    "mov r11,r5                          \n"    /* save original multiplier for mulxsu and mulxss */ 
    "mov r12,r5                          \n"    /* mulxuu_multiplier (will be shifted) */ 
    "movi r16,1                          \n"    /* used to create "rori B,A,1" from "ror B,A,r16" */

   /* Now
    * r3 = multiplicand
    * r5 = mul_multiplier
    * r6 = 4 * dest_register (used later as offset to sp)
    * r7 = temp
    * r9 = mul_product
    * r10 = mulxuu_product
    * r11 = original multiplier
    * r12 = mulxuu_multiplier
    * r14 = loop counter (already initialized)
    * r16 = 1
    */


   /*
    *   for (count = 32; count > 0; --count)
    *   {
    */
"multiply_loop:                          \n"

   /*
    *       mul_product <<= 1;
    *       lsb = multiplier & 1;
    */
    "slli r9,r9,1                        \n"
    "andi r7,r12,1                       \n"

   /*
    *       if (lsb == 1)
    *       {
    *           mulxuu_product += multiplicand;
    *       }
    */
    "beq r7,zero,mulx_skip               \n"
    "add r10,r10,r3                      \n"
    "cmpltu r7,r10,r3                    \n"    /* Save the carry from the MSB of mulxuu_product. */ 
    "ror r7,r7,r16                       \n"    /* r7 = 0x80000000 on carry, or else 0x00000000 */
"mulx_skip: \n"

   /*
    *       if (MSB of mul_multiplier == 1)
    *       {
    *           mul_product += multiplicand;
    *       }
    */
    "bge r5,zero,mul_skip                \n"
    "add r9,r9,r3                        \n"
    "mul_skip: \n"

   /*
    *       mulxuu_product >>= 1;           logical shift
    *       mul_multiplier <<= 1;           done with MSB
    *       mulx_multiplier >>= 1;          done with LSB
    */
    "srli r10,r10,1                      \n"
    "or r10,r10,r7                       \n"    /* OR in the saved carry bit. */
    "slli r5,r5,1                        \n"
    "srli r12,r12,1                      \n"


   /*
    *   }
    */
    "subi r14,r14,1                      \n"
    "bne r14,zero,multiply_loop          \n"


   /*
    *  Multiply emulation loop done.
    */

   /* Now
    * r3 = multiplicand
    * r4 = OPX
    * r6 = 4 * dest_register (used later as offset to sp)
    * r7 = temp
    * r9 = mul_product
    * r10 = mulxuu_product
    * r11 = original multiplier
    */


    /* Calculate address for result from 4 * dest_register */
    "add r6,r6,sp                        \n"


   /*
    *  Select/compute the result based on OPX.
    */


    /* OPX == mul?  Then store. */
    "xori r7,r4,0x27                     \n"
    "beq r7,zero,store_product           \n"

    /* It's one of the mulx.. opcodes.  Move over the result. */
    "mov r9,r10                          \n"

    /* OPX == mulxuu?  Then store. */
    "xori r7,r4,0x07                     \n"
    "beq r7,zero,store_product           \n"

    /* Compute mulxsu
     *
     * mulxsu = mulxuu - (rA < 0) ? rB : 0; 
     */
    "bge r3,zero,mulxsu_skip             \n"
    "sub r9,r9,r11                       \n"
    "mulxsu_skip:                        \n"

    /* OPX == mulxsu?  Then store. */
    "xori r7,r4,0x17                     \n"
    "beq r7,zero,store_product           \n"

    /* Compute mulxss
     *
     * mulxss = mulxsu - (rB < 0) ? rA : 0;
     */
    "bge r11,zero,mulxss_skip            \n"
    "sub r9,r9,r3                        \n"
"mulxss_skip:                                \n"
    /* At this point, assume that OPX is mulxss, so store */


"store_product:                          \n"
    "stw  r9,0(r6)                       \n"


"restore_registers: \n"
                        /* No need to restore r0. */
    "ldw at,    4(sp)                    \n"
    "ldw r2,    8(sp)                    \n"
    "ldw r3,   12(sp)                    \n"
    "ldw r4,   16(sp)                    \n"
    "ldw r5,   20(sp)                    \n"
    "ldw r6,   24(sp)                    \n"
    "ldw r7,   28(sp)                    \n"
    "ldw r8,   32(sp)                    \n"
    "ldw r9,   36(sp)                    \n"
    "ldw r10,  40(sp)                    \n"
    "ldw r11,  44(sp)                    \n"
    "ldw r12,  48(sp)                    \n"
    "ldw r13,  52(sp)                    \n"
    "ldw r14,  56(sp)                    \n"
    "ldw r15,  60(sp)                    \n"
    "ldw r16,  64(sp)                    \n"
    "ldw r17,  68(sp)                    \n"
    "ldw r18,  72(sp)                    \n"
    "ldw r19,  76(sp)                    \n"
    "ldw r20,  80(sp)                    \n"
    "ldw r21,  84(sp)                    \n"
    "ldw r22,  88(sp)                    \n"
    "ldw r23,  92(sp)                    \n"
    "ldw et,   96(sp)                    \n"
    "ldw bt,  100(sp)                    \n"
    "ldw gp,  104(sp)                    \n"
                     /* Don't corrupt sp. */
    "ldw fp,  112(sp)                    \n"
                        /* Don't corrupt ea. */
    "ldw ba,  120(sp)                    \n"
    "ldw ra,  124(sp)                        \n"
    "addi sp, sp, 128                    \n"
    "eret                                \n"

"#else                                   \n"     /* ALT_NO_INSTRUCTION_EMULATION is defined */

   /*
    *  You have waived your right to unimplemented-instruction emulation.
    *
    *  If execution gets here, your program was compiled for a full-featured
    *  Nios II core, but it is running on a smaller core, and instruction
    *  emulation has been disabled by defining ALT_NO_INSTRUCTION_EMULATION.
    *
    *  You can work around the problem by re-enabling instruction emulation,
    *  or you can figure out why your program is being compiled for a system
    *  other than the one that it is running on.
    */

"no_instruction_emulation:                \n"
    "br no_instruction_emulation          \n"

"#end                                     \n"/* ALT_NO_INSTRUCTION_EMULATION */ 


    "trap_handler:                        \n"
   /*
    *  Since there is no trap handler defined, a trap acts like a nop.
    */
    "eret                                 \n"
    );
