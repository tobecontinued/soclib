/*********************************************************************
	fichier userio.c  
	Written Alain greiner 
	Date : 15/09/2009

These function implement the drivers for the SoCLib peripherals.
*********************************************************************/

#include "stdio.h"

volatile int	DMA_SYNC = 0;

volatile int	TTY_GET_SYNC[8] = {0,0,0,0,0,0,0,0};
volatile char	TTY_GET_DATA[8] = {0,0,0,0,0,0,0,0};

volatile int	TTY_PUT_SYNC[8] = {0,0,0,0,0,0,0,0};
volatile char	TTY_PUT_DATA[8] = {0,0,0,0,0,0,0,0};

/*********************************************************************
 	user_getc()
Fetch a single ascii character from the TTY terminal implicitely
defined by the processor ID.
This function uses two arays indexed by the TTY index:
- TTY_GET_DATA[] is written by the hardware and read by the software. 
- TTY_GET_SYNC[] is set by the hardware and reset by the software.
This function is polling the buffer in user mode.
***********************************************************************/
void	user_getc(char* buf)
{
	int	tty_index = procid();
	while ( TTY_GET_SYNC[tty_index] == 0 ) { }	// polling
	*buf = TTY_GET_DATA[tty_index];
	TTY_GET_SYNC[tty_index] = 0;

}
/***********************************************************************
 	user_putc()
Write a single ascii character to the TTY terminal implicitely
defined by the processor ID.
This function uses two arays indexed by the TTY index:
- TTY_PUT_DATA[] is written by the software and read by the hardware. 
- TTY_PUT_SYNC[] is reset by the hardware and set by the software.
This function is polling the buffer in user mode.
*************************************************************************/
void	user_putc(char val)
{
	int	tty_index = procid();
	while ( TTY_PUT_SYNC[tty_index] != 0 ) { }	// polling
	TTY_PUT_DATA[tty_index] = val;
	TTY_PUT_SYNC[tty_index] = 1;

}
