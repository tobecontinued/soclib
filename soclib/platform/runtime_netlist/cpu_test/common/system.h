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

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "soclib/tty.h"
#include "segmentation.h"

#define base(x) (void*)(x##_BASE)

#if __mips__

#if __mips >= 32
#define get_cp0(x, sel)									\
({unsigned int __cp0_x;								\
__asm__("mfc0 %0, $"#x", "#sel:"=r"(__cp0_x));	\
__cp0_x;})
#else
#define get_cp0(x, sel)									\
({unsigned int __cp0_x;								\
__asm__("mfc0 %0, $"#x:"=r"(__cp0_x));	\
__cp0_x;})
#endif

static inline int procnum()
{
    return (get_cp0(15, 1)&0x3ff);
}

#elif __PPC__

#define dcr_get(x)					\
({unsigned int __val;				\
__asm__("mfdcr %0, "#x:"=r"(__val));\
__val;})

#define spr_get(x)					\
({unsigned int __val;				\
__asm__("mfspr %0, "#x:"=r"(__val));\
__val;})

static inline int procnum()
{
    return dcr_get(0);
}

#elif __MICROBLAZE__


#define fsl_get(x)				  \
({unsigned int __val;			  \
__asm__("get %0, RFSL"#x:"=r"(__val));\
__val;})

static inline uint32_t fsl_getd(uint32_t no)
{
    uint32_t val;
    __asm__("getd %0, %1"
            : "=r"(val)
            : "r"(no)
        );
    return val;
}

static inline int procnum()
{
    return fsl_get(0);
}

#elif defined(__arm__)

static inline int procnum()
{
	int i;
	asm (
		"mrc p15,0,%0,c0,c0,5"
		: "=r" (i)
		);
	return i;
}

#endif /* platform switch */

static inline int putchar(const int x)
{
	soclib_io_write8(
		base(TTY),
		procnum()*TTY_SPAN+TTY_WRITE,
		(char)x);
	return (char)x;
}

static inline int getchar()
{
	return soclib_io_read8(
		base(TTY),
		procnum()*TTY_SPAN+TTY_READ);
}

void exit(int retval);

#endif /* SYSTEM_H_ */
