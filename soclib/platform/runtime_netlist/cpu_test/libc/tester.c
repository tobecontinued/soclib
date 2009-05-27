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
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo
 */

#include <stdint.h>

struct pair_s {
	const char *str;
	int i;
};

struct pair_s pairs [] = {
	{"deux", 2},
	{"deux-mille", 2000},
	{"deux-mille trois-cents", 2300},
	{"mille", 1000},
	{"mille neuf-cent quatre-vingt deux", 1982},
	{"mille neuf-cent quatre-vingt dix", 1990},
	{"mille neuf-cent quatre-vingt quatre", 1984},
	{"un", 1},
};

int lookup(const char *a)
{
	size_t min = 0;
	size_t max = sizeof(pairs)/sizeof(*pairs);

	while (min != max) {
		size_t cur = min+((max-min)/2);
		printf("cmp: %d %d %s %s\n", min, max, a, pairs[cur].str);
		int d = strcmp(a, pairs[cur].str);
		if ( ! d )
			return pairs[cur].i;
		if ( d < 0 )
			max = cur;
		else
			min = cur;
	}
	return -1;
}
