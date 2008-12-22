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
#include "stdint.h"

typedef struct mwmr_s {
    unsigned int width;
	unsigned int gdepth;
    uint32_t *buffer;
	uint32_t *lock;
	volatile soclib_mwmr_status_s status;
} mwmr_t;

#define MWMR_INITIALIZER(width, depth, data, lock)						   \
	{ width, width*depth, data, (lock), SOCLIB_MWMR_STATUS_INITIALIZER }

void
mwmr_hw_init( void *coproc, enum SoclibMwmrWay way,
			  unsigned int no, const mwmr_t *mwmr );

void mwmr_initialize_pointer (mwmr_t *p_mwmr_t, unsigned int width, unsigned int depth, uint32_t *const buffer, uint32_t *lock);

void mwmr_config( void *coproc, unsigned int no, const uint32_t val );

uint32_t mwmr_status( void *coproc, unsigned int no );

void mwmr_write( mwmr_t *mwmr, const void *buffer, size_t size );
void mwmr_read( mwmr_t *mwmr, void *buffer, size_t size );
size_t mwmr_try_read( mwmr_t *fifo, void *_ptr, size_t lensw );
size_t mwmr_try_write( mwmr_t *fifo, const void *_ptr, size_t lensw );

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
