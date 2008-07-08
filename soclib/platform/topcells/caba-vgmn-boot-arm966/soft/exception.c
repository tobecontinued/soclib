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

".section .textEXCEP,\"ax\"\n"
"excep_reset:\n"
	/* vector address + 0x0 */
"	b excep_reset\n"
"excep_undef:\n"
	/* vector address + 0x4 */
"	b excep_undef\n"
"excep_swi:\n"
	/* vector address + 0x8 */
"	b swi_handler\n"
"excep_prefetch:\n"
	/* vector address + 0xc */
"	b excep_prefetch\n"
"excep_data:\n"
	/* vector address + 0x10 */
"	b excep_data\n"
"reserved_excep_address:\n"
	/* reserved exception address */
"	b reserved_excep_address\n"
"excep_irq:\n"
	/* vector address + 0x18 */
"	b excep_irq\n"
"excep_fiq:\n"
	/* vector address + 0x1c */
"	b excep_fiq\n"
/* the different exception handlers appear here */
"swi_handler:\n"
"	movs pc, r14\n"

".section .textCODE,\"ax\"\n"
".global init\n"
		
"init:\n"
"	bl check_cp15\n"
	// bl check_swi
"	bl init_stack\n"
"	bl c_start\n"
"	b init\n"

"check_cp15:\n"
	/* 0x41259660 */
"	eor r0, r0, r0\n"
"	eor r1, r1, r1\n"
"	eor r2, r2, r2\n"
"	eor r3, r3, r3\n"
"	eor r4, r4, r4\n"
"	eor r5, r5, r5\n"
"	eor r6, r6, r6\n"
"	add r0, r0, #1\n"
"	mrc p15, 0, r1, c0, c0, 0\n"
"	ldr r2, id_code_reg\n"
"	cmp r1, r2\n"
"	bne L1_check_cp15\n"
	/* 0x440440 */
"	eor r0, r0, r0\n"
"	add r0, r0, #2\n"
"	mrc p15, 0, r3, c0, c0, 2\n"
"	ldr r2, tcm_size_reg\n"
"	cmp r3, r2\n"
"	bne L1_check_cp15\n"
"	mov pc, lr\n"
"L1_check_cp15:\n"
"	b L1_check_cp15\n"

"check_swi:\n"
"	swi #0\n"
"	mov pc, lr\n"

"init_stack:\n"
	/* initialize stack register
	 * and stack memory */
"	ldr sp, init_stack_addr\n"
"	ldr r1, init_stack_addr + 4\n"
"	eor r0, r0, r0\n"
"L1_init_stack:\n"
"	str r0, [r1]\n"
"	add r1, r1, #4\n"
"	cmp r1, sp\n"
"	bne L1_init_stack\n"
"	mov pc, lr\n"
	
"init_stack_addr:\n"
"	.long __stack_start\n"
"	.long __stack_end\n"
"	.long __stack_size\n"

"id_code_reg: \n"
"	.long	0x41259660\n"
"tcm_size_reg:\n"
"	.long   0x00440440\n"

".end\n"
);
