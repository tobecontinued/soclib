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

#include "soclib/mwmr_controller.h"
#include "system.h"
#include "mwmr.h"

#include "../segmentation.h"

static const int period[4] = {10000};

#define WIDTH 4
#define DEPTH 16

int main(void)
{
	uputs("Hello from processor ");
	putchar(procnum()+'0');
	putchar('\n');

	uint32_t data[WIDTH*DEPTH];
	mwmr_t mwmr = MWMR_INITIALIZER(WIDTH, DEPTH, data, (uint32_t*)base(LOCK));

	mwmr_hw_init(base(MWMR0), MWMR_FROM_COPROC, 0, &mwmr);
	mwmr_hw_init(base(MWMR1), MWMR_TO_COPROC, 0, &mwmr);
	while(1)
		;
	return 0;
}
