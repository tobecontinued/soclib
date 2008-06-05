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

#ifndef COMMON_FACTORY_H
#define COMMON_FACTORY_H

#include <map>
#include "exception.h"
#include "inst_arg.h"
#include "module_holder.h"

namespace soclib { namespace common {

class Factory
{
public:
	typedef ModuleHolder& factory_func_t(
		const std::string &name,
		::soclib::common::inst::InstArg &args,
		::soclib::common::inst::InstArg &env );
	typedef std::map<std::string, Factory*> map_t;

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
	ModuleHolder &operator()(
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
