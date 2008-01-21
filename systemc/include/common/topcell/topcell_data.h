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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#ifndef TOPCELL_DATA_H
#define TOPCELL_DATA_H

#include "common/int_tab.h"
#include "common/segment.h"
#include "common/mapping_table.h"
#include "common/inst/inst_arg.h"
#include <stdint.h>

namespace soclib { namespace common { namespace topcell {

class Spec
{
public:
    Spec() {}
    virtual ~Spec() {}
    virtual Spec* dup() const = 0;
    virtual const std::string type() const = 0;
    virtual void setIn( inst::InstArg & ) const = 0;
};

class SpecList
{
    std::vector<Spec*> m_data;

    SpecList &operator=(SpecList&);
public:
    typedef std::vector<Spec*>::const_iterator iterator;

    SpecList();
    SpecList( const SpecList &ref );
    ~SpecList();
    inline void add( const Spec&s )
    {
        m_data.push_back(s.dup());
    }
    inline void add( Spec *s )
    {
        m_data.push_back(s);
    }
    inline size_t size() const { return m_data.size(); }
    inline iterator begin() const { return m_data.begin(); }
    inline iterator end() const { return m_data.end(); }
};

}}}

#endif
