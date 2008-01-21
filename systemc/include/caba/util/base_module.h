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
 */
#ifndef SOCLIB_CABA_BASE_MODULE_H_
#define SOCLIB_CABA_BASE_MODULE_H_

#include <sstream>
#include "common/base_module.h"
#include "caba/inst/inst.h"

namespace soclib { namespace caba {

class BaseModule
    : public soclib::BaseModule
{
    static std::vector<BaseModule*> s_existing_modules;
    std::vector<inst::BasePort*> m_ports;
    BaseModule();
    BaseModule(const BaseModule &);
    const BaseModule &operator=(const BaseModule &);
public:
    BaseModule( sc_core::sc_module_name &name );
    virtual ~BaseModule();

    void portRegister( inst::BasePort *port );
    template<typename port_t> inline
    void portRegister( const std::string &name, port_t &_port ) {
        typedef inst::Port<port_t, typename inst::traits<port_t>::sig_t> T;
        T *port = new T( name, _port );
        portRegister( port );
    }
    template<typename port_t> inline
    void portRegisterN( const std::string &name, port_t *_port, const size_t n ) {
        for ( size_t i=0; i<n; ++i ) {
            std::ostringstream o("");
            o << name << '[' << i << ']';
            portRegister(o.str(), _port[i]);
        }
    }
    void autoConn();
    static void autoConnAll();

    inst::BasePort& portGet( const std::string &name );
    inline inst::BasePort &operator []( const std::string &name )
    {
        return portGet(name);
    }

	virtual void trace(sc_core::sc_trace_file &tf, const std::string base_name, unsigned int what)
    {
    }
};

}}

#endif /* SOCLIB_CABA_BASE_MODULE_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

