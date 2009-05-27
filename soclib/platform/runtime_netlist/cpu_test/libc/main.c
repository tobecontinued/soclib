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

#include "system.h"
#include "stdio.h"

int lookup(const char *);

int main(void)
{
	assert( lookup("deux") == 2 );
	assert( lookup("deux-mille") == 2000 );
	assert( lookup("deux-mille trois-cents") == 2300 );
	assert( lookup("mille") == 1000 );
	assert( lookup("mille neuf-cent quatre-vingt deux") == 1982 );
	assert( lookup("mille neuf-cent quatre-vingt dix") == 1990 );
	assert( lookup("mille neuf-cent quatre-vingt quatre") == 1984 );
	assert( lookup("un") == 1 );

	exit(0);
}
