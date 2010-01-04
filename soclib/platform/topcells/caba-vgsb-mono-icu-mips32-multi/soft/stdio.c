/*********************************************************************
	fichier stdio.c  
	Written Alain greiner & Nicolas Pouillon   
	Date : 19/10/2009

These function implement the drivers for the SoCLib peripherals.
*********************************************************************/

#include <stdarg.h>

#include "stdio.h"

#include "timer.h"
#include "tty.h"
#include "gcd.h"
#include "icu.h"
#include "dma.h"

/*********************************************************************
We define a generic C function to implement all system calls.
*********************************************************************/
inline int  sys_call(	int call_no,
			int arg_0, 
			int arg_1, 
			int arg_2, 
			int arg_3)
{
	register	int	reg_no_and_output asm("v0") = call_no;
	register	int	reg_a0 asm("a0") = arg_0;
	register	int	reg_a1 asm("a1") = arg_1;
	register	int	reg_a2 asm("a2") = arg_2;
	register	int	reg_a3 asm("a3") = arg_3;

	asm volatile(
		"syscall"
		: "=r" (reg_no_and_output) 	// arguments de sortie
		: "r" (reg_a0),			// arguments d'entrée
		  "r" (reg_a1),
		  "r" (reg_a2), 
		  "r" (reg_a3),
	          "r" (reg_no_and_output)
		: "memory", 			// ressources modifiees:
		   "at",
		   "v1",
		   "ra",			// Ces registres persistants seront sauvegardes
		   "t0",			// sur la pile par le compilateur
		   "t1",			// seulement s'ils contiennent des donnees
		   "t2",			// calculees par la fonction effectuant le syscall,
		   "t3",			// et que ces valeurs sont reutilisees par cette
		   "t4",			// fonction au retour du syscall.
		   "t5",
		   "t6",
		   "t7",
		   "t8",
		   "t9"
	);
	return reg_no_and_output;
}

/********************************************************************
	procid()     
Returns the processor ident.
********************************************************************/
int 	procid()
{
	return	sys_call(SYSCALL_PROCID,
			0, 0, 0, 0);
}
/********************************************************************
	proctime()
Returns the local processor time.
********************************************************************/
int	proctime()
{
	return sys_call(SYSCALL_PROCTIME,
			0, 0, 0, 0);
}
/********************************************************************
	exit()     
Exit the program with a TTY message, and enter an infinite loop...
********************************************************************/
int	exit()
{
	int	tty_index = procid();
	return  sys_call(SYSCALL_EXIT, 
			tty_index, 
			0, 0, 0);
}

//////////////////////
//	TTY
//////////////////////

/********************************************************************
	tty_puts()
Display a string on a terminal.
The terminal index is implicitely defined by the processor ID.
The string must be terminated by a NUL character.
This function returns 0 in case of success.
********************************************************************/
int 	tty_puts(char* string)
{
	int	tty_index = procid();
	int 	length = 0;
	while ( string[length] != 0) {
		length++;
	}
	return	sys_call(SYSCALL_TTY_WRITE, 
		tty_index,
		(int)string,
		length,
		0);
}
/********************************************************************
	tty_putc()
Display a single ascii character on a terminal.
The terminal index is implicitely defined by the processor ID.
This function returns 0 in case of success.
********************************************************************/
int  	tty_putc(char byte)
{
	int	tty_index = procid();
	return sys_call(SYSCALL_TTY_WRITE, 
		tty_index,
		(int)(&byte),
		1,
		0);
}
/********************************************************************
	tty_putw()
Display the value of a 32 bits word (decimal characters).
The terminal index is implicitely defined by the processor ID.
This function returns 0 in case of success.
********************************************************************/
int  	tty_putw(int val)
{
	int	tty_index = procid();
	char	buf[10];
	int	i;
	for( i=0 ; i<10 ; i++ ) {
		buf[9-i] = (val % 10) + 0x30;
		val = val / 10;
	}
	return sys_call(SYSCALL_TTY_WRITE, 
		tty_index,
		(int)buf,
		10,
		0);
}
/********************************************************************
 	tty_gets()
Fetch a string from a terminal to a bounded length buffer.
The terminal index is implicitely defined by the processor ID.
Up to (bufsize - 1) characters (including the non printable
characters) will be copied into buffer, and the string is 
completed by a NUL character.
The function returns if a <LF> character is read. 
The function returns 0 if a valid C string is stored in the buffer.
********************************************************************/
int  	tty_gets(char* buf, int bufsize)
{
	int	tty_index = procid();
	int 	ret = sys_call(SYSCALL_TTY_READ,
			tty_index,
			(int)buf,
			(bufsize - 1),
			0);
	if( ret >= 0) {
		buf[ret] = 0;
		return 0;
	} else {
		return -1;
	}
}
/********************************************************************
 	tty_getc()
Fetch a single ascii character from a terminal.
The terminal index is implicitely defined by the processor ID.
The function returns 0 if a valid char is stored in the buffer.
********************************************************************/
int 	tty_getc(char* buf)
{
	int	tty_index = procid();
	return sys_call(SYSCALL_TTY_READ,
			tty_index,
			(int)buf,
			1,
			0);
}
/********************************************************************
	tty_getw()
Fetch a string of decimal characters from a terminal.
The terminal index is implicitely defined by the processor ID.
The characters are written in a 20 characters buffer, 
that can be uncompletely filled if a <CR> character is read.
This string is converted to a int value.
The non decimal characters are ignored.
The function returns 0 if a valid int is stored in the buffer.
********************************************************************/
int	tty_getw(int* word)
{
	int		tty_index = procid();
	unsigned char 	buf[20] = "abcdefghijllmnopqrs\0";
	int 		val = 0;
	int 		i;
	int 		ret = sys_call(SYSCALL_TTY_READ,
					tty_index,
					(int)buf,
					20,
					0);
	if ( ret >= 0 ) {
		for( i=0 ; i<ret ; i++ ) {
			if ( (buf[i] > 0x29) && (buf[i] < 0x40) ) {	
				val = val*10 + (buf[i] - 0x30);
			}
		}
		*word = val;
		return 0;
	} else {
		return -1;
	}
}

/*********************************************************************
	tty_printf()
This function is a simplified version of the mutek_printf() function.
Only a limited number of formats are supported:
	- %d : signed decimal
	- %u : unsigned decimal
	- %x : hexadecimal
	- %c : char
	- %s : string
*********************************************************************/
int 	tty_printf(char *format, ...)
{
	va_list ap;
	va_start(ap, format);

printf_text:

	while (*format) {
      		unsigned int i;
      		for (i = 0; format[i] && format[i] != '%'; i++)
		;
      		if (i) {
			sys_call(SYSCALL_TTY_WRITE,
				procid(),
				(int)format,
				i,0);
	  		format += i;
		}
      		if (*format == '%') {
	  		format++;
	  		goto printf_arguments;
		}
    	} // end while 

	va_end(ap);
    	return 0;

printf_arguments: 

    	{
    	int			val = va_arg(ap, long);
    	char			buf[20];
    	char*			pbuf;	
    	unsigned int		len = 0;
    	static const char	HexaTab[] = "0123456789ABCDEF";
    	unsigned int		i;

    	switch (*format++) {
      	case ('c'): 			// char conversion 
		len = 1;
		buf[0] = val;
		pbuf = buf;
		break;
      	case ('d'): 			// decimal signed integer 
		if (val < 0) {
	    		val = -val;
			sys_call(SYSCALL_TTY_WRITE,
				procid(),
				(int)"-",
				1,0);
	  	}
      	case ('u'): 			// decimal unsigned integer 
        	for( i=0 ; i<10 ; i++) {
        	  	buf[9-i] = HexaTab[val % 10];  
          		if (!(val /= 10)) break;
        	}
		len =  i+1;
		pbuf = &buf[9-i];
		break;
      	case ('x'): 			// hexadecimal integer 
		sys_call(SYSCALL_TTY_WRITE,
			procid(),
			(int)"0x",
			2,0);
        	for( i=0 ; i<8 ; i++) {
          		buf[7-i] = HexaTab[val % 16];  
          		if (!(val /= 16)) break;
        	}
		len =  i+1;
		pbuf = &buf[7-i];
		break;
      	case ('s'):  			// string 
        	{
		char *str = (char*)val;
        	while ( str[len] ) len++;
		pbuf = (char*)val;
        	}	
        	break;
      	default:
		goto printf_text;
      	} // end switch 

	sys_call(SYSCALL_TTY_WRITE,
		procid(),
		(int)pbuf,
		len,0);
    	goto printf_text;
  	}
} // end printf()

///////////////////////
//	TIMER
//////////////////////

/********************************************************************
	timer_set_mode()
********************************************************************/
int 	timer_set_mode(int timer_index, int val) 
{
	return sys_call(SYSCALL_TIMER_WRITE,
			timer_index, 
			TIMER_MODE,
			val,
			0);
}
/********************************************************************
	timer_set_period()
********************************************************************/
int 	timer_set_period(int timer_index, int val) 
{
	return sys_call(SYSCALL_TIMER_WRITE,
			timer_index,
			TIMER_PERIOD,
			val,
			0);
}
/********************************************************************
	timer_reset_irq()
********************************************************************/
int 	timer_reset_irq(int timer_index) 
{
	return sys_call(SYSCALL_TIMER_WRITE,
			timer_index,
			TIMER_RESETIRQ,
			0, 0);
}
/********************************************************************
	timer_get_time()
********************************************************************/
int	timer_get_time(int timer_index, int* time)
{
	return sys_call(SYSCALL_TIMER_READ,
			timer_index,
			TIMER_VALUE,
			(int)time,
			0);
}

///////////////////////
//	GCD
//////////////////////

/********************************************************************
	gcd_set_opa(int val)
Set operand A in the GCD (Greater Common Divider) coprocessor.
********************************************************************/
int 	gcd_set_opa(int val)
{
	return sys_call(SYSCALL_GCD_WRITE,
			GCD_OPA,
			val,
			0, 0);
}
/********************************************************************
	gcd_set_opb(int val)
Set operand B in the GCD (Greater Common Divider) coprocessor.
********************************************************************/
int 	gcd_set_opb(int val)
{
	return sys_call(SYSCALL_GCD_WRITE,
			GCD_OPB,
			val,
			0, 0);
}
/********************************************************************
	gcd_start()
Start computation in the GCD (Greater Common Divider) coprocessor.
********************************************************************/
int	gcd_start(int val)
{
	return sys_call(SYSCALL_GCD_WRITE,
			GCD_START,
			0, 0, 0);
}
/********************************************************************
	gcd_get_status(int* val)
Get status fromn the GCD (Greater Common Divider) coprocessor.
The value is nul when the coprocessor is idle (computation completed)
********************************************************************/
int 	gcd_get_status(int* val)
{
	return sys_call(SYSCALL_GCD_READ,
			GCD_STATUS,
			(int)val,
			0, 0);
}
/********************************************************************
	gcd_get_result(int* val)
Get result fromn the GCD (Greater Common Divider) coprocessor.
********************************************************************/
int 	gcd_get_result(int* val)
{
	return sys_call(SYSCALL_GCD_READ,
			GCD_OPA,
			(int)val,
			0, 0);
}
///////////////////////
//	ICU
//////////////////////

/********************************************************************
	icu_set_mask()
Set some bits in the Interrupt Enable Mask of the ICU component.
Each bit set in the written word will be set in the Mask Enable.
********************************************************************/
int 	icu_set_mask(int val)
{
	return sys_call(SYSCALL_ICU_WRITE,
		ICU_MASK_SET,
		val,
		0, 0);
}
/********************************************************************
	icu_clear_mask()
Reset some bits in the Interrupt Enable Mask of the ICU component.
Each bit set in the written word will be reset in the Mask Enable.
********************************************************************/
int 	icu_clear_mask(int val)
{
	return sys_call(SYSCALL_ICU_WRITE,
		ICU_MASK_CLEAR,
		val,
		0, 0);
}
/********************************************************************
	icu_get_mask()
Read the Interrupt Enable Mask of the ICU component.
********************************************************************/
int 	icu_get_mask(int* buffer)
{
	return sys_call(SYSCALL_ICU_READ,
		ICU_MASK,
		(int)buffer,
		0, 0);
}
/********************************************************************
	icu_get_irqs()
Read the value of the 32 interrupt lines (IRQ inputs).
********************************************************************/
int 	icu_get_irqs(int* buffer)
{
	return sys_call(SYSCALL_ICU_READ,
		ICU_INT,
		(int)buffer,
		0, 0);
}
/********************************************************************
	icu_get_index()
Read the index of the highest priority active interrupt.
(If no active interrupt, -1 is returned).
********************************************************************/
int 	icu_get_index(int* buffer)
{
	return sys_call(SYSCALL_ICU_READ,
		ICU_IT_VECTOR,
		(int)buffer,
		0, 0);
}

///////////////////
// 	DMA
///////////////////

/********************************************************************
	dma_set_source()
Set the source buffer address in the DMA coprocessor.
********************************************************************/
int 	dma_set_source(void *p) 
{
	return sys_call(SYSCALL_DMA_WRITE,
			DMA_SRC,
			(int)p,
			0, 0);
}
/********************************************************************
	dma_set_dest()
Set the destination buffer address in the DMA coprocessor.
********************************************************************/
int 	dma_set_dest(void *p) 
{
	return sys_call(SYSCALL_DMA_WRITE,
			DMA_DST,
			(int)p,
			0, 0);
}
/********************************************************************
	dma_set_length()
Set the operating mode in the DMA coprocessor.
********************************************************************/
int 	dma_set_length(int val) 
{
	return sys_call(SYSCALL_DMA_WRITE,
			DMA_MODE,
			val,
			0, 0);
}
/********************************************************************
	dma_reset
Reset the DMA coprocessor & aknowledge the DMA IRQ.
********************************************************************/
int 	dma_reset() 
{
	return sys_call(SYSCALL_DMA_WRITE,
			DMA_RESET,
			0, 0, 0);
}

///////////////////
//    LOCKS
///////////////////

/********************************************************************
	lock_get()
This system call performs a spin-lock acquisition.
It is dedicated to the SoCLib LOCKS peripheral.
In case of busy waiting, there is a random delay 
of about 100 cycles between two successive lock read, 
to avoid bus saturation.
********************************************************************/
int	lock_get(int lock_index)
{
	return sys_call(SYSCALL_LOCKS_READ,
			lock_index,
			0, 0, 0);
}

/********************************************************************
	lock_release()
You must use this system call to release a spin-lock, 
as the LOCKS peripheral is in the kernel segment.
********************************************************************/
int	lock_release(int lock_index)
{
	return sys_call(SYSCALL_LOCKS_WRITE,
		lock_index,
		0, 0, 0);
}
/********************************************************************
	rand()
Returns the local processor time.
********************************************************************/
int	rand()
{
	int	x = sys_call(SYSCALL_PROCTIME,
			0, 0, 0, 0);
	if((x & 0xF) > 7) 	return (x*x & 0xFFFF);
	else			return (x*x*x & 0xFFFF);
}

