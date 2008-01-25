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

#include "io.h"

enum SoclibMwmrRegisters {
    MWMR_IOREG_MAX = 16,
    MWMR_RESET = MWMR_IOREG_MAX,
    MWMR_CONFIG_FIFO_WAY,
    MWMR_CONFIG_FIFO_NO,
    MWMR_CONFIG_STATE_ADDR,
    MWMR_CONFIG_OFFSET_ADDR,
    MWMR_CONFIG_LOCK_ADDR,
    MWMR_CONFIG_DEPTH,
    MWMR_CONFIG_WIDTH,
    MWMR_CONFIG_BASE_ADDR,
    MWMR_CONFIG_RUNNING,
};

enum SoclibMwmrWay {
    MWMR_TO_COPROC,
    MWMR_FROM_COPROC,
};

#endif /* MWMR_CONTROLLER_REGISTERS_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

