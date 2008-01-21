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

BaseModule &mips(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new IssWrapper<soclib::common::MipsIss>(
        name.c_str(),
        args.get<int>("ident") );
	return tmp;
}

BaseModule &gdb_mips(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	if ( args.has("start_frozen") )
		soclib::common::GdbServer<soclib::common::MipsIss>::start_frozen();
	BaseModule &tmp =
		*new IssWrapper<soclib::common::GdbServer<soclib::common::MipsIss> >(
        name.c_str(),
        args.get<int>("ident") );
	return tmp;
}

BaseModule &ppc405(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new IssWrapper<soclib::common::Ppc405Iss>(
        name.c_str(),
        args.get<int>("ident") );
	return tmp;
}

BaseModule &gdb_ppc405(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	if ( args.has("start_frozen") )
		soclib::common::GdbServer<soclib::common::Ppc405Iss>::start_frozen();
	BaseModule &tmp =
		*new IssWrapper<soclib::common::GdbServer<soclib::common::Ppc405Iss> >(
        name.c_str(),
        args.get<int>("ident") );
	return tmp;
}

BaseModule &microblaze(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new IssWrapper<soclib::common::MicroBlazeIss>(
        name.c_str(),
        args.get<int>("ident") );
	return tmp;
}

BaseModule &gdb_microblaze(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	if ( args.has("start_frozen") )
		soclib::common::GdbServer<soclib::common::MicroBlazeIss>::start_frozen();
	BaseModule &tmp =
		*new IssWrapper<soclib::common::GdbServer<soclib::common::MicroBlazeIss> >(
        name.c_str(),
        args.get<int>("ident") );
	return tmp;
}

namespace {

soclib::common::Factory<soclib::caba::BaseModule> mips_factory("mips", &mips);
soclib::common::Factory<soclib::caba::BaseModule> gdb_mips_factory("gdb_mips", &gdb_mips);
soclib::common::Factory<soclib::caba::BaseModule> ppc405_factory("ppc405", &ppc405);
soclib::common::Factory<soclib::caba::BaseModule> gdb_ppc405_factory("gdb_ppc405", &gdb_ppc405);
soclib::common::Factory<soclib::caba::BaseModule> microblaze_factory("microblaze", &microblaze);
soclib::common::Factory<soclib::caba::BaseModule> gdb_microblaze_factory("gdb_microblaze", &gdb_microblaze);

}

}}
