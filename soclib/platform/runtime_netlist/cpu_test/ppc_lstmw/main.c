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

void cpy_10_regs( void*, void* );

int main(void)
{
	int i;
	uint32_t test[10];

	cpy_10_regs(test, main);

	for ( i=0; i<10; ++i ) {
		assert( test[i] == ((uint32_t *)main)[i] && "cpy_10_reg is bad" );
	}

	for ( i=0; i<10; ++i )
		test[i] = 0;

	cpy_10_regs2(test, main);

	for ( i=0; i<10; ++i ) {
		assert( test[i] == ((uint32_t *)main)[i] && "cpy_10_reg2 is bad" );
	}

	exit(0);
	while(1)
		;
}
