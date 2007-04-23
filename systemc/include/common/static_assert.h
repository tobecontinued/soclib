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
#ifndef SOCLIB_STATIC_ASSERT_H_
#define SOCLIB_STATIC_ASSERT_H_

/** \cond false */
/* Hide implementation details from doxygen */

namespace soclib { namespace common {

/*
 * Here we only define implementation for true, if we try to
 * instantiate the false version, this will fail.
 */
template<bool condition>
 class compile_time_assertion_failed;

template<> class compile_time_assertion_failed<true>{
    enum { value = 1 };
};

template<int n> class compile_time_assertion_test {
    enum { value = n };
};

}}

#define _assert_concat_r(x, y) x##y
#define _assert_concat(x, y) _assert_concat_r(x, y)
/** \endcond */

#define IS_POW_OF_2(x) (!((x)&((x)-1)))

/**
 * \brief A compile-time assertion
 *
 * Use it like assert()
 *
 * \param condition Condition to check for, must be compile-time
 * evaluable.
 */
#define static_assert(condition) \
    typedef ::soclib::common::compile_time_assertion_test< \
    sizeof(::soclib::common::compile_time_assertion_failed< (bool)( condition ) >)> \
    _assert_concat(static_assert_typedef_, __LINE__)

#endif /* SOCLIB_STATIC_ASSERT_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

