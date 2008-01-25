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
 *         Nicolas Pouillon <nipo@ssji.net>, 2008
 *
 * Maintainers: nipo
 */

#include "soclib/mwmr_controller.h"

typedef struct mwmr_s {
	volatile unsigned int usage;
	uint32_t lock;
	volatile unsigned int readp;
	volatile unsigned int writep;
    const unsigned int width;
	const unsigned int gdepth;
    uint32_t *const buffer;
} mwmr_t;

#define MWMR_INITIALIZER(width, depth, data) \
	{ 0, 0, 0, 0, width, width*depth, data }

void
mwmr_hw_init( void *coproc, enum SoclibMwmrWay way,
			  unsigned int no, const mwmr_t *mwmr );

