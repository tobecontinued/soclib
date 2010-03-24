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
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo
 */
#ifndef ISS_MEMCHECKER_REGISTERS_H
#define ISS_MEMCHECKER_REGISTERS_H

// #include "soclib_io.h" // for auto finding purposes

enum SoclibIssMemcheckerRegisters {
	ISS_MEMCHECKER_MAGIC,
	ISS_MEMCHECKER_R1,
	ISS_MEMCHECKER_R2,
	ISS_MEMCHECKER_CONTEXT_ID_CREATE, // r1 = base, r2 = size, val = id
	ISS_MEMCHECKER_CONTEXT_ID_DELETE, // value = id
	ISS_MEMCHECKER_CONTEXT_ID_CHANGE, // r1 = id_orig, value = new_id
	ISS_MEMCHECKER_CONTEXT_CURRENT,  // value = id

	ISS_MEMCHECKER_MEMORY_REGION_UPDATE, // r1 = base, r2 = size, val = new_state

	ISS_MEMCHECKER_ENABLE_CHECKS,
	ISS_MEMCHECKER_DISABLE_CHECKS,
	
    /* mark a single word as initialized */
	ISS_MEMCHECKER_INITIALIZED,  // value = word addr

	ISS_MEMCHECKER_REGISTER_MAX,
};

#define ISS_MEMCHECKER_REGION_FREE 1
#define ISS_MEMCHECKER_REGION_ALLOC 2
#define ISS_MEMCHECKER_REGION_NONALLOC_STACK 4

#define ISS_MEMCHECKER_CHECK_SP (1<<0)
#define ISS_MEMCHECKER_CHECK_FP (1<<1)
#define ISS_MEMCHECKER_CHECK_INIT (1<<2)
#define ISS_MEMCHECKER_CHECK_REGION (1<<3)

#define ISS_MEMCHECKER_MAGIC_VAL 0x4d656d63

#endif /* ISS_MEMCHECKER_REGISTERS_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

