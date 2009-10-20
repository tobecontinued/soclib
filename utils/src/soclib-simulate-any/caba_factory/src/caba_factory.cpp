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

#include <stdint.h>

#include "caba_base_module.h"
#include "factory.h"
#include "inst_arg.h"
#include "fifo_reader.h"
#include "fifo_writer.h"

#include "inst.h"
#include "port_traits.h"

namespace soclib {

namespace common {
namespace inst {

register_signal_for_port_with_t(typename word_t,
								soclib::caba::FifoOutput<word_t>,
								soclib::caba::FifoSignals<word_t>);
register_signal_for_port_with_t(typename word_t,
								soclib::caba::FifoInput<word_t>,
								soclib::caba::FifoSignals<word_t>);

}}

namespace caba {

using soclib::common::ModuleHolder;


namespace {

template<typename module_t>
ModuleHolder &inst_fifo_rw(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	module_t *module =
		new module_t(
			name.c_str(),
			args.get<std::string>("bin"),
			args.get<std::vector<std::string> >("argv") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("fifo", module->p_fifo);
	return *mh;
}

using soclib::common::Factory;
using soclib::caba::ModuleHolder;

Factory fifo_reader_factory("fifo_reader", &inst_fifo_rw<FifoReader<uint32_t> >);
Factory fifo_writer_factory("fifo_writer", &inst_fifo_rw<FifoWriter<uint32_t> >);

}

}}
