asm(
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
