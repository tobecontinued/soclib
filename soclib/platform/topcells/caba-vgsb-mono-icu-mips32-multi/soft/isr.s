######################################################################################
#	File : isr.s
#	Authors : Alain Greiner & Franck Wajsburt
#	Date : 15/10/2009
######################################################################################
#	These routines must be "intalled" by the boot code in the
#	interrupt vector, depending on the system architecture.
######################################################################################

	.section .isr,"ax",@progbits

######################################################################################
#		_isr_dma
#	This minimal ISR acknoledges the interrupt from DMA
#	and reset the global variable DMA_SYNC for software signaling.
#	Register  $26 is modified.
######################################################################################
	.extern		DMA_SYNC
	.extern		seg_dma_base
	.global		_isr_dma		# makes isr_dma an external symbol
_isr_dma:
	la	$26,	seg_dma_base		# load dma base address
	sw	$0,	0($26)			# reset IRQ 
	la	$26,	DMA_SYNC		# load DMA_SYNC adress
	sw	$0,	0($26)			# reset DMA_SYNC
	jr	$31				# returns to interrupt handler

######################################################################################
#		_isr_timer*
#	This family of four ISRs handle the TIMER[i] IRQs.
#	It acknowledges the IRQ and displays a message on the TTY 
#	corresponding to the procid.
#	Registers $4, $5, $6 are mofified.
######################################################################################
	.extern		_tty_write
	.extern		_itoa_print
	.extern		seg_timer_base
	.global		_isr_timer0		# makes isr_timer0 an external symbol
_isr_timer0:
	addiu	$29,	$29,	-4		# save return address in stack
	sw	$31,	0($29)
	la	$4,	seg_timer_base		# load timer base address
	sw	$0,	0xC($4)			# reset IRQ timer
	mFc0	$4,	$15			# access CP0.procid
	andi	$4,	$4,	0xFF		# $4 <= TTY index
	la	$5,	message_timer0		# $5 <= buffer address
	li	$6,	40			# $6 <= length
	jal	_tty_write			# call _tty_write
	nop
	mfc0	$4,	$9			# get cycle number
	jal	_itoa_print			# print 
	nop
	lw	$31,	0($29)			# restore return address from stack
	addiu	$29,	$29,	4
	jr	$31				# returns to interrupt handler
message_timer0:
	.asciiz "\n!!! interrupt timer0 received at cycle "

	.global		_isr_timer1		# makes isr_timer0 an external symbol
_isr_timer1:
	addiu	$29,	$29,	-4		# save return address in stack
	sw	$31,	0($29)
	la	$4,	seg_timer_base		# load timer base address
	sw	$0,	0x1C($4)		# reset IRQ timer
	mFc0	$4,	$15			# access CP0.procid
	andi	$4,	$7,	0xFF		# $4 <= TTY index
	la	$5,	message_timer1		# $5 <= buffer address
	li	$6,	40			# $6 <= length
	jal	_tty_write			# call _tty_write
	nop
	mfc0	$4,	$9			# get cycle number
	jal	_itoa_print			# print 
	nop
	lw	$31,	0($29)			# restore return address from stack
	addiu	$29,	$29,	4
	jr	$31				# returns to interrupt handler
message_timer1:
	.asciiz "\n!!! interrupt timer1 received at cycle "

	.global		_isr_timer2		# makes isr_timer0 an external symbol
_isr_timer2:
	addiu	$29,	$29,	-4		# save return address in stack
	sw	$31,	0($29)
	la	$4,	seg_timer_base		# load timer base address
	sw	$0,	0x2C($4)		# reset IRQ timer
	mFc0	$4,	$15			# access CP0.procid
	andi	$4,	$7,	0xFF		# $4 <= TTY index
	la	$5,	message_timer2		# $5 <= buffer address
	li	$6,	40			# $6 <= length
	jal	_tty_write			# call _tty_write
	nop
	mfc0	$4,	$9			# get cycle number
	jal	_itoa_print			# print 
	nop
	addiu	$29,	$29,	4
	lw	$31,	0($29)			# restore return address from stack
	addiu	$29,	$29,	4
	jr	$31				# returns to interrupt handler
message_timer2:
	.asciiz "\n!!! interrupt timer2 received at cycle "

	.global		_isr_timer3		# makes isr_timer0 an external symbol
_isr_timer3:
	addiu	$29,	$29,	-4		# save return address in stack
	sw	$31,	0($29)
	la	$4,	seg_timer_base		# load timer base address
	sw	$0,	0x3C($4)		# reset IRQ timer
	mFc0	$4,	$15			# access CP0.procid
	andi	$4,	$7,	0xFF		# $4 <= TTY index
	la	$5,	message_timer3		# $5 <= buffer address
	li	$6,	40			# $6 <= length
	jal	_tty_write			# call _tty_write
	nop
	mfc0	$4,	$9			# get cycle number
	jal	_itoa_print			# print 
	nop
	lw	$31,	0($29)			# restore return address from stack
	addiu	$29,	$29,	4
	jr	$31				# returns to interrupt handler
message_timer3:
	.asciiz "\n!!! interrupt timer3 received at cycle "

######################################################################################
#		_isr_tty*_get
#	This family of ISRs handle the TTY GET IRQs.
#	It writes in the TTY_GET_DATA[i] & TTY_GET_SYNC[i] arrays.
#	There is one ISR per terminal: (i) is the IRQ line index.
#	They implement a simple data lost policy :
#	If the TTY_GET_DATA buffer is not empty (TTY_GET_SYNC[i] != 0),
#	the previous character is overwritten by the new character.
#	The ISR acknowledges the interrupt from TTY by reading in the TTY 
#	DATA register, and writes the character in the TTY_GET_DATA[i] buffer.
#	Then, it set the synchronisation variable TTY_GET_SYNC[i].
#	Registers $26, $27 are modified.
######################################################################################
	.extern		TTY_GET_SYNC
	.extern		TTY_GET_DATA
	.extern		seg_tty_base
	.global		_isr_tty0_get 		# makes _isr_tty0_get an external symbol
_isr_tty0_get:
	la	$26,	seg_tty_base		# load tty base address
	lb	$27,	0x8($26)		# get character & reset IRQ
	la	$26,	TTY_GET_DATA		# load TTY_GET_DATA address
	sb	$27,	0($26)			# writes in TTY_GET_DATAR[0]
	la	$26,	TTY_GET_SYNC		# load TTY_GET_SYNC address
	li	$27,	1			# non-zero value
	sw	$27,	0($26)			# set the TTY_GET_SYNC[0] 
	jr	$31				# returns to interrupt handler

	.global		_isr_tty1_get 		# makes _isr_tty1_get an external symbol
_isr_tty1_get:
	la	$26,	seg_tty_base		# load tty base address
	lb	$27,	0x18($26)		# get character & reset IRQ
	la	$26,	TTY_GET_DATA		# load TTY_GET_DATA base address
	sb	$27,	1($26)			# writes in TTY_GET_DATA[1]
	la	$26,	TTY_GET_SYNC		# load TTY_GET_SYNC base address
	li	$27,	1			# non-zero value
	sw	$27,	4($26)			# set the TTY_GET_SYNC[1] 
	jr	$31				# returns to interrupt handler

	.global		_isr_tty2_get 		# makes _isr_tty1_get an external symbol
_isr_tty2_get:
	la	$26,	seg_tty_base		# load tty base address
	lb	$27,	0x28($26)		# get character & reset IRQ
	la	$26,	TTY_GET_DATA		# load TTY_GET_DATA base address
	sb	$27,	2($26)			# writes in TTY_GET_DATA[2]
	la	$26,	TTY_GET_SYNC		# load TTY_GET_SYNC base address
	li	$27,	1			# non-zero value
	sw	$27,	8($26)			# set the TTY_GET_SYNC[2] 
	jr	$31				# returns to interrupt handler

	.global		_isr_tty3_get 		# makes _isr_tty1_get an external symbol
_isr_tty3_get:
	la	$26,	seg_tty_base		# load tty base address
	lb	$27,	0x38($26)		# get character & reset IRQ
	la	$26,	TTY_GET_DATA		# load TTY_GET_DATA base address
	sb	$27,	3($26)			# writes in TTY_GET_DATA[3]
	la	$26,	TTY_GET_SYNC		# load TTY_GET_SYNC base address
	li	$27,	1			# non-zero value
	sw	$27,	12($26)			# set the TTY_GET_SYNC[3] 
	jr	$31				# returns to interrupt handler

######################################################################################
