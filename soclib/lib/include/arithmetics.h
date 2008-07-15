/* -*- c++ -*-
 *
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
#ifndef SOCLIB_COMMON_ARITHMETICS_H
#define SOCLIB_COMMON_ARITHMETICS_H

#include <stdint.h>
#include <math.h>

namespace soclib { namespace common {


static inline int32_t sign_ext16( int16_t val )
{
    return val;
}

static inline int32_t sign_ext8( int8_t val )
{
    return val;
}

static inline int32_t sign_ext26( int32_t val )
{
    uint32_t ext = (val&(1<<25)) ? 0xfc000000 : 0;
    return val|ext;
}

static inline bool carry( uint32_t a, uint32_t b, uint32_t c )
{
    return ((uint64_t)a+(uint64_t)b+(uint64_t)c)>>32;
}

static inline bool overflow( uint32_t a, uint32_t b, uint32_t c )
{
    return ((b^(a+b+c))&~(a^b))>>31;
}

static inline uint32_t uint32_log2(uint32_t n)
{
    return (uint32_t)(0.5f+log2(n));
}

template<typename T>
static inline T clz( T n )
{
#if __GNUC__
    if ( sizeof(T) == sizeof(unsigned int) )
        return __builtin_clz(n);
    else if ( sizeof(T) == sizeof(unsigned long) )
        return __builtin_clzl(n);
    else
#endif
    {
        for ( int i = 31; i>=0; --i )
            if ( (1<<i)&n )
                return 31-i;
        return 32;
    }
}

template<typename T>
static inline T clo( T n )
{
    return clz(~n);
}

}}

#endif /* SOCLIB_COMMON_ARITHMETICS_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

