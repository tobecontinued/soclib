// -*- c++ -*-

#ifndef TOPCELL_PRIVATE_DATA_H_
#define TOPCELL_PRIVATE_DATA_H_

#include <cassert>
#include <vector>
#include "common/int_tab.h"
#include "common/segment.h"
#include "common/mapping_table.h"
#include "common/topcell/topcell_data.h"

namespace soclib { namespace common { namespace topcell {

template<typename T>
class Param
    : public Spec
{
    const std::string m_name;
    const T m_value;
public:
    Param( const Param<T> &ref )
        : m_name(ref.m_name), m_value(ref.m_value)
    {
    }
    Param( const std::string &name, const T value )
        : m_name(name), m_value(value)
    {
    }
    ~Param() {}
    Spec* dup() const
    {
        return new Param(*this);
    }
    const std::string type() const {return "Param";}
    void setIn( inst::InstArg &args ) const
    {
        args.add( m_name, m_value );
    }
};


class Conn
    : public Spec
{
    const std::string m_local_sig_name;
    const std::string m_peer_name;
    const std::string m_peer_sig_name;
public:
    Conn( const Conn &ref );
    Conn( const std::string &local_sig_name,
          const std::string &peer_name,
          const std::string &peer_sig_name );
    ~Conn();
    Spec* dup() const;
    inline const std::string &local_sig_name() const { return m_local_sig_name; }
    inline const std::string &peer_name() const { return m_peer_name; }
    inline const std::string &peer_sig_name() const { return m_peer_sig_name; }
    const std::string type() const;
    void setIn( inst::InstArg & ) const;
};


class Segment
    : public Spec
{
    const std::string m_name;
    uint32_t m_base;
    uint32_t m_length;
    bool m_cacheable;

public:
    Segment( const Segment &ref );
    Segment( const std::string &name,
             uint32_t base,
             uint32_t length,
             bool cacheable );
    ~Segment();
    Spec* dup() const;
    inline std::string name() const { return m_name; }
    inline uint32_t base() const { return m_base; }
    inline uint32_t length() const { return m_length; }
    inline bool cached() const { return m_cacheable; }
    const std::string type() const;
    const ::soclib::common::Segment segment( const ::soclib::common::IntTab & ) const;
    void setIn( inst::InstArg & ) const;
};


class Id
    : public Spec
{
    const soclib::common::IntTab m_id;
public:
    Id( const Id &ref );
    Id( const soclib::common::IntTab &id );
    inline const soclib::common::IntTab &id() const
    {
        return m_id;
    }
    ~Id();
    Spec* dup() const;
    const std::string type() const;
    void setIn( inst::InstArg & ) const;
};


}}}

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

