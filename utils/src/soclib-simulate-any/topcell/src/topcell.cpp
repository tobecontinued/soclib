// -*- c++ -*-
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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "topcell_data.h"
#include "topcell.h"
#include "inst_arg.h"
#include "base_module.h"
#include "factory.h"
#include "inst.h"
#include <cstdio>
#include "data.h"

int topcell_parse();
int topcell_line;
int topcell_column;
extern FILE* topcell_in;
soclib::common::TopCell *topcell_parser_retval;

namespace soclib { namespace common { namespace topcell {

SpecList::SpecList()
{}

SpecList::SpecList( const SpecList &ref )
{
    for ( iterator i = ref.begin();
          i != ref.end();
          i++ )
        add(*(*i));
}

SpecList::~SpecList()
{
    for ( iterator i = begin();
          i != end();
          ++i )
        delete *i;
}

Conn::Conn( const Conn &ref )
    : m_local_sig_name(ref.m_local_sig_name),
      m_peer_name(ref.m_peer_name),
      m_peer_sig_name(ref.m_peer_sig_name)
{
}

Conn::Conn( const std::string &local_sig_name,
            const std::string &peer_name,
            const std::string &peer_sig_name )
    : m_local_sig_name(local_sig_name),
      m_peer_name(peer_name),
      m_peer_sig_name(peer_sig_name)
{
}

Conn::~Conn() {}

Spec* Conn::dup() const
{
    return new Conn(*this);
}

void Conn::setIn( inst::InstArg &args ) const
{
    args.get<std::vector<Conn*> >( "_conns" ).push_back( new Conn(*this) );
}

const std::string Conn::type() const {return "Conn";}

Segment::Segment( const Segment &ref )
    : m_name(ref.m_name),
      m_base(ref.m_base),
      m_length(ref.m_length),
      m_cacheable(ref.m_cacheable)
{
}

Segment::Segment( const std::string &name,
                  uint32_t base,
                  uint32_t length,
                  bool cacheable )
    : m_name(name),
      m_base(base),
      m_length(length),
      m_cacheable(cacheable)
{
}

Segment::~Segment() {}

Spec* Segment::dup() const
{
    return new Segment(*this);
}

const ::soclib::common::Segment Segment::segment( const ::soclib::common::IntTab &id ) const
{
    return ::soclib::common::Segment( m_name, m_base, m_length, id, m_cacheable );
}

void Segment::setIn( inst::InstArg &args ) const
{
    args.get<std::vector< ::soclib::common::Segment> >( "_segments" ).push_back(
        segment( args.get<soclib::common::IntTab>("_vci_id") ) );
}

const std::string Segment::type() const {return "Segment";}


Id::Id( const Id &ref )
    : m_id(ref.m_id)
{
}

Id::Id( const soclib::common::IntTab &id )
    : m_id(id)
{
}

Id::~Id() {}

Spec* Id::dup() const
{
    return new Id(*this);
}

void Id::setIn( inst::InstArg &args ) const
{
    args.add( "_vci_id", m_id );
}

const std::string Id::type() const {return "Id";}

} // End namespace soclib::common::topcell

TopCell::TopCell( const std::string &filename, int argc, const char *const *argv )
    : m_clk("clk"),
      m_resetn("resetn"),
      m_wrapped_clk( m_clk ),
      m_wrapped_resetn( m_resetn )
{
    topcell_in = std::fopen(filename.c_str(), "r");
    topcell_parser_retval = this;
    int argvno = 1;
    for ( int i=0; i<argc; ++i ) {
        std::string raw = argv[i];
        
        if ( size_t n = raw.find("=") ) {
            std::string left(raw, 0, n);
            std::string right(raw, n+1);
            if ( right[0] <= '9' && right[0] >= '0' ) {
                std::istringstream val(right);
                uint32_t v;
                val >> v;
                m_env.add(left, (int)v);
            } else {
                m_env.add(left, right);
            }
        } else {
            std::ostringstream argname;
            argname << "argv[" << argvno << "]";
            m_env.add(argname.str(), raw);
        }
    }
    assert(topcell_in);
    topcell_line = 1;
    topcell_column = 0;
    if ( topcell_parse() )
        throw soclib::exception::RunTimeError("Parse error");
    topcell_parser_retval = NULL;
    finalize();
}

void TopCell::finalize()
{
    instanciateAll();
    soclib::common::ModuleHolder::autoConnAll();
}

void TopCell::reset()
{
    sc_core::sc_start(sc_core::sc_time(0, sc_core::SC_NS));
    m_resetn = false;
    sc_core::sc_start(sc_core::sc_time(2, sc_core::SC_NS));
    m_resetn = true;
}


int TopCell::run()
{
    reset();
    if ( m_env.has("sim_max_cycles" ) ) {
        uint32_t max = m_env.get<int>("sim_max_cycles");
        return run(max);
    } else {
        std::cout << "Running for an infinite time" << std::endl;
        sc_core::sc_start();
    }
    if ( m_env.has("sim_default_retval" ) )
        return m_env.get<int>("sim_default_retval" );
    return 0;
}

int TopCell::run( const uint32_t ncycles )
{
    reset();
    std::cout << "Running for " << ncycles << " cycles" << std::endl;
    sc_core::sc_start(sc_core::sc_time(ncycles, sc_core::SC_NS));
    if ( m_env.has("sim_default_retval" ) )
        return m_env.get<int>("sim_default_retval" );
    return 0;
}

soclib::common::Factory& TopCell::get_factory(
    const std::string &cell )
{
    if ( m_env.has("vci_param") ) {
            std::ostringstream o;
            o << cell << '<' << m_env.get<std::string>("vci_param") << '>';
            try {
                return soclib::common::Factory::get( o.str() );
            } catch(...) {
            }
    }
    return soclib::common::Factory::get( cell );
}

void TopCell::cell_instanciate(
    const std::string &cell,
    const std::string &name,
    soclib::common::inst::InstArg *args )
{
    soclib::common::Factory &factory = get_factory(cell);
    soclib::common::ModuleHolder &mod = factory( name, *args, m_env );
    m_env.add( name, &mod );

    std::vector<topcell::Conn*> &v = args->get<std::vector<topcell::Conn*> >( "_conns" );
    for ( std::vector<topcell::Conn*>::iterator i = v.begin();
          i != v.end();
          i++ ) {
        topcell::Conn &c = **i;
        soclib::common::ModuleHolder &peer = *m_env.get<soclib::common::ModuleHolder*>(c.peer_name());
        mod[c.local_sig_name()] / peer[c.peer_sig_name()];
    }
    m_wrapped_resetn / mod["resetn"];
    m_wrapped_clk / mod["clk"];
}

void TopCell::prepare(
    enum inst_mode_e mode,
    const std::string &cell,
    const std::string &name,
    soclib::common::inst::InstArg *args )
{
    struct cell_ref ref;
    ref.mode = mode;
    ref.cell = cell;
    ref.name = name;
    ref.args = args;

    soclib::common::MappingTable &mt = m_env.get<soclib::common::MappingTable>( "mapping_table" );
    std::vector< ::soclib::common::Segment> &segments = args->get<std::vector< ::soclib::common::Segment> >( "_segments" );
    for ( std::vector< ::soclib::common::Segment>::iterator i = segments.begin();
          i != segments.end();
          i++ )
        mt.add(*i);

    m_cells.push_back(ref);
}

void TopCell::instanciateAll()
{
    for ( std::vector<struct cell_ref>::iterator i = m_cells.begin();
          i != m_cells.end();
          i++ )
    {
        struct cell_ref &c = *i;
        try {
//        std::cout << "Creating " << c.name << std::endl;
            switch (c.mode) {
            case MODE_CABA:
            case MODE_TLMT:
                cell_instanciate( c.cell, c.name, c.args );
                break;
            }
        } catch ( ... ) {
            std::cout << "Exception while creating " << c.name << std::endl;
            throw;
        }
        cleanup_args(c.args);
    }
}

soclib::common::inst::InstArg* TopCell::new_args()
{
    soclib::common::inst::InstArg *args = new soclib::common::inst::InstArg;
    args->add( "_conns", std::vector<topcell::Conn*>() );
    args->add( "_segments", std::vector< ::soclib::common::Segment>() );
    return args;
}

void TopCell::cleanup_args(soclib::common::inst::InstArg*args)
{
    std::vector<topcell::Conn*> &v = args->get<std::vector<topcell::Conn*> >( "_conns" );
    for ( std::vector<topcell::Conn*>::iterator i = v.begin();
          i != v.end();
          i++ )
        delete *i;
}


}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

