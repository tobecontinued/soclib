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

#include "iss_wrapper.h"
#include "mips.h"
#include "ppc405.h"
#include "microblaze.h"
#include "nios2_fast.h"
#include "gdbserver.h"
#include "caba_base_module.h"
#include "factory.h"
#include "inst_arg.h"
#include "fifo_reader.h"
#include "fifo_writer.h"

namespace soclib { namespace caba {

using soclib::caba::IssWrapper;

namespace {

template<typename iss_t, bool with_gdb>
BaseModule &inst_caba_cpu(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	using soclib::common::GdbServer;
	if ( with_gdb && args.has("with_gdb") && args.get<int>("with_gdb")  ) {
		if ( args.has("start_frozen") )
			GdbServer<iss_t>::start_frozen(args.get<int>("start_frozen"));
		return
			*new IssWrapper<GdbServer<iss_t> >(
				name.c_str(),
				args.get<int>("ident") );
	} else {
		return
			*new IssWrapper<iss_t>(
				name.c_str(),
				args.get<int>("ident") );
	}
}


template<typename module_t>
BaseModule &inst_fifo_rw(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	return
		*new module_t(
			name.c_str(),
			args.get<std::string>("bin"),
			args.get<std::vector<std::string> >("argv") );
}

using soclib::common::Factory;
using soclib::caba::BaseModule;

Factory<BaseModule> mipsel_factory("mipsel", &inst_caba_cpu<soclib::common::MipsElIss, true>);
Factory<BaseModule> mipseb_factory("mipseb", &inst_caba_cpu<soclib::common::MipsEbIss, true>);
Factory<BaseModule> nios2_factory("nios2", &inst_caba_cpu<soclib::common::Nios2fIss, false>);
Factory<BaseModule> ppc405_factory("ppc405", &inst_caba_cpu<soclib::common::Ppc405Iss, true>);
Factory<BaseModule> microblaze_factory("microblaze", &inst_caba_cpu<soclib::common::MicroBlazeIss, false>);
Factory<BaseModule> fifo_reader_factory("fifo_reader", &inst_fifo_rw<FifoReader<uint32_t> >);
Factory<BaseModule> fifo_writer_factory("fifo_writer", &inst_fifo_rw<FifoWriter<uint32_t> >);

}

}}
