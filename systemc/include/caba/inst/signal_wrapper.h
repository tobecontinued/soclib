// -*- c++ -*-
#ifndef SIGNAL_WRAPPER_H
#define SIGNAL_WRAPPER_H

#include <string>

namespace soclib { namespace caba { namespace inst {
class BaseSignal;
template <typename sig_t> class Signal;
}}}

#include "port_wrapper.h"

namespace soclib {
namespace caba {
namespace inst {

class BaseSignal {
public:
	inline BaseSignal();
	virtual ~BaseSignal() {}
	inline BaseSignal &operator /( BasePort &port );
};

template <typename sig_t>
class Signal
	: public BaseSignal
{
	sig_t &m_sig;
public:
	inline Signal( sig_t &sig );
	inline sig_t& get();
	static inline Signal& create()
	{
		return *new Signal<sig_t>(*new sig_t);
	}
};

}}}

#endif
