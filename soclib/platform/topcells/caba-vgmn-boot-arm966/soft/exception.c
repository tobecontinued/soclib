asm(
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
);
