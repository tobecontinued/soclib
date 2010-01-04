/****************************************************************************************
	File : syscalls.c
 	Written by Alain Greiner & Nicolas Pouillon
	Date : september 2009

	Basic System Calls for peripheral access.
	These system calls are used by the MIPS GIET, that is running
	on the MIPS32 processor architecture.

	The supported peripherals are:
	- the SoClib vci_multi_tty
	- the SocLib vci_timer
	- the SocLib vci_dma
	- The SoCLib vci_icu
	- The SoClib vci_locks
	- The SoCLib vci_gcd
****************************************************************************************/

#include "syscalls.h"

struct plouf;

extern struct plouf seg_timer_base;
extern struct plouf seg_tty_base;
extern struct plouf seg_gcd_base;
extern struct plouf seg_dma_base;
extern struct plouf seg_locks_base;
extern struct plouf seg_icu_base;

extern struct plouf max_timer_number;
extern struct plouf max_tty_number;
extern struct plouf max_locks_number;

#define in_giet __attribute__((section (".syscalls")))

////////////////////////////////////////////////////////////////////////////////////////
//	_procid()
// Access CP0 and returns processor ident
////////////////////////////////////////////////////////////////////////////////////////
in_giet int _procid()
{
	int ret;
	asm volatile( "mfc0 %0, $15, 1": "=r"(ret) );
	return (ret & 0xFF);
} 

////////////////////////////////////////////////////////////////////////////////////////
//	_proctime()
// Access CP0 and returns processor time
////////////////////////////////////////////////////////////////////////////////////////
in_giet int _proctime()
{
	int ret;
	asm volatile( "mfc0 %0, $9": "=r"(ret) );
	return ret;
} 
///////////////////////////////////////////////////////////////////////////////////////
//	_timer_write()
// Write a 32 bits word in a memory mapped register of the VCI_MULTI_TIMER
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_timer_write(	int 	timer_index,
				int	register_index,
				int	value)
{
	volatile int*	timer_address;
	int		max;

	max = (int)&max_timer_number;
	if( (timer_index >= max) || (register_index >= 4) ) return -1;

	timer_address = (int*)(void*)&seg_timer_base + (timer_index*TIMER_SPAN);
	timer_address[register_index] = value;			// write word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_timer_read()
// Read a 32 bits word in a memory mapped register of the VCI_MULTI_TIMER
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_timer_read(	int 	timer_index,
				int	register_index,
				int*	buffer)
{
	volatile int*	timer_address;
	int		max;

	max = (int)&max_timer_number;
	if( (timer_index >= max) || (register_index >= 4) ) return -1;

	timer_address = (int*)(void*)&seg_timer_base + (timer_index*TIMER_SPAN);
	*buffer = timer_address[register_index];		// read word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_tty_write()
// Write characters from a fixed length buffer to a terminal of the VCI_MULTI_TTY
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_tty_write(	int 	tty_index,
				char*	buffer,
				int	length)
{
	volatile char*	tty_address;
	int		max;
	int		i;

	max = (int)&max_tty_number;
	if( tty_index >= max ) return -1;

	tty_address = (char*)(void*)&seg_tty_base + (tty_index*TTY_SPAN*4);
	for ( i=0 ; i < length ; i++ )  {
		tty_address[TTY_WRITE*4] = buffer[i];		// write character
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_tty_read()
// Fetch characters to a fixed length buffer from a terminal of the VCI_MULTI_TTY
// This is a blocking call : it contains a loop that reads the TTY status
// register until a character is typed on the keyboard.
// Exits if a <LF> is read or when the destination buffer is full.
// The special <BS> character cancel the previous typed character. 
// Returns the number of characters actually written in the buffer.
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_tty_read(	int 	tty_index,
				char*	buffer,
				int	length)
{
	volatile char*	tty_address;
	int		max;
	int		i;
	char		byte;

	max = (int)&max_tty_number;
	if( tty_index >= max ) return -1;

	tty_address = (char*)(void*)&seg_tty_base + (tty_index*TTY_SPAN*4);
	for ( i=0 ; i < length ; i++ )  {
		while ( tty_address[TTY_STATUS*4] == 0 ) { 	// read status
			// busy waiting
		}
		byte = tty_address[TTY_READ*4];			// read character
		if(byte == 0x08) {				// test backspace
			if (i > 0)	i=i-2;
			else		i=i-1;
		} else {
			buffer[i] = byte;			// write to buffer
			if( byte == 0x0A ) break;		// test <LF>
		}
	}
	return (i+1); // number of written characters
}
///////////////////////////////////////////////////////////////////////////////////////
//	_exit()
// Exit (suicide) after printing message on  a TTY terminal.
///////////////////////////////////////////////////////////////////////////////////////
in_giet int	_exit()
{
	char  	buf[] = "\n\n!!!  Exit  Processor          !!!\n";
	int	pid = _procid();
	int	max = (int)&max_tty_number;

	if(pid < max) {
		buf[24] = '0';
		buf[25] = 'x';
		buf[26] = (char)((pid>>8) & 0xF) + 0x30;
		buf[27] = (char)((pid>>4) & 0xF) + 0x30;
		buf[28] = (char)(pid & 0xF)      + 0x30;
		_tty_write(pid, buf, 36);
	}

	while(1) {
		asm volatile("nop");				// infinite loop...
	}
}
///////////////////////////////////////////////////////////////////////////////////////
//	_dma_write()
// Write a 32 bits word in a memory mapped register of the DMA coprocesor
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_dma_write(	int 	register_index,
				int	value)
{
	volatile	int*	dma_address;
	if( register_index >= 5 ) return -1;

	dma_address = (int*)(void*)&seg_dma_base;
	dma_address[register_index] = value;			// write word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_dma_read()
// Read a 32 bits word in a memory mapped register of the DMA coprocessor
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_dma_read(	int 	register_index,
				int*	buffer)
{
	volatile	int*	dma_address;
	if( register_index >= 5 ) return -1;

	dma_address = (int*)(void*)&seg_dma_base;
	*buffer = dma_address[register_index];			// read word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_icu_write()
// Write a 32 bits word in a memory mapped register of the ICU peripheral
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_icu_write(	int 	register_index,
				int	value)
{
	volatile	int*	icu_address;
	if( register_index >= 4 ) return -1;

	icu_address = (int*)(void*)&seg_icu_base;
	icu_address[register_index] = value;			// write word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_icu_read()
// Read a 32 bits word in a memory mapped register of the ICU peripheral
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_icu_read(	int 	register_index,
				int*	buffer)
{
	volatile	int*	icu_address;
	if( register_index >= 4 ) return -1;

	icu_address = (int*)(void*)&seg_icu_base;
	*buffer = icu_address[register_index];			// read word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_gcd_write()
// Write a 32 bits word in a memory mapped register of the GCD coprocessor
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_gcd_write(	int 	register_index,
				int	value)
{
	volatile	int*	gcd_address;
	if( register_index >= 4 ) return -1;

	gcd_address = (int*)(void*)&seg_gcd_base;
	gcd_address[register_index] = value;			// write word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_gcd_read()
// Read a 32 bits word in a memory mapped register of the GCD coprocessor
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_gcd_read(	int 	register_index,
				int*	buffer)
{
	volatile	int*	gcd_address;
	if( register_index >= 4 ) return -1;

	gcd_address = (int*)(void*)&seg_gcd_base;
	*buffer = gcd_address[register_index];			// read word
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_locks_write()
// Release a spin-lock in the LOCKS peripheral
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_locks_write(	int 	lock_index)
			
{
	volatile	int*	locks_address;
	int		max;

	max = (int)&max_locks_number;
	if( lock_index >= max ) return -1;

	locks_address = (int*)(void*)&seg_locks_base;
	locks_address[lock_index] = 0;				// write 0
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
//	_locks_read()
// Try to take a spin-lock in the LOCKS peripheral.
// This is a blocking call, as there is a busy-waiting loop,
// until the lock is granted to the requester.
// There is an internal delay of about 100 cycles between
// two successive lock read, to avoid bus saturation.
///////////////////////////////////////////////////////////////////////////////////////
in_giet int 	_locks_read(	int 	lock_index)
{
	volatile	int*	locks_address;
	int		max;
	int		delay;
	int		i;

	max = (int)&max_locks_number;
	if( lock_index >= max ) return -1;

	locks_address = (int*)(void*)&seg_locks_base;

	while( locks_address[lock_index] != 0) {
		delay = _proctime() & 0xFF; 		// busy waiting
		for( i=0 ; i<delay ; i++) {		// with a pseudo random
			asm volatile("nop");		// delay between bus access
		}
	}
	return 0;
}


