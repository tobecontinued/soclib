##################################################################################
#	File : giet.s
#	Author : Franck Wajsburt & Alain Greiner
#	Date : 2009/10/15
##################################################################################
#	Interruption/Exception/Trap Handler for MIPS32 processor
#	The base address of the segment containing this code
#	MUST be 0x80000000, in order to have the entry point
#	at address 0x80000180 !!!
#	All messages are printed on the TTY defined by the processor ID.
##################################################################################
#	History : 
#	15/09/2009 The GIET entry point has been modified to comply with
#		the MIPS32 specification : 0x80000180
#	5/10/2009  The Syscall handler has been modified to comply with the
#		MIPS32 specification : the value stored in EPC register is the
#		syscall instruction address => it must be incremented by 4
#		to obtain the return address.
#	15/10/2009 The Interrupt handler has been modified to comply with the
#		VCI_ICU specification : The IRQ index is obtained by a read
#		to icu_base_address + 16.	
##################################################################################

	.section .giet,"ax",@progbits
	.align 2                
	.global interrupt_vector	# makes interrupt_vector an external symbol
	.global _itoa_print		# makes _itoa_print an external symbol

	.extern	seg_icu_base
	.extern	seg_tty_base
	.extern	isr_default

	.extern _procid 
	.extern _proctime
	.extern _tty_write
	.extern _tty_read
	.extern _timer_write
	.extern _timer_read
	.extern _icu_write
	.extern _icu_read
	.extern _dma_write 
	.extern _dma_read 
	.extern _gcd_write 
	.extern _gcd_read 
	.extern _locks_read
	.extern _locks_write
	.extern _exit 

	.ent  giet             
                                                              
################################################################
#	Cause Table (indexed by the Cause register)
################################################################
tab_causes: 
	.word cause_int   	# external interrupt  
	.word cause_ukn    	# undefined exception  
	.word cause_ukn    	# undefined exception
	.word cause_ukn    	# undefined exception
	.word cause_adel   	# illegal address read exception
	.word cause_ades   	# illegal address write exception
	.word cause_ibe    	# instruction bus error exception
	.word cause_dbe    	# data bus error exception
	.word cause_sys    	# system call
	.word cause_bp     	# breakpoint exception
	.word cause_ri     	# illegal codop exception
	.word cause_ukn    	# undefined exception
	.word cause_ovf		# arithmetic overflow exception    
	.word cause_ukn    	# undefined exception
	.word cause_ukn    	# undefined exception
	.word cause_ukn    	# undefined exception

	.space 320	

################################################################
#	Entry point (at address 0x80000180)
################################################################
giet:
	mfc0  	$27, 	$13		# Cause Register analysis
	lui    	$26, 	0x8000		# $26 <= tab_causes  
	andi  	$27, 	$27, 	0x3c    
	addu  	$26, 	$26, 	$27     
	lw    	$26, 	($26)        
	jr    	$26            		# Jump indexed by CR 
 
################################################################
#	System Call Handler
# A system call is handled as a special function call.
# - $2 contains the system call index (< 16).
# - $4, $5, $6, $7 contain the arguments values.
# - The return address (EPC) is saved in the stack.
# - $2, $3, $4, $5, $6, $7 are modified by the system call.
#
# In case of undefinedsystem call, an error message displays 
# the value of EPC on the TTY corresponding to the processor,
# and the user program is killed.
################################################################
cause_sys: 
	addiu 	$29, 	$29, 	-4      # stack pointer move
	mfc0  	$26, 	$14          	# load EPC
	addiu	$26,	$26,	4	# compute return address
	sw    	$26, 	0($29) 		# save it in the stack       
#
#	mfc0  	$26,  	$12          	# load SR
#	li	$27,	0xFFFFFFED	# Mask for UM & EXL bits
#	and  	$26,  	$26, 	$27     # UM <= 0 / EXL = 0
#	mtc0  	$26,  	$12          	# interrupt enabled
#
	andi  	$26,  	$2, 	0xf     # $26 <= syscall index (i < 16) 
	sll   	$26,  	$26, 	2       # $26 <= index * 4
	la    	$27,  	tab_syscalls	# $27 <= syscall vector base
	addu  	$27,  	$27, 	$26       
	lw    	$26,  	($27)         	# get the syscall address
	jalr  	$26                	# jump to the proper syscall
#
#	mfc0  	$26,  	$12          	# load SR
#	li	$27,	0x00000012	# Mask for UM & EXL bits
#	or  	$26,  	$26, 	$27	# UM <= 1 & EXL = 1
#	mtc0  	$26,  	$12         	# interrupt disabled 
#
	lw	$26, 	0($29)        	# return address from stack
	addiu	$29, 	$29, 	4	# stack pointer restore
	mtc0	$26,	$14		# restore EPC
	eret   				# exit GIET 

_sys_ukn: 
	mfc0	$4,	$15, 1
	andi	$4,	$4,	0xFF	# $4 <= processor id
	la    	$5, 	msg_uknsyscall  # $5 <= message address
	li  	$6,	36		# $6 <= message length
	jal   	_tty_write 		# print 
	mfc0	$4,	$15, 1
	andi	$4,	$4,	0xFF	# $4 <= processor id
	la    	$5, 	msg_epc         # $5 <= message address
	li  	$6,	8 		# $6 <= message length
	jal   	_tty_write 		# print
	mfc0  	$4,  	$14 		# $4 <= EPC         
	jal	_itoa_print		# print
	j	_exit 			# end of program

#################################################################
# System Call Table (indexed by syscall index)
tab_syscalls: 
	.word _procid		#0 
	.word _proctime		#1 
	.word _tty_write	#2
	.word _tty_read     	#3
	.word _timer_write	#4
	.word _timer_read	#5
	.word _gcd_write	#6
	.word _gcd_read		#7
	.word _icu_write	#8
	.word _icu_read		#9
	.word _dma_write	#A
	.word _dma_read		#B
	.word _locks_write	#C
	.word _locks_read	#D
	.word _exit 		#E
	.word _sys_ukn  	#F
  
###################################################################
#	Interrupt Handler
# This simple interrupt handler cannot be interrupted.
# It uses an external ICU component (Interrupt controler Unit)
# that concentrates up to 32 interrupts lines to a single IRQ
# line that can be connected to any of the 6 MIPS IT inputs.
# This component returns the highest priority interrupt index
# (smaller indexes have the highest priority).
# Any value larger than 31 means "no active interrupt", and
# the default ISR (that does nothing) is executed.
# The interrupt vector (32 ISR addresses array stored at
# interrupt_vector address) is initialised with the default
# ISR address. The actual ISR addresses are supposed to be written 
# in the interrupt vector array by the boot code.
# Registers $1 to $7, as well as register  $31 and EPC 
# are saved in the interrupted program stack, before calling the 
# Interrupt Service Routine.
# Registers $1 to $7 can be used by the ISR code.
############################################################
cause_int: 
	addiu 	$29,	$29,	-36	# stack space reservation
	.set noat
	sw    	$1,  	0($29)		# save $1     
	.set at
	sw    	$2,  	4($29)		# save $2     
	sw    	$3,  	8($29)		# save $3     
	sw    	$4,  	12($29)		# save $4     
	sw    	$5,  	16($29)		# save $5     
	sw    	$6,  	20($29)		# save $6     
	sw    	$7,  	24($29)		# save $7     
	sw    	$31,  	28($29)		# save $31     
	mfc0	$27,	$14
	sw	$27,	32($29)		# save EPC
	la	$26,	seg_icu_base
	lw	$26,	16($26)		# read interrupt index
	srl	$27,	$26,	5
	bne	$27,	$0,	restore # do nothing if index > 31
	sll	$26,	$26,	2	# index * 4
	la	$27,	interrupt_vector
	addu	$26,	$26,	$27
	lw    	$26, 	($26)		# read ISR address        
	jalr   	$26            		# call ISR 
restore:
	.set noat
	lw    	$1,  	0($29)    	# restore $1
	.set at
	lw    	$2,  	4($29)    	# restore $2
	lw    	$3,  	8($29)    	# restore $3
	lw    	$4,  	12($29)    	# restore $4
	lw    	$5,  	16($29)    	# restore $5
	lw    	$6,  	20($29)    	# restore $6
	lw    	$7,  	24($29)    	# restore $7
	lw    	$31,  	28($29)    	# restore $31
	lw    	$27,  	32($29)    	# return address (EPC)
	addiu	$29,	$29,	36	# restore stack pointer
	mtc0	$27,	$14		# restore EPC
	eret   				# exit GIET 

# Interrupt Vector Table (indexed by interrupt index)
# 32 words corresponding to 32 ISR addresses 
interrupt_vector:
	.word isr_default		# ISR 0
	.word isr_default		# ISR 1
	.word isr_default		# ISR 2
	.word isr_default		# ISR 3
	.word isr_default		# ISR 4
	.word isr_default		# ISR 5
	.word isr_default		# ISR 6
	.word isr_default		# ISR 7
	.word isr_default		# ISR 8
	.word isr_default		# ISR 9
	.word isr_default		# ISR 10
	.word isr_default		# ISR 11
	.word isr_default		# ISR 12
	.word isr_default		# ISR 13
	.word isr_default		# ISR 14
	.word isr_default		# ISR 15
	.word isr_default		# ISR 16
	.word isr_default		# ISR 17
	.word isr_default		# ISR 18
	.word isr_default		# ISR 19
	.word isr_default		# ISR 20
	.word isr_default		# ISR 21
	.word isr_default		# ISR 22
	.word isr_default		# ISR 23
	.word isr_default		# ISR 24
	.word isr_default		# ISR 25
	.word isr_default 		# ISR 26
	.word isr_default 		# ISR 27
	.word isr_default 		# ISR 28
	.word isr_default 		# ISR 29
	.word isr_default 		# ISR 30
	.word isr_default 		# ISR 31

# The default ISR is called when no specific ISR has been installed in the
# interrupt vector. It simply displays a message on TTY 0.

isr_default:
        addiu   $29,    $29,    -4      # get space in stack
        sw      $31,    0($29)          # to save the return address
	mfc0	$4,	$15,	1
	andi	$4,	$4,	0xFF	# $4 <= processor id
        la      $5,     msg_default 	# $5 <= string address
	addi	$6,	$0,	36	# $6 <= string length
        jal     _tty_write              # print
        lw      $31,    0($29)          # restore return address
        addiu   $29,    $29,    4       # free space
        jr      $31                     # returns to interrupt handler

############################################################  
#	Exception Handler
# Same code for all fatal exceptions :
# Print the exception type and the values of EPC & BAR
# on the TTY correspondintg to the processor PROCID,
# and the user program is killed.
############################################################
cause_bp:   
cause_ukn:  
cause_ri:   
cause_ovf:  
cause_adel: 
cause_ades: 
cause_ibe:  
cause_dbe:  
	mfc0  	$26,  	$13       	# $26 <= CR
	andi  	$26,  	$26, 	0x3C    # $26 <= cause_index * 4 
	la    	$27, 	mess_causes	# mess_cause table base address
	addu  	$27,  	$26, 	$27	# pointer on the message base address
	mfc0	$4,	$15,	1
	andi	$4,	$4,	0xFF	# $4 <= processor id
	lw    	$5,  	($27)        	# $5 <= message address
	li	$6,	36		# $6 <= message length
	jal   	_tty_write 		# print  
	mfc0	$4,	$15,	1
	andi	$4,	$4,	0xFF	# $4 <= processor id
	la    	$5, 	msg_epc         # $5 <= message address
	li	$6,	8		# $6 <= message length
	jal   	_tty_write		# print
	mfc0  	$4,  	$14          
	jal   	_itoa_print		# print EPC
	mfc0	$4,	$15,	1
	andi	$4,	$4,	0xFF	# $4 <= processor id
	la    	$5, 	msg_bar         # $5 <= mesage address
	li	$6,	8		# $6 <= message length
	jal   	_tty_write		# print
	mfc0  	$4,  	$8          
	jal   	_itoa_print		# print BAR
	mfc0	$4,	$15,	1
	andi	$4,	$4,	0xFF	# $4 <= processor id
	j     	_exit			# end program       

# Exceptions Messages table (indexed by CAUSE)  
mess_causes:   
	.word msg_ukncause
	.word msg_ukncause
	.word msg_ukncause
	.word msg_ukncause
	.word msg_adel
	.word msg_ades
	.word msg_ibe
	.word msg_dbe
	.word msg_ukncause
	.word msg_bp 
	.word msg_ri 
	.word msg_ukncause
	.word msg_ovf 
	.word msg_ukncause 
	.word msg_ukncause
	.word msg_ukncause

##################################################################
#	All messages
# Messages length are fixed : 8 or 36 characters...
##################################################################  
msg_bar:       	.asciiz "\nBAR  = "  
msg_epc:       	.asciiz "\nEPC  = "  
msg_default: 	.asciiz "\n\n  !!! Default ISR  !!!           \n"
msg_uknsyscall: .asciiz "\n\n  !!! Undefined System Call !!!  \n"     
msg_ukncause:  	.asciiz "\n\nException : strange unknown cause\n"   
msg_adel:      	.asciiz "\n\nException : illegal read address \n"
msg_ades:      	.asciiz "\n\nException : illegal write address\n"
msg_ibe:       	.asciiz "\n\nException : inst bus error       \n"         
msg_dbe:       	.asciiz "\n\nException : data bus error       \n" 
msg_bp:        	.asciiz "\n\nException : breakpoint           \n"        
msg_ri:        	.asciiz "\n\nException : reserved instruction \n"      
msg_ovf:       	.asciiz "\n\nException : arithmetic overflow  \n"       
		.align 2                

################################################################
# The _itoa_print function displays a 32 bits word 
# as a 10 characters hexadecimal string
# - $3 contains the TTY base address
# - $4 contains the 32 bits word
# Registers $2, $4, $5, $6, $7 are modified.
################################################################
itoa_table:	.ascii "0123456789ABCDEF"
itoa_buf:  	.ascii "0x00000000"
		.align 2                

_itoa_print:
	add	$5,	$4,	$0	# $5 <= word to print
	la	$6,	itoa_table	# $6 <= Table address
	la	$4,	itoa_buf + 2	# $4 <= Buffer address
	ori	$7,	$0,	32	# i <= 32
itoa_loop:
	addi	$29,	$29,	-4	# move stack pointer
	sw	$31,	($29)		# save return address
	addi	$7,	$7,	-4	# i <= i -  4
	srlv	$2,	$5,	$7	# $2 <= val >> i	
	andi	$2,	$2,	0xF	# $2 <= 4 bits
	addu	$2,	$6,	$2	# address Table[x]
	lb	$2,	($2)		# load char from Table
	sb	$2,	($4)		# store char to Buffer
	addiu	$4,	$4,	1	# increment Buffer address
	bgtz	$7,	itoa_loop	# exit loop when i==0
	mfc0	$4,	$15
	andi	$4,	$4,	0xFF	# $4 <= processor id
	la	$5,	itoa_buf	# $5 <= Buffer address
	li  	$6,	10		# $6 <= Buffer length
	jal	_tty_write		#print	
	lw	$31,	($29)
	addi	$29,	$29,	4	# restore stack pointer
	jr	$31

	.end giet

