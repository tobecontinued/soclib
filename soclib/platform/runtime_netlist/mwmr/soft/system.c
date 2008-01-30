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
#include "soclib/timer.h"

void uputs(const char *str)
{
	while (*str)
		putchar(*str++);
}

void puti(const int i)
{
	if ( i>10 )
		puti(i/10);
	putchar(i%10+'0');
}

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	uputs(__FUNCTION__);
	putchar(' ');
	puti(type);
	putchar('\n');
	while(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	uputs(__FUNCTION__);
	putchar('\n');
}

void interrupt_hw_handler(unsigned int irq)
{
	uputs(__FUNCTION__);
	puti(irq);
	putchar('\n');
}
