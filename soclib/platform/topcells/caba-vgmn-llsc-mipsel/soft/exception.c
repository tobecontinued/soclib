/*
    This file is part of MutekH.

    MutekH is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MutekH is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MutekH; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

    Copyright Alexandre Becoulet <alexandre.becoulet@lip6.fr> (c) 2006

*/

asm(
    ".section        .excep,\"ax\",@progbits			\n"

#if SOCLIB_MIPS32
	".space 0x180    \n"
#elif SOCLIB_MIPS_R3000
	".space 0x80    \n"
#else
#error Unknown mips flavor
#endif

    ".globl mips_interrupt_entry				\n"
    "mips_interrupt_entry:					\n"
    ".set push							\n"
    ".set noat							\n"

    /* save registers */
    "	addu	$sp,	-4*32				\n"

    "	sw	$1,	 1*4($sp)		        \n" /* AT reg */

    "	sw	$2,	 2*4($sp)		        \n" /* Return value regs */
    "	sw	$3,	 3*4($sp)		        \n" /* Return value regs */

    "	mfc0	$1,	$13				\n" /* read Cause */

    "	sw	$4,	 4*4($sp)		        \n" /* Args regs */
    "	sw	$5,	 5*4($sp)		        \n" /* Args regs */
    "	sw	$6,	 6*4($sp)		        \n" /* Args regs */
    "	sw	$7,	 7*4($sp)		        \n" /* Args regs */

    "	sw	$8,	 8*4($sp)		        \n" /* Temp regs */
    "	sw	$9,	 9*4($sp)		        \n" /* Temp regs */
    "	sw	$10,	10*4($sp)		        \n" /* Temp regs */
    "	sw	$11,	11*4($sp)		        \n" /* Temp regs */

    "	mfc0	$8,	$14				\n" /* read EPC */

    "	sw	$12,	12*4($sp)		        \n" /* Temp regs */
    "	sw	$13,	13*4($sp)		        \n" /* Temp regs */
    "	sw	$14,	14*4($sp)		        \n" /* Temp regs */
    "	sw	$15,	15*4($sp)		        \n" /* Temp regs */
    "	sw	$24,	24*4($sp)		        \n" /* Temp regs */
    "	sw	$25,	25*4($sp)		        \n" /* Temp regs */

    "	sw	$31,	31*4($sp)		        \n" /* Return address regs */

    "	sw	$8,	0*4($sp)		        \n" /* EPC reg */

    "	andi	$9,	$1,	0x3c			\n" /* extract cause */
    "	beq	$9,	$0,	interrupt_hw		\n"
    "	li	$10,	(8<<2)				\n"
    "	beq	$9,	$10,	interrupt_sys		\n"

    /*************************************************************
		exception handling
    **************************************************************/

    "interrupt_ex:					\n"

    /* add missing registers in reg table */
    "	sw	$16,	16*4($sp)		        \n"
    "	sw	$17,	17*4($sp)		        \n"
    "	sw	$18,	18*4($sp)		        \n"
    "	sw	$19,	19*4($sp)		        \n"
    "	sw	$20,	20*4($sp)		        \n"
    "	sw	$21,	21*4($sp)		        \n"
    "	sw	$22,	22*4($sp)		        \n"
    "	sw	$23,	23*4($sp)		        \n"
    "	sw	$26,	26*4($sp)		        \n"
    "	sw	$27,	27*4($sp)		        \n"
    "	sw	$28,	28*4($sp)		        \n"
    "	sw	$29,	29*4($sp)		        \n"
    "	sw	$30,	30*4($sp)		        \n"

    /* exception function arguments */
    "	srl	$4,	$9,	2			\n" /* adjust cause arg */
    "	move	$5,	$8				\n" /* execution pointer */
    "	mfc0	$6,	$8				\n" /* bad address if any */
    "	addiu	$7,	$sp,	0			\n" /* register table on stack */
    "	addiu	$8,	$sp,				\n" /* stack pointer */

    "	sw	$8,	4*4($sp)			\n"
    "	la		$1,	interrupt_ex_handler	\n"
    "	jalr	$1					\n"

    "	j	return					\n"

    /*************************************************************
		syscall handling
    **************************************************************/
    "interrupt_sys:					\n"

    "	la		$1,	interrupt_sys_handler	\n"
    "	jalr	$1					\n"

    "	j	return_val				\n"

    /*************************************************************
		hardware interrupts handling
    **************************************************************/
    "interrupt_hw:					\n"

    "	srl	$4,	$1,	8			\n"
    "	andi	$4,	$4,	0xff			\n"

    "	la		$1,	interrupt_hw_handler	\n"
    "	jalr	$1					\n"

    /************************************************************/

    /* restore registers */
    "return:						\n"
    "	lw	$2,	 2*4($sp)		        \n"
    "	lw	$3,	 3*4($sp)		        \n"

    "return_val:					\n"
    "	lw	$1,	 1*4($sp)		        \n"

    "	lw	$26,	 0*4($sp)		        \n" /* EPC reg */

    "	lw	$4,	 4*4($sp)		        \n"
    "	lw	$5,	 5*4($sp)		        \n"
    "	lw	$6,	 6*4($sp)		        \n"
    "	lw	$7,	 7*4($sp)		        \n"

    "	lw	$8,	 8*4($sp)		        \n"
    "	lw	$9,	 9*4($sp)		        \n"
    "	lw	$10,	10*4($sp)		        \n"
    "	lw	$11,	11*4($sp)		        \n"
    "	lw	$12,	12*4($sp)		        \n"
    "	lw	$13,	13*4($sp)		        \n"
    "	lw	$14,	14*4($sp)		        \n"
    "	lw	$15,	15*4($sp)		        \n"

    "	lw	$24,	24*4($sp)		        \n"
    "	lw	$25,	25*4($sp)		        \n"

    "	lw	$31,	31*4($sp)		        \n"

    "	addu	$sp,	4*32				\n"

    ".set noreorder					\n"
    "	jr	$26					\n"
    "	rfe						\n"

    ".set pop						\n"
    );

asm(
    ".section        .reset,\"ax\",@progbits		\n"

    ".globl cpu_boot					\n"
    "cpu_boot:						\n"

    ".set push						\n"
    ".set noreorder					\n"

    "li        $8,   0x0000FF15  			\n"
    "mtc0      $8,	$12         			\n"

    /* get CPU id and adjust stack */

    "mfc0	$8,	$15, 1				\n"
    "la         $sp,	_stack - 16	\n"
	"andi   $8, $8, 0x3ff		\n"
    "sll	$8,	$8,	10		\n"
    "subu	$sp,	$sp,	$8		\n"

    /* setup global data pointer */
    "la	   $gp,   _gp					\n"

    "la         $8,   main				\n"
    "j          $8					\n"
    "nop						\n"
    "							\n"
    ".set pop						\n"
    );

