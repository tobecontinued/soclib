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

#include "caba/processor/iss_wrapper.h"
#include "common/iss/mips.h"
#include "common/iss/ppc405.h"
#include "common/iss/microblaze.h"
#include "common/iss/gdbserver.h"
#include "caba/util/base_module.h"
#include "common/inst/factory.h"
#include "common/inst/inst_arg.h"

namespace soclib { namespace caba {

using soclib::caba::IssWrapper;

template<typename iss_t>
BaseModule &inst_caba_cpu(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	using soclib::common::GdbServer;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  ) {
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

namespace {

using soclib::common::Factory;
using soclib::caba::BaseModule;

Factory<BaseModule> mipsel_factory("mipsel", &inst_caba_cpu<soclib::common::MipsElIss>);
Factory<BaseModule> mipseb_factory("mipseb", &inst_caba_cpu<soclib::common::MipsEbIss>);
Factory<BaseModule> ppc405_factory("ppc405", &inst_caba_cpu<soclib::common::Ppc405Iss>);
Factory<BaseModule> microblaze_factory("microblaze", &inst_caba_cpu<soclib::common::MicroBlazeIss>);

}

}}
