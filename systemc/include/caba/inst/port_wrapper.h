// -*- c++ -*-
#ifndef PORT_WRAPPER_H
#define PORT_WRAPPER_H

#include <string>

namespace soclib { namespace caba { namespace inst {
class BasePort;
template <typename port_t, typename sig_t> class Port;
}}}

#include "signal_wrapper.h"

namespace soclib {
namespace caba {
namespace inst {

class BasePort {
	const std::string m_port_name;
	std::string m_owner_name;
protected:
	bool m_connected;
public:
	BasePort( const std::string & );
	BasePort( const BasePort & );
	virtual ~BasePort();
	virtual void connectTo( BaseSignal& sig ) = 0;
	inline void ownerNameSet( const std::string &name )
	{
		m_owner_name = name;
	}
	const std::string fullName() const
	{
		return m_owner_name + '.' + m_port_name;
	}
	inline bool isNamed( const std::string &name ) const
	{
		return name == m_port_name;
	}
	virtual BaseSignal &operator /( BasePort &port ) = 0;
	virtual BaseSignal &alone() = 0;
	inline void autoConn()
	{
		if ( m_connected ) return;
		std::cerr << "Warning: autoconnecting " << fullName() << " to nothing" << std::endl;
		alone();
	}
};

template <typename port_t, typename sig_t>
class Port
	: public BasePort
{
	port_t &m_port;
public:
	inline Port( const std::string &name, port_t &port );
	void connectTo( BaseSignal &sig );
	BaseSignal &operator /( BasePort &port );
	BaseSignal &alone();
};

}}}

#endif
