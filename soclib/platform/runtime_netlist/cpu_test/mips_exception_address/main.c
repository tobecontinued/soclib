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

#include "soclib/simhelper.h"

#include "segmentation.h"

void *bug_addr = 0;

int main(void)
{
	int i;
	void *undef = 0xffff0000;

	printf("Test begins\n");
	for ( i=0; i<1000; i++ )
		asm volatile("nop");

	printf("test2\n");

	asm volatile(
		"la  %0, the_bug_address     \n\t"
		"sw %0, (%1)                \n\t"
		"the_bug_address:       \n\t"
		"lbu %0, 0(%2)     \n\t"
		"bnez %0,  1f     \n\t"
		"nop \n\t"
		"1: \n\t"
		: "=&r"(i)
		: "r"(&bug_addr), "r"(undef)
		: "memory"
	);

	printf("Test returns\n");

	exit(1);
	return 1;
}
