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

#include "ops.h"

int main(void)
{
	test_val(add, (0, 0), 0);
	test_val(add, (3, 2), 5);
	test_val(add, (0x80000000, 0x80000000), 0);
	test_val(add, (0xffffffff, 1), 0);
	test_val(add, (0xffffffff, 2), 1);

	test_val(sub, (0, 0), 0);
	test_val(sub, (2, 1), 1);
	test_val(sub, (2, (uint32_t)-1), 3);
	test_val(sub, (0x80000000, 0x80000000), 0);
	test_val(sub, (0x80000000, 1), 0x7fffffff);
	test_val(sub, (0xffffffff, 1), 0xfffffffe);
	test_val(sub, (0xffffffff, 2), 0xfffffffd);
	test_val(sub, (0xffffffff, 0xffffffff), 0);

	test_val(and, (0, 0), 0);
	test_val(and, (3, 2), 2);
	test_val(and, (0x80000000, 0xaaaaaaaa), 0x80000000);
	test_val(and, (0xffffffff, 1), 1);
	test_val(and, (0xffffffff, 2), 2);

	test_val(or, (0, 0), 0);
	test_val(or, (3, 2), 3);
	test_val(or, (0x80000000, 0xaaaaaaaa), 0xaaaaaaaa);
	test_val(or, (0xffffffff, 1), 0xffffffff);
	test_val(or, (0xffffffff, 2), 0xffffffff);

	test_val(xor, (0, 0), 0);
	test_val(xor, (3, 2), 1);
	test_val(xor, (0x80000000, 0xaaaaaaaa), 0x2aaaaaaa);
	test_val(xor, (0xffffffff, 1), 0xfffffffe);
	test_val(xor, (0xffffffff, 2), 0xfffffffd);

	test_val(mul, (0, 0), 0);
	test_val(mul, (0, 1), 0);
	test_val(mul, (0, 0xffffffff), 0);
	test_val(mul, (0xffffffff, 0xffffffff), 1);
	test_val(mul, (42, 38), 1596);

	test_val(div, (0, 1), 0);
	test_val(div, (0, 0xffffffff), 0);
	test_val(div, (2, 1), 2);
	test_val(div, (2, (uint32_t)-1), 0);
	test_val(div, (0x42, (uint32_t)-3), 0);
	test_val(div, (531226, (uint32_t)300), 1770);
	test_val(div, (131072, (uint32_t)600), 131072/600);

	test_33_shift(srl, 0xaaaaaaaa);
	test_33_shift(srl, 0x55555555);
	test_33_shift(srl, 0x00000000);
	test_33_shift(srl, 0xffffffff);

	test_33_shift(sll, 0xaaaaaaaa);
	test_33_shift(sll, 0x55555555);
	test_33_shift(sll, 0x00000000);
	test_33_shift(sll, 0xffffffff);

	test_33_shift(mod, 0xaaaaaaaa);
	test_33_shift(mod, 0x12345678);
	test_33_shift(mod, 1000);
	test_33_shift(mod, 0x7fffffff);

	test_val(mklemask, (0), 0);
	test_val(mklemask, (1), 0x1);
	test_val(mklemask, (31), 0x7fffffff);
	test_val(mklemask, (32), 0xffffffff);

	test_val(sign_ext8, (0), 0);
	test_val(sign_ext8, (0x80), 0xffffff80);
	test_val(sign_ext8, (0x7f), 0x7f);
	test_val(sign_ext8, (0xff), 0xffffffff);

	test_val(sign_ext16, (0), 0);
	test_val(sign_ext16, (0x8000), 0xffff8000);
	test_val(sign_ext16, (0x7fff), 0x7fff);
	test_val(sign_ext16, (0xffff), 0xffffffff);

	test_33_shift(rotl, 0xaaaaaaaa);
	test_33_shift(rotl, 0x55555555);
	test_33_shift(rotl, 0x00000000);
	test_33_shift(rotl, 0xffffffff);

	test_33_shift(rotr, 0xaaaaaaaa);
	test_33_shift(rotr, 0x55555555);
	test_33_shift(rotr, 0x00000000);
	test_33_shift(rotr, 0xffffffff);

	test_val(mkmask, (10, 0), 0x3ff);
	test_val(mkmask, (32, 0), 0xffffffff);
	test_val(mkmask, (0, 0), 0);
	test_val(mkmask, (15+8, 15), 0x007f8000);

	test_val(extract, (0xfffffff7, 3, 2), 0x2);
	test_val(extract, (0xffffffff, 3, 0), 0);
	test_val(extract, (0xffffffff, 31, 1), 1);
	test_val(extract, (0x7fffffff, 31, 1), 0);
	test_val(extract, (0xffeeffff, 0, 32), 0xffeeffff);

	test_val(extract, (0xaaaaaaaa, 3, 2), 1);
	test_val(extract, (0xaaaaaaaa, 3, 0), 0);
	test_val(extract, (0xaaaaaaaa, 31, 1), 1);
	test_val(extract, (0xaaaaaaaa, 0, 32), 0xaaaaaaaa);

	test_val(extract, (0xff817fff, 15, 8), 0x02);
	test_val(extract, (0x00778000, 15, 8), 0xef);

	test_val(insert, (0xffffffff, 0, 0, 32), 0);
	test_val(insert, (0xffffffff, 6, 15, 8), 0xff837fff);
	test_val(insert, (0, 0xffffffff, 15, 8), 0x007f8000);

	test_val(extract_15_8, (0xff807fff), 0);
	test_val(extract_15_8, (0x007f8000), 0xff);

	test_val(insert_15_8, (0xffffffff, 0), 0xff807fff);
	test_val(insert_15_8, (0, 0xffffffff), 0x007f8000);

	test_val(swap16, (0x11223344), 0x4433);
	test_val(swap16, (0x112233), 0x3322);
	test_val(swap16, (0x1122), 0x2211);

	test_val(swap32, (0x11223344), 0x44332211);
	test_val(swap32, (0x112233), 0x33221100);
	test_val(swap32, (0x1122), 0x22110000);

	test_val(swap16_2, (0x11223344), 0x22114433);
	test_val(swap16_2, (0x112233), 0x11003322);
	test_val(swap16_2, (0x1122), 0x00002211);

	exit(0);
}
