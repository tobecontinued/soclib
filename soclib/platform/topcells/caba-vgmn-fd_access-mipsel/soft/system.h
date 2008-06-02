/*
 *
 * SOCLIB_GPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU GPLv2.
 * 
 * SoCLib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 * 
 * SOCLIB_GPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#ifndef USER_H_
#define USER_H_

#include "stdint.h"
#include "soclib/tty.h"
#include "soclib/fd_access.h"
#include "../segmentation.h"

#define base(x) (void*)(x##_BASE)

void uputs(const char *);

#ifdef __mips__

#define get_cp0(x, sel)									\
({unsigned int __cp0_x;								\
__asm__("mfc0 %0, $"#x", "#sel:"=r"(__cp0_x));	\
__cp0_x;})

static inline int procnum()
{
    return (get_cp0(15,1)&0x3ff);
}

#endif

static inline void putc(const char x)
{
	soclib_io_write8(
		base(TTY),
		procnum()*TTY_SPAN+TTY_WRITE,
		x);
}

static inline char getc()
{
	return soclib_io_read8(
		base(TTY),
		procnum()*TTY_SPAN+TTY_READ);
}

#endif

extern int errno;

int open( const char *path, const int how, const int mode );
int close( const int fd );
int read( const int fd, const void *buffer, const size_t len );
int write( const int fd, const void *buffer, const size_t len );
int lseek( const int fd, const off_t offset, int whence );
