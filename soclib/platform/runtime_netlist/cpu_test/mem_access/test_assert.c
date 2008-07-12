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

#include "data.h"

int memcmp(const void *_l, const void *_r, size_t n)
{
	const uint8_t *l = _l, *r = _r;
	size_t i;
	for (i=0; i<n; ++i)
		if ( l[i] != r[i] )
			return l[i] - r[i];
	return 0;
}

int memcmp4(const void *_l, const void *_r, size_t n)
{
	const uint32_t *l = _l, *r = _r;
	size_t i;
	n /= 4;
	for (i=0; i<n; ++i)
		if ( l[i] != r[i] )
			return l[i] - r[i];
	return 0;
}

#define test(x) if ( ! (x)) goto err;

void test_assert(volatile struct data_test* d)
{
	test(d->i32 == 0x01020304);
	test(d->i16[0] == 0x0506);
	test(d->i16[1] == 0x0708);
	test(d->i8[0] == 0x09);
	test(d->i8[1] == 0x0a);
	test(d->i8[2] == 0x0b);
	test(d->i8[3] == 0x0c);

#if defined(MIPSEB) || defined(PPC)
#warning Big endian test
	test(0==memcmp(d, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c", 12));
	test(0==memcmp4(d, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c", 12));
#elif defined(MIPSEL)
#warning Little endian test
	test(0==memcmp(d, "\x04\x03\x02\x01\x06\x05\x08\x07\x09\x0a\x0b\x0c", 12));
	test(0==memcmp4(d, "\x04\x03\x02\x01\x06\x05\x08\x07\x09\x0a\x0b\x0c", 12));
#endif
	return;
  err:
	{
		size_t i;
		for ( i=0; i<sizeof(struct data_test); ++i )
			printf("%02x ", ((const char *)d)[i]);
		printf(" \n");
	}
	assert(!"failed");
}
