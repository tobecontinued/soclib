/*********************************************************************************
	fichier stdio.h  
	Written Alain greiner & Nicolas Pouillon   
	Date : 15/09/2009
*********************************************************************************/

#ifndef _STDIO_H_
#define _STDIO_H_

#define SYSCALL_PROCID		0x0
#define SYSCALL_PROCTIME	0x1
#define SYSCALL_TTY_WRITE	0x2
#define SYSCALL_TTY_READ 	0x3
#define SYSCALL_TIMER_WRITE	0x4
#define SYSCALL_TIMER_READ 	0x5
#define SYSCALL_GCD_WRITE	0x6
#define SYSCALL_GCD_READ	0x7
#define SYSCALL_ICU_WRITE	0x8
#define SYSCALL_ICU_READ	0x9
#define SYSCALL_DMA_WRITE	0xA
#define SYSCALL_DMA_READ	0xB
#define SYSCALL_LOCKS_WRITE	0xC
#define SYSCALL_LOCKS_READ	0xD
#define SYSCALL_EXIT    	0xE

/****************************************************************
this is a generic C function to implement all system calls.
- The first argument is the system call index.
- The four next arguments are the system call arguments.
They will be written in registers $2, $4, $5, $6, $7.
****************************************************************/
int    sys_call(int call_no, 
		int arg_o, 
		int arg_1, 
		int arg_2, 
		int arg_3);

/****************************************************************
These functions access the MIPS protected registers
****************************************************************/
int 	procid();
int 	proctime();
int 	exit();
int	rand();

/****************************************************************
These functions access the MULTI_TTY peripheral
****************************************************************/
int 	tty_puts(char* string);
int  	tty_putc(char byte);
int 	tty_putw(int word);
int 	tty_gets(char* buf, int bufsize);
int 	tty_getc(char* byte);
int 	tty_getw(int* word);
int	tty_printf(char* format,...);

/****************************************************************
These functions access the MULTI_TIMER peripheral
****************************************************************/
int	timer_set_mode(int timer_index, int val);
int	timer_set_period(int timer_index, int val);
int	timer_reset_irq(int timer_index);
int	timer_get_time(int timer_index, int* time);

/****************************************************************
These functions access the GCD peripheral
****************************************************************/
int	gcd_set_opa(int val);
int	gcd_set_opb(int val);
int	gcd_start();
int	gcd_get_result(int* val);
int	gcd_get_status(int* val);

/****************************************************************
These functions access the ICU peripheral
****************************************************************/
int	icu_set_mask(int val);
int	icu_clear_mask(int val);
int	icu_get_mask(int* buffer);
int	icu_get_irqs(int* buffer);
int	icu_get_index(int* buffer);
		
/****************************************************************
These functions access the DMA peripheral
****************************************************************/
int    	dma_set_source(void *p);
int    	dma_set_dest(void *p);
int    	dma_set_length(int val);
int    	dma_reset();

/****************************************************************
These functions access the LOCKS peripheral
****************************************************************/
int	lock_get(int lock_index);
int	lock_release(int lock_index);

#endif
