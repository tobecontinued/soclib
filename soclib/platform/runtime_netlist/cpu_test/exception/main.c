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

int main(void)
{
	int i;
	for (i=0; i<10000; ++i)
		__asm__ __volatile__("nop");
#if defined(__mips__)
	asm volatile("syscall\n\tnop");
#elif defined(__PPC__)
	asm volatile("sc");
#elif defined(__MICROBLAZE__)
    asm volatile("bralid r0, 0x8\n\tnop");
#else
#error Unsupported arch
#endif

	exit(1);
	return 1;
}
