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
#ifndef SOCLIB_COMMON_TOPCELL_H
#define SOCLIB_COMMON_TOPCELL_H

#include <systemc>
#include <stdint.h>
#include "inst/inst_arg.h"
#include "inst/factory.h"
#include "caba/inst/signal_wrapper"

namespace soclib { namespace common {

enum inst_mode_e {
	MODE_CABA,
	MODE_TLMT,
};

class TopCell
{
	struct cell_ref {
		enum inst_mode_e mode;
		std::string cell;
		std::string name;
		soclib::common::inst::InstArg *args;
	};

	sc_core::sc_clock m_clk;
	sc_core::sc_signal<bool> m_resetn;
	soclib::caba::inst::Signal<sc_core::sc_clock> m_wrapped_clk;
	soclib::caba::inst::Signal<sc_core::sc_signal<bool> > m_wrapped_resetn;
	soclib::common::inst::InstArg m_env;
	std::vector<struct cell_ref> m_cells;
	
	static void cleanup_args(soclib::common::inst::InstArg*);

	template<typename base_t>
	void cell_instanciate( const std::string &cell,
						   const std::string &name,
						   soclib::common::inst::InstArg *args );
	void instanciateAll();
	template<typename base_t>
	soclib::common::Factory<base_t> & get_factory( const std::string &cell );
	void finalize();
public:
	TopCell( const std::string &filename, int argc, const char *const*argv );
	int run();
	void reset();
	int run(const uint32_t ncycles);
	void prepare( enum inst_mode_e mode,
				  const std::string &cell,
				  const std::string &name,
				  soclib::common::inst::InstArg *args );
	static soclib::common::inst::InstArg* new_args();
	inline soclib::common::inst::InstArg &env() {return m_env;}
};

}}

#endif
