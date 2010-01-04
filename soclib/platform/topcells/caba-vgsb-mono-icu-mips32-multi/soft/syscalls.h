/****************************************************************************************
	File : syscalls.h
 	Written by Alain Greiner & Nicolas Pouillon
	Date : september 2009

	These system calls are used by the MIPS GIET, that is running
	on the MIPS32 processor architecture.

	The supported peripherals are:
	- the SoClib vci_multi_tty
	- the SocLib vci_multi_timer
	- the SocLib vci_dma
	- The SoCLib vci_icu
	- The SoClib vci_locks
	- The SoCLib vci_gcd
****************************************************************************************/

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "tty.h"
#include "dma.h"
#include "gcd.h"
#include "timer.h"
#include "icu.h"

int _procid();
int _proctime();
int _exit();

int _timer_write(int timer_index, int register_index, int value);
int _timer_read(int timer_index, int register_index, int* buffer);

int _tty_write(int tty_index, char* buffer, int	length);
int _tty_read(int tty_index, char* buffer, int length);

int _dma_write(int register_index, int	value);
int _dma_read(int register_index, int*	buffer);

int _icu_write(int register_index, int	value);
int _icu_read(int register_index, int*	buffer);

int _gcd_write(int register_index, int	value);
int _gcd_read(int register_index, int*	buffer);

int _locks_write(int lock_index);
int _locks_read(int lock_index);

#endif

