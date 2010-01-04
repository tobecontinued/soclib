#################################################################################
#	File : reset.s
#	Author : Alain Greiner
#	Date : 15/09/2009
#################################################################################
# 	This is an improved boot code : 
#	- It initializes the Status Register (SR) 
#	- It defines the stack size  and initializes the stack pointer ($29) 
#	- It initializes the interrupt vector with three ISR addresses
#	- It initializes the EPC register, and jumps to the main in user mode.
#################################################################################
		
	.section .reset,"ax",@progbits

	.extern	seg_stack_base
	.extern	seg_data_base
	.extern	main
	.extern _isr_dma
	.extern _isr_tty_get
	.extern _isr_timer0

	.globl  reset	 			# makes reset an external symbol 
	.ent	reset
	.align	2

reset:
       	.set noreorder

# initializes stack pointer
	la	$29,	seg_stack_base
	addiu	$29,	$29,	0x4000	

# initializes interrupt vector
        la      $26,    interrupt_vector        # interrupt vector address
        la      $27,    _isr_timer0             # isr_timer0 address
        sw      $27,    0($26)                  # interrupt_vector[0] <= _isr_timer0
        la      $27,    _isr_tty0_get           # isr_tty_get address
        sw      $27,    4($26)                  # interrupt_vector[1] <= _isr_tty_get
        la      $27,    _isr_dma                # isr_dma address
        sw      $27,    8($26)                  # interrupt_vector[2] <= _isr_dma

# initializes SR register
       	li	$26,	0x0000FF13		# IRQ activation
       	mtc0	$26,	$12			

# jump to main in user mode
	la	$26,	main
	mtc0	$26,	$14
	eret

	.end	reset
	.size	reset, .-reset

	.set reorder
