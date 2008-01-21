#include "caba/util/base_module.h"
#include "caba/inst/inst.h"

namespace soclib { namespace caba {

std::vector<BaseModule*> BaseModule::s_existing_modules;

BaseModule::BaseModule( sc_core::sc_module_name &name )
	: soclib::BaseModule(name)
{
    s_existing_modules.push_back(this);
}

BaseModule::~BaseModule()
{
// TODO !
//    s_existing_modules.erase(this);
	for ( std::vector<inst::BasePort*>::iterator i = m_ports.begin();
		  i != m_ports.end();
		  ++i )
		delete *i;
}

void BaseModule::autoConn()
{
	for ( std::vector<inst::BasePort*>::iterator i = m_ports.begin();
		  i != m_ports.end();
		  ++i )
		(*i)->autoConn();
}

void BaseModule::autoConnAll()
{
	for ( std::vector<BaseModule*>::iterator i = s_existing_modules.begin();
		  i != s_existing_modules.end();
		  ++i )
		(*i)->autoConn();
}

void BaseModule::portRegister( inst::BasePort *port )
{
	m_ports.push_back(port);
	m_ports.back()->ownerNameSet(name());
}

inst::BasePort& BaseModule::portGet( const std::string &req_name )
{
	for ( std::vector<inst::BasePort*>::iterator i = m_ports.begin();
		  i != m_ports.end();
		  ++i ) {
		if ( (*i)->isNamed(req_name) )
			return **i;
	}
	throw soclib::exception::RunTimeError(
		std::string("Cant find port `")+req_name+
		"' in module `"+name()+"'");
}

namespace inst {

BasePort::BasePort( const std::string &name )
	: m_port_name(name),
      m_connected(false)
{
}

BasePort::BasePort( const BasePort &port )
	: m_port_name(port.m_port_name),
	  m_owner_name(port.m_owner_name),
      m_connected(false)
{
}

BasePort::~BasePort()
{}

template <> void
Port<sc_core::sc_in<bool>, sc_core::sc_signal<bool> >::connectTo( BaseSignal &_sig )
{
	typedef Signal<sc_core::sc_clock> ck_t;
	typedef Signal<sc_core::sc_signal<bool> > wrapped_signal_t;

	if ( ck_t *sig = dynamic_cast<ck_t*>(&_sig) ) {
//		std::cout << "Connecting clock " << fullName() << std::endl;
		m_port(sig->get());
        m_connected = true;
	} else if ( wrapped_signal_t *sig = dynamic_cast<wrapped_signal_t*>(&_sig) ) {
//		std::cout << "Connecting " << fullName() << std::endl;
		m_port(sig->get());
        m_connected = true;
	} else
		throw soclib::exception::RunTimeError("Cant connect "+fullName());
}

}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

