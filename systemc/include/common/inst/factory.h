// -*- c++ -*-
#ifndef COMMON_FACTORY_H
#define COMMON_FACTORY_H

#include <map>
#include "common/exception.h"
#include "common/inst/inst_arg.h"

namespace soclib { namespace common {

template<typename module_t>
class Factory
{
public:
	typedef module_t& factory_func_t(
		const std::string &name,
		::soclib::common::inst::InstArg &args,
		::soclib::common::inst::InstArg &env );
	typedef std::map<std::string, Factory<module_t>*> map_t;

	Factory( const std::string &name, factory_func_t *factory )
		: m_func(factory)
	{
//		std::cout << "Registering factory " << name << std::endl;
		reg()[name] = this;
	}
	static Factory& get(const std::string &name)
	{
		if ( reg().count(name) )
			return *reg()[name];
		else
			throw soclib::exception::RunTimeError(
				std::string("Module named `")+name+"' not found");
	}
	module_t &operator()(
		const std::string &name,
		::soclib::common::inst::InstArg &args,
		::soclib::common::inst::InstArg &env )
	{
		return m_func(name, args, env);
	}
private:
	factory_func_t *m_func;
	static map_t &reg()
	{
		static map_t r;
		return r;
	}
};

}}

#endif
