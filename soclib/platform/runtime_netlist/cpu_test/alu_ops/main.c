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
#include "tester.h"

int main(void)
{
	assert(sizeof(uint64_t) == 8);
	assert(sizeof(int64_t) == 8);
	assert(sizeof(uint32_t) == 4);
	assert(sizeof(int32_t) == 4);


#define test(f, a) test_32(f, a)
#define test_val(f, a, v) test_val_32(f, a, v)

	test_val(add, (0LL, 0LL), 0LL);
	test_val(add, (3LL, 2LL), 5LL);
	test_val(add, (0x80000000LL, 0x80000000LL), 0LL);
	test_val(add, (0xffffffffLL, 1LL), 0LL);
	test_val(add, (0xffffffffLL, 2LL), 1LL);

	test_val(sub, (0LL, 0LL), 0LL);
	test_val(sub, (2LL, 1LL), 1LL);
	test_val(sub, (2LL, (uint32_t)-1LL), 3LL);
	test_val(sub, (0x80000000LL, 0x80000000LL), 0LL);
	test_val(sub, (0x80000000LL, 1LL), 0x7fffffffLL);
	test_val(sub, (0xffffffffLL, 1LL), 0xfffffffeLL);
	test_val(sub, (0xffffffffLL, 2LL), 0xfffffffdLL);
	test_val(sub, (0xffffffffLL, 0xffffffffLL), 0LL);

	test_val(and, (0LL, 0LL), 0LL);
	test_val(and, (3LL, 2LL), 2LL);
	test_val(and, (0x80000000LL, 0xaaaaaaaaLL), 0x80000000LL);
	test_val(and, (0xffffffffLL, 1LL), 1LL);
	test_val(and, (0xffffffffLL, 2LL), 2LL);

	test_val(or, (0LL, 0LL), 0LL);
	test_val(or, (3LL, 2LL), 3LL);
	test_val(or, (0x80000000LL, 0xaaaaaaaaLL), 0xaaaaaaaaLL);
	test_val(or, (0xffffffffLL, 1LL), 0xffffffffLL);
	test_val(or, (0xffffffffLL, 2LL), 0xffffffffLL);

	test_val(xor, (0LL, 0LL), 0LL);
	test_val(xor, (3LL, 2LL), 1LL);
	test_val(xor, (0x80000000LL, 0xaaaaaaaaLL), 0x2aaaaaaaLL);
	test_val(xor, (0xffffffffLL, 1LL), 0xfffffffeLL);
	test_val(xor, (0xffffffffLL, 2LL), 0xfffffffdLL);

	test_val(mul, (0LL, 0LL), 0LL);
	test_val(mul, (0LL, 1LL), 0LL);
	test_val(mul, (0LL, 0xffffffffLL), 0LL);
	test_val(mul, (0xffffffffLL, 0xffffffffLL), 1LL);
	test_val(mul, (42LL, 38LL), 1596LL);

	test_val(div, (0LL, 1LL), 0LL);
	test_val(div, (0LL, 0xffffffffLL), 0LL);
	test_val(div, (2LL, 1LL), 2LL);
	test_val(div, (2LL, -1LL), 0LL);
	test_val(div, (0x42LL, -3LL), 0LL);
	test_val(div, (531226LL, (uint32_t)300LL), 1770LL);
	test_val(div, (131072LL, (uint32_t)600LL), 131072/600LL);

	test_33_shift(srl, 0xaaaaaaaaLL);
	test_33_shift(srl, 0x55555555LL);
	test_33_shift(srl, 0x00000000LL);
	test_33_shift(srl, 0xffffffffLL);

	test_33_shift(sll, 0xaaaaaaaaLL);
	test_33_shift(sll, 0x55555555LL);
	test_33_shift(sll, 0x00000000LL);
	test_33_shift(sll, 0xffffffffLL);

	test_33_shift(mod, 0xaaaaaaaaLL);
	test_33_shift(mod, 0x12345678LL);
	test_33_shift(mod, 1000LL);
	test_33_shift(mod, 0x7fffffffLL);

	test_val(mklemask, (0LL), 0LL);
	test_val(mklemask, (1LL), 0x1LL);
	test_val(mklemask, (31LL), 0x7fffffffLL);
	test_val(mklemask, (32LL), 0xffffffffLL);

	test_val(sign_ext8, (0LL), 0LL);
	test_val(sign_ext8, (0x80LL), 0xffffff80LL);
	test_val(sign_ext8, (0x7fLL), 0x7fLL);
	test_val(sign_ext8, (0xffLL), 0xffffffffLL);

	test_val(sign_ext16, (0LL), 0LL);
	test_val(sign_ext16, (0x8000LL), 0xffff8000LL);
	test_val(sign_ext16, (0x7fffLL), 0x7fffLL);
	test_val(sign_ext16, (0xffffLL), 0xffffffffLL);

	test_33_shift(rotl, 0xaaaaaaaaLL);
	test_33_shift(rotl, 0x55555555LL);
	test_33_shift(rotl, 0x00000000LL);
	test_33_shift(rotl, 0xffffffffLL);

	test_33_shift(rotr, 0xaaaaaaaaLL);
	test_33_shift(rotr, 0x55555555LL);
	test_33_shift(rotr, 0x00000000LL);
	test_33_shift(rotr, 0xffffffffLL);

	test_val(mkmask, (10LL, 0LL), 0x3ffLL);
	test_val(mkmask, (32LL, 0LL), 0xffffffffLL);
	test_val(mkmask, (0LL, 0LL), 0LL);
	test_val(mkmask, (15+8LL, 15LL), 0x007f8000LL);

	test_val(extract, (0xfffffff7LL, 3LL, 2LL), 0x2LL);
	test_val(extract, (0xffffffffLL, 3LL, 0LL), 0LL);
	test_val(extract, (0xffffffffLL, 31LL, 1LL), 1LL);
	test_val(extract, (0x7fffffffLL, 31LL, 1LL), 0LL);
	test_val(extract, (0xffeeffffLL, 0LL, 32LL), 0xffeeffffLL);

	test_val(extract, (0xaaaaaaaaLL, 3LL, 2LL), 1LL);
	test_val(extract, (0xaaaaaaaaLL, 3LL, 0LL), 0LL);
	test_val(extract, (0xaaaaaaaaLL, 31LL, 1LL), 1LL);
	test_val(extract, (0xaaaaaaaaLL, 0LL, 32LL), 0xaaaaaaaaLL);

	test_val(extract, (0xff817fffLL, 15LL, 8LL), 0x02LL);
	test_val(extract, (0x00778000LL, 15LL, 8LL), 0xefLL);

	test_val(insert, (0xffffffffLL, 0LL, 0LL, 32LL), 0LL);
	test_val(insert, (0xffffffffLL, 6LL, 15LL, 8LL), 0xff837fffLL);
	test_val(insert, (0LL, 0xffffffffLL, 15LL, 8LL), 0x007f8000LL);

	test_val(extract_15_8, (0xff807fffLL), 0LL);
	test_val(extract_15_8, (0x007f8000LL), 0xffLL);

	test_val(insert_15_8, (0xffffffffLL, 0LL), 0xff807fffLL);
	test_val(insert_15_8, (0LL, 0xffffffffLL), 0x007f8000LL);

	test_val(swap, (0x11223344LL), 0x44332211LL);
	test_val(swap, (0x112233LL), 0x33221100LL);
	test_val(swap, (0x1122LL), 0x22110000LL);

#undef test
#undef test_val
#define test(f, a) test_64(f, a)
#define test_val(f, a, v) test_val_64(f, a, v)

	test_val(add, (0LL, 0LL), 0LL);
	test_val(add, (3LL, 2LL), 5LL);
	test_val(add, (0x80000000LL, 0x80000000LL), 0x100000000LL);
	test_val(add, (0xffffffffLL, 1LL), 0x100000000LL);
	test_val(add, (0xffffffffLL, 2LL), 0x100000001LL);

	test_val(sub, (0LL, 0LL), 0LL);
	test_val(sub, (2LL, 1LL), 1LL);
	test_val(sub, (2LL, (uint64_t)-1LL), 3LL);
	test_val(sub, (0x80000000LL, 0x80000000LL), 0LL);
	test_val(sub, (0x80000000LL, 1LL), 0x7fffffffLL);
	test_val(sub, (0xffffffffLL, 1LL), 0xfffffffeLL);
	test_val(sub, (0xffffffffLL, 2LL), 0xfffffffdLL);
	test_val(sub, (0xfffffffeLL, 0xffffffffLL), (uint64_t)-1);

	test_val(and, (0LL, 0LL), 0LL);
	test_val(and, (3LL, 2LL), 2LL);
	test_val(and, (0x80000000LL, 0xaaaaaaaaLL), 0x80000000LL);
	test_val(and, (0xffffffffLL, 1LL), 1LL);
	test_val(and, (0xffffffffLL, 2LL), 2LL);

	test_val(or, (0LL, 0LL), 0LL);
	test_val(or, (3LL, 2LL), 3LL);
	test_val(or, (0x80000000LL, 0xaaaaaaaaLL), 0xaaaaaaaaLL);
	test_val(or, (0xffffffffLL, 1LL), 0xffffffffLL);
	test_val(or, (0xffffffffLL, 2LL), 0xffffffffLL);

	test_val(xor, (0LL, 0LL), 0LL);
	test_val(xor, (3LL, 2LL), 1LL);
	test_val(xor, (0x80000000LL, 0xaaaaaaaaLL), 0x2aaaaaaaLL);
	test_val(xor, (0xffffffffLL, 1LL), 0xfffffffeLL);
	test_val(xor, (0xffffffffLL, 2LL), 0xfffffffdLL);

	test_val(mul, (0LL, 0LL), 0LL);
	test_val(mul, (0LL, 1LL), 0LL);
	test_val(mul, (0LL, 0xffffffffLL), 0LL);
	test_val(mul, (0xffffffffLL, 0xffffffffLL), 0xfffffffe00000001LL);
	test_val(mul, (42LL, 38LL), 1596LL);

	test_val(div, (0LL, 1LL), 0LL);
	test_val(div, (0LL, 0xffffffffLL), 0LL);
	test_val(div, (2LL, 1LL), 2LL);
	test_val(div, (2LL, -1LL), 0LL);
	test_val(div, (0x42LL, -3LL), 0LL);
	test_val(div, (531226LL, (uint32_t)300LL), 1770LL);
	test_val(div, (131072LL, (uint32_t)600LL), 131072/600LL);

	test_33_shift(srl, 0xaaaaaaaaLL);
	test_33_shift(srl, 0x55555555LL);
	test_33_shift(srl, 0x00000000LL);
	test_33_shift(srl, 0xffffffffLL);

	test_33_shift(sll, 0xaaaaaaaaLL);
	test_33_shift(sll, 0x55555555LL);
	test_33_shift(sll, 0x00000000LL);
	test_33_shift(sll, 0xffffffffLL);

	test_33_shift(mod, 0xaaaaaaaaLL);
	test_33_shift(mod, 0x12345678LL);
	test_33_shift(mod, 1000LL);
	test_33_shift(mod, 0x7fffffffLL);

	test_val(mklemask, (0LL), 0LL);
	test_val(mklemask, (1LL), 0x1LL);
	test_val(mklemask, (31LL), 0x7fffffffLL);
	test_val(mklemask, (48LL), 0xffffffffffffLL);
	test_val(mklemask, (32LL), 0xffffffffLL);

	test_val(sign_ext8, (0LL), 0LL);
	test_val(sign_ext8, (0x80LL), 0xffffffffffffff80LL);
	test_val(sign_ext8, (0x7fLL), 0x7fLL);
	test_val(sign_ext8, (0xffLL), 0xffffffffffffffffLL);

	test_val(sign_ext16, (0LL), 0LL);
	test_val(sign_ext16, (0x8000LL), 0xffffffffffff8000LL);
	test_val(sign_ext16, (0x7fffLL), 0x7fffLL);
	test_val(sign_ext16, (0xffffLL), 0xffffffffffffffffLL);

	test_33_shift(rotl, 0xaaaaaaaaLL);
	test_33_shift(rotl, 0x55555555LL);
	test_33_shift(rotl, 0x00000000LL);
	test_33_shift(rotl, 0xffffffffLL);

	test_33_shift(rotr, 0xaaaaaaaaLL);
	test_33_shift(rotr, 0x55555555LL);
	test_33_shift(rotr, 0x00000000LL);
	test_33_shift(rotr, 0xffffffffLL);

	test_val(mkmask, (10LL, 0LL), 0x3ffLL);
	test_val(mkmask, (32LL, 0LL), 0xffffffffLL);
	test_val(mkmask, (0LL, 0LL), 0LL);
	test_val(mkmask, (15+8LL, 15LL), 0x007f8000LL);

	test_val(extract, (0xfffffff7LL, 3LL, 2LL), 0x2LL);
	test_val(extract, (0xffffffffLL, 3LL, 0LL), 0LL);
	test_val(extract, (0xffffffffLL, 31LL, 1LL), 1LL);
	test_val(extract, (0x7fffffffLL, 31LL, 1LL), 0LL);
	test_val(extract, (0xffeeffffLL, 0LL, 32LL), 0xffeeffffLL);

	test_val(extract, (0xaaaaaaaaLL, 3LL, 2LL), 1LL);
	test_val(extract, (0xaaaaaaaaLL, 3LL, 0LL), 0LL);
	test_val(extract, (0xaaaaaaaaLL, 31LL, 1LL), 1LL);
	test_val(extract, (0xaaaaaaaaLL, 0LL, 32LL), 0xaaaaaaaaLL);

	test_val(extract, (0xff817fffLL, 15LL, 8LL), 0x02LL);
	test_val(extract, (0x00778000LL, 15LL, 8LL), 0xefLL);

	test_val(insert, (0xffffffffLL, 0LL, 0LL, 32LL), 0LL);
	test_val(insert, (0xffffffffLL, 6LL, 15LL, 8LL), 0xff837fffLL);
	test_val(insert, (0LL, 0xffffffffLL, 15LL, 8LL), 0x007f8000LL);

	test_val(extract_15_8, (0xff807fffLL), 0LL);
	test_val(extract_15_8, (0x007f8000LL), 0xffLL);

	test_val(insert_15_8, (0xffffffffLL, 0LL), 0xff807fffLL);
	test_val(insert_15_8, (0LL, 0xffffffffLL), 0x007f8000LL);

	test_val(swap, (0x1122334455667788LL), 0x8877665544332211LL);
	test_val(swap, (0x11223344556677LL), 0x7766554433221100LL);
	test_val(swap, (0x778844551122LL), 0x2211554488770000LL);

	exit(0);
}
