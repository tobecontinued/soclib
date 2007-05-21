/* -*- c++ -*-
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

#ifndef SOCLIB_COMMON_ENDIAN_H_
#define SOCLIB_COMMON_ENDIAN_H_

#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __linux__

#  include <endian.h>

#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)

#  include <machine/endian.h>
#  define __BYTE_ORDER BYTE_ORDER
#  define __LITTLE_ENDIAN LITTLE_ENDIAN
#  define __BIG_ENDIAN BIG_ENDIAN

#else

#  ifdef __LITTLE_ENDIAN__
#    define __BYTE_ORDER __LITTLE_ENDIAN
#  endif

#  if defined(i386) || defined(__i386__)
#    define __BYTE_ORDER __LITTLE_ENDIAN
#  endif

#  if defined(sun) && defined(unix) && defined(sparc)
#    define __BYTE_ORDER __BIG_ENDIAN
#  endif

#endif /* os switches */

#ifndef __BYTE_ORDER
# error Need to know endianess
#endif


#if __BYTE_ORDER == __LITTLE_ENDIAN
# define uint32_le_to_machine(x) (x)
# define uint32_machine_to_le(x) (x)
# define uint32_be_to_machine(x) ::soclib::endian::uint32_swap(x)
# define uint32_machine_to_be(x) ::soclib::endian::uint32_swap(x)

# define uint16_le_to_machine(x) (x)
# define uint16_machine_to_le(x) (x)
# define uint16_be_to_machine(x) ::soclib::endian::uint16_swap(x)
# define uint16_machine_to_be(x) ::soclib::endian::uint16_swap(x)
#else
# define uint32_le_to_machine(x) ::soclib::endian::uint32_swap(x)
# define uint32_machine_to_le(x) ::soclib::endian::uint32_swap(x)
# define uint32_be_to_machine(x) (x)
# define uint32_machine_to_be(x) (x)

# define uint16_le_to_machine(x) ::soclib::endian::uint16_swap(x)
# define uint16_machine_to_le(x) ::soclib::endian::uint16_swap(x)
# define uint16_be_to_machine(x) (x)
# define uint16_machine_to_be(x) (x)
#endif


namespace soclib { namespace endian {

static inline uint32_t uint32_swap(uint32_t x)
{
    return (
        ( (x & 0xff)   << 24 ) |
        ( (x & 0xff00) <<  8 ) |
        ( (x >>  8) & 0xff00 ) |
        ( (x >> 24) &   0xff )
        );
}

static inline uint16_t uint16_swap(uint16_t x)
{
    return ((x << 8) | (x >> 8));
}

template<size_t t> struct baset;
template<> struct baset<4> { typedef uint32_t base_t; };
template<> struct baset<2> { typedef uint16_t base_t; };
template<> struct baset<1> { typedef uint8_t base_t; };

template<size_t n> struct swapper_funcs;
template<>struct swapper_funcs<1> {
    static uint8_t le_to_machine(uint8_t x) { return x; }
    static uint8_t machine_to_le(uint8_t x) { return x; }
    static uint8_t be_to_machine(uint8_t x) { return x; }
    static uint8_t machine_to_be(uint8_t x) { return x; }
};
template<>struct swapper_funcs<2> {
    static uint16_t le_to_machine(uint16_t x) { return uint16_le_to_machine(x); }
    static uint16_t machine_to_le(uint16_t x) { return uint16_machine_to_le(x); }
    static uint16_t be_to_machine(uint16_t x) { return uint16_be_to_machine(x); }
    static uint16_t machine_to_be(uint16_t x) { return uint16_machine_to_be(x); }
};
template<>struct swapper_funcs<4> {
    static uint32_t le_to_machine(uint32_t x) { return uint32_le_to_machine(x); }
    static uint32_t machine_to_le(uint32_t x) { return uint32_machine_to_le(x); }
    static uint32_t be_to_machine(uint32_t x) { return uint32_be_to_machine(x); }
    static uint32_t machine_to_be(uint32_t x) { return uint32_machine_to_be(x); }
};

template<typename io_t> struct swapper
{
    typedef swapper_funcs<sizeof(io_t)> funcs;
    static io_t le_to_machine(io_t x) { return funcs::le_to_machine(x); }
    static io_t machine_to_le(io_t x) { return funcs::machine_to_le(x); }
    static io_t be_to_machine(io_t x) { return funcs::be_to_machine(x); }
    static io_t machine_to_be(io_t x) { return funcs::machine_to_be(x); }
};

}}

#define le_to_machine(x) ::soclib::endian::swapper<typeof(x)>::le_to_machine(x)
#define machine_to_le(x) ::soclib::endian::swapper<typeof(x)>::machine_to_le(x)
#define be_to_machine(x) ::soclib::endian::swapper<typeof(x)>::be_to_machine(x)
#define machine_to_be(x) ::soclib::endian::swapper<typeof(x)>::machine_to_be(x)

#define __ENDIAN_REVERSE_ARGS(b01, b02, b03, b04, b05, b06, b07, b08, b09, b10, b11, b12, b13, b14, b15, b16, ...)    \
			       b16; b15; b14; b13; b12; b11; b10; b09; b08; b07; b06; b05; b04; b03; b02; b01;

#define __ENDIAN_ARGS(b01, b02, b03, b04, b05, b06, b07, b08, b09, b10, b11, b12, b13, b14, b15, b16, ...)    \
			       b01; b02; b03; b04; b05; b06; b07; b08; b09; b10; b11; b12; b13; b14; b15; b16;

#if __BYTE_ORDER == __LITTLE_ENDIAN
#  define ENDIAN_BITFIELD(...)	__ENDIAN_REVERSE_ARGS(__VA_ARGS__,,,,,,,,,,,,,,,)
#else
#  define ENDIAN_BITFIELD(...)	__ENDIAN_ARGS(__VA_ARGS__,,,,,,,,,,,,,,,)
# endif

#define PACKED_BITFIELD(...) \
struct { ENDIAN_BITFIELD(__VA_ARGS__); } __attribute__((packed))

#endif /* SOCLIB_COMMON_ENDIAN_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

