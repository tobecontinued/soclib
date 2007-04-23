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

#ifndef SOCLIB_EXCEPTION_H_
#define SOCLIB_EXCEPTION_H_

#include <string>
#include <iostream>

namespace soclib { namespace exception {

class Exception
{
    std::string m_type;
    std::string m_message;

public:
    Exception( const std::string &type,
               const std::string &message )
            : m_type(type), m_message(message)
    {}

    const std::string &message() const
    {
        return m_message;
    }

    void print( std::ostream &o ) const
    {
        o << "<soclib::exception::" << m_type << ": " << m_message;
    }

    friend std::ostream &operator << (std::ostream &o, const Exception &e)
    {
        e.print(o);
        return o;
    }
};

class ValueError
    : Exception
{
public:
    ValueError( const std::string &message )
            : Exception( "ValueError", message )
    {}
};

class Collision
    : Exception
{
public:
    Collision( const std::string &message )
            : Exception( "Collision", message )
    {}
};

class RunTimeError
    : Exception
{
public:
    RunTimeError( const std::string &message )
            : Exception( "RunTimeError", message )
    {}
};

}}

#endif /* SOCLIB_EXCEPTION_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

