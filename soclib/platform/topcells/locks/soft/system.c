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

#include "system.h"
#include "stdio.h"
#include "soclib/timer.h"

int puts(const char *str)
{
	while (*str)
		putchar(*str++);
	return 0;
}

extern uint32_t lock;

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	lock_lock(&lock);
	printf("%s type=0x%x at 0x%x dptr=0x%x\n", __FUNCTION__, type, (uint32_t)execptr, (uint32_t)dataptr);
	lock_unlock(&lock);
	exit(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	printf("%s irq=%d\n", __FUNCTION__, irq);
	exit(1);
}

void interrupt_hw_handler(unsigned int irq)
{
	printf("%s irq=%d\n", __FUNCTION__, irq);
	exit(1);
}

static inline volatile uint32_t ll( uint32_t *addr )
{
	uint32_t ret;
	__asm__ __volatile__("ll %0, 0(%1)":"=r"(ret):"p"(addr));
	return ret;
}

/*
 * Store conditional, return 0 when succes
 */
static inline volatile uint32_t sc( uint32_t *addr, uint32_t value )
{
	__asm__ __volatile__("sc %0, 0(%1)":"=r"(value):"p"(addr), "0"(value):"memory");
	return !value;
}

uint32_t atomic_inc( uint32_t *addr )
{
	uint32_t val, failed;
	do {
		val = ll(addr);
		val += 1;
		failed = sc(addr, val);
	} while (failed);
	return val;
}

void lock_lock( uint32_t *lock )
{
	uint32_t n = procnum()+1;
	__asm__ __volatile__(
		".set push        \n\t"
		".set noreorder   \n\t"
		".set noat        \n\t"
		"1:               \n\t"
		"ll    $2, 0(%0)  \n\t"
		"beq   $2, %1, 2f \n\t"
		"nop              \n\t"
		"bnez  $2, 1b     \n\t"
		"or    $1, $0, %1 \n\t"
		"sc    $1, 0(%0)  \n\t"
		"beqz  $1, 1b     \n\t"
		"nop              \n\t"
		"2:               \n\t"
		".set pop         \n\t"
		:
		: "p"(lock), "r"(n)
		: "$1", "$2", "memory"
		);
}

void lock_unlock( uint32_t *lock )
{
	*lock = 0;
}
