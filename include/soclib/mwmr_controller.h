/*
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef MWMR_CONTROLLER_REGISTERS_H
#define MWMR_CONTROLLER_REGISTERS_H

#define MWMR_SOFT_RESET 0x0
#define MWMR_FIFO_CONFIG (0x1 << 8)
#define MWMR_COPRO_CONFIG (0x2 << 8)
#define MWMR_COPRO_STATUS (0x3 << 8)
#define MWMR_TYPE_MASK 0x300
#define FIFO_RW_MASK 0x80

#define MWMR_FIFO_TO_COPROC   (0)
#define MWMR_FIFO_FROM_COPROC (1<<7)

#define MWMR_FIFO_ID_OFFSET 5
#define MWMR_FIFO_ID_MASK 0x3

#define MWMR_FIFO(way, num) \
     ((((num)&MWMR_FIFO_ID_MASK)<<MWMR_FIFO_ID_OFFSET)\
        &(way))

#define MWMR_CONFIG_STATE_AD   0x0
#define MWMR_CONFIG_OFFSET_AD  0x4
#define MWMR_CONFIG_LOCK_AD    0x8
#define MWMR_CONFIG_DEPTH      0xC
#define MWMR_CONFIG_BASE_AD    0x10
#define MWMR_CONFIG_RUNNING    0x14
#define MWMR_CONFIG_REG_MASK   0x1C

#if 0
#define FIFOREAD 0x0
#define FIFOWRITE (0x1 << 7)
#define FIFO_0 (0x0 << 5)
#define FIFO_1 (0x1 << 5)
#define FIFO_2 (0x2 << 5)
#define FIFO_3 (0x3 << 5)
#define FIFO_MASK (0x3 << 5)
#endif

#endif /* MWMR_CONTROLLER_REGISTERS_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

