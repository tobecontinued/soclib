// -*- c++ -*-

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
