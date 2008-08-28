asm(
".section .textEXCEP,\"ax\"\n"
"_start:\n"
".global _start \n"
"excep_reset:\n"
	/* vector address + 0x0 */
"	b bootstart\n"
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
".global bootstart\n"
		
"bootstart:\n"
"	bl init_stack\n"
"	bl c_start\n"
"	b bootstart\n"

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
   );


