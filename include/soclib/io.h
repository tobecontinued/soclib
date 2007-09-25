/*
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_IO__H
#define SOCLIB_IO__H

#include <stdint.h>

static inline void soclib_io_set(void *comp_base, int reg, uint32_t val)
{
	volatile uint32_t *addr = (uint32_t *)comp_base;

#if __PPC__
    reg <<= 2;
    asm("stwbrx %0, %1, %2":: "b"(val), "b"(addr), "b"(reg) : "memory" );
#else
	addr += reg;
	*addr = val;
#endif
}

static inline uint32_t soclib_io_get(void *comp_base, int reg)
{
	volatile uint32_t *addr = (uint32_t *)comp_base;

#if __PPC__
    uint32_t val;
    reg <<= 2;
    asm("lwbrx %0, %1, %2": "=b"(val): "b"(addr), "b"(reg) );
    return val;
#else
	addr += reg;
	return *addr;
#endif
}

static inline void soclib_io_write8(void *comp_base, int reg, uint8_t val)
{
	volatile uint32_t *addr = (uint32_t *)comp_base;
	addr += reg;

	*(uint8_t *)addr = val;
}

static inline uint8_t soclib_io_read8(void *comp_base, int reg)
{
	volatile uint32_t *addr = (uint32_t *)comp_base;
	addr += reg;

	return *(uint8_t *)addr;
}

#endif /* SOCLIB_IO__H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

