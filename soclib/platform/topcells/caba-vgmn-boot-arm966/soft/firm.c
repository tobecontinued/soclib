asm(
".section .textFIRM,\"ax\" \n"
	
"_start: \n"
"	.global _start \n"
"	b bootstart \n"
	
"bootstart:\n"
"	bl init_tcm\n"
"	bl copy_kernel\n"
"	bl copy_excep\n"
"	bl launch_kernel\n"
"	b bootstart\n"

"init_tcm:\n"
"	eor r1, r1, r1\n" 		/* clear mask */
"	add r1, r1, #0x04\n"	/* set DTCM enable in mask */
"	add r1, r1, #0x1000\n"	/* set ITCM enable in mask */
"	mrc p15, 0, r2, c1, c0, 0\n" /* load cp15 control register */
"	orr r3, r2, r1\n" 		/* apply mask */
"	mcr p15, 0, r3, c1, c0, 0\n" /*  set new cp15 control register */
"	mov pc, lr\n"

"copy_chunk:\n"
	/* this method expects some parameters in the registers:
	 * r1 = source address
	 * r2 = target address
	 * r3 = size of the chunk to copy
	 */
	/* copy using r4 as buffer:	
	 * - decrement r3 
	 * - load address(r1 + r3) into r4
	 * - store r4 into address(r2 + r3)
	 * - if r3 == 0 copy finished, return
	 */
"	sub r3, r3, #4\n"
"	ldr r4, [r1, +r3]\n"
"	str r4, [r2, +r3]\n"
"	cmp r3, #0\n"
"	bne copy_chunk\n"
"	mov pc, lr\n"
	
"v_textEXCEP:\n"
"	.long __textEXCEP_rom_start\n"
"	.long __textEXCEP_start\n"
"	.long __textEXCEP_size\n"
"copy_excep:\n"
	/* r1 contains the source address */
	/* r2 contains the target address */
	/* r3 contains the copy size */
"	ldr r1, v_textEXCEP\n"
"	ldr r2, v_textEXCEP + 4\n"
"	ldr r3, v_textEXCEP + 8\n"
"	mov r10, lr\n"
"	bl copy_chunk\n"
	/* enable the exceptions */
"	eor r1, r1, r1\n"	/* clear mask */
"	add r1, r1, #0x2000\n"	/* set alternate vector select in the mask */
"	mrc p15, 0, r2, c1, c0, 0\n" /* load cp15 control register */
"	eor r3, r2, r1\n" 		/* apply mask */
"	mcr p15, 0, r3, c1, c0, 0\n" /*  set new cp15 control register */
"	mov lr, r10\n"
"	mov pc, lr\n"

"v_textCODE:\n"
"	.long __textCODE_rom_start\n"
"	.long __textCODE_start\n"
"	.long __textCODE_size\n"
"copy_kernel:\n"
	/* r1 contains the source address */
	/* r2 contains the target address */
	/* r3 contains the copy size */
"	ldr r1, v_textCODE\n"
"	ldr r2, v_textCODE + 4\n"
"	ldr r3, v_textCODE + 8\n"
"	mov r10, lr\n"
"	bl copy_chunk\n"
"	mov lr, r10\n"
"	mov pc, lr\n"
	
"launch_kernel:\n"
"	eor r1, r1, r1\n"
"	add r1, r1, #0x2000\n"
"	mov pc, r1\n"
);
