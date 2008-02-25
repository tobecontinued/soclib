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

void test( uint32_t addr, uint32_t val );

typedef union {
	uint32_t i[2];
	struct {
		uint32_t i0;
		char p00;
		char p01;
		char p02;
		char p03;
	} __attribute__((packed));
	struct {
		char p10;
		uint32_t i1;
		char p11;
		char p12;
		char p13;
	} __attribute__((packed));
	struct {
		char p20;
		char p21;
		uint32_t i2;
		char p22;
		char p23;
	} __attribute__((packed));
	struct {
		char p30;
		char p31;
		char p32;
		uint32_t i3;
		char p33;
	} __attribute__((packed));
	struct {
		char p40;
		char p41;
		char p42;
		char p43;
		uint32_t i4;
	} __attribute__((packed));
} truc_t;

int main(void)
{
	{
		volatile uint8_t test_b[12];

		*(uint32_t*)&test_b[0] = 0x11223344;
		*(uint32_t*)&test_b[4] = 0x55667788;
		test((uint32_t)&test_b[0+1], 0x11223344);
#if defined MIPSEL
		test((uint32_t)&test_b[1+1], 0x88112233);
		test((uint32_t)&test_b[2+1], 0x77881122);
		test((uint32_t)&test_b[3+1], 0x66778811);
#elif defined MIPSEB
		test((uint32_t)&test_b[1+1], 0x22334455);
		test((uint32_t)&test_b[2+1], 0x33445566);
		test((uint32_t)&test_b[3+1], 0x44556677);
#else
#error Not supported
#endif
		test((uint32_t)&test_b[4+1], 0x55667788);
	}

	{
		volatile truc_t a;
		a.i[0] = 0x11223344;
		a.i[1] = 0x55667788;
		
		printf("%08x\n", a.i0);
		printf("%08x\n", a.i1);
		printf("%08x\n", a.i2);
		printf("%08x\n", a.i3);
		printf("%08x\n", a.i4);

		assert(a.i0 == 0x11223344);
#if defined MIPSEL
		assert(a.i1 == 0x88112233);
		assert(a.i2 == 0x77881122);
		assert(a.i3 == 0x66778811);
#elif defined MIPSEB
		assert(a.i1 == 0x22334455);
		assert(a.i2 == 0x33445566);
		assert(a.i3 == 0x44556677);
#else
#error Not supported
#endif
		assert(a.i4 == 0x55667788);
	}


	exit(0);
	while(1)
		;
}
