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

#include "vci_dma.h"
#include "vci_fd_access.h"
#include "vci_framebuffer.h"
#include "vci_icu.h"
#include "vci_locks.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_simhelper.h"
#include "vci_timer.h"
#include "vci_mwmr_controller.h"
#include "vci_vgmn.h"
#include "vci_simple_crossbar.h"
#include "vci_local_crossbar.h"
#include "vci_xcache.h"
#include "vci_factory.h"

namespace soclib { namespace caba {

namespace {

template<typename vci_param>
BaseModule& framebuffer(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciFrameBuffer<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("width"),
		args.get<int>("height") );
	return tmp;
}

template<typename vci_param>
BaseModule& ram(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule *tmp;
	if ( args.has("loads") ) {
		soclib::common::ElfLoader el(args.get<std::string>("loads"));
		tmp = new VciMultiRam<vci_param>(
			name.c_str(),
			args.get<soclib::common::IntTab>("_vci_id"),
			env.get<MappingTable>("mapping_table"),
			el );
	} else {
		tmp = new VciMultiRam<vci_param>(
			name.c_str(),
			args.get<soclib::common::IntTab>("_vci_id"),
			env.get<MappingTable>("mapping_table") );
	}
	return *tmp;
}

template<typename vci_param>
BaseModule& tty(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciMultiTty<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<std::vector<std::string> >("names") );
	return tmp;
}

template<typename vci_param>
BaseModule& timer(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciTimer<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("nirq") );
	return tmp;
}

template<typename vci_param>
BaseModule& vgmn(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciVgmn<vci_param>(
        name.c_str(),
        env.get<soclib::common::MappingTable>("mapping_table"),
        args.get<int>("n_initiators"),
        args.get<int>("n_targets"),
        args.get<int>("min_latency"),
        args.get<int>("fifo_size") );
	return tmp;
}

template<typename vci_param>
BaseModule& xcache(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciXCache<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("icache_lines"),
        args.get<int>("icache_words"),
        args.get<int>("dcache_lines"),
        args.get<int>("dcache_words") );
	return tmp;
}

template<typename vci_param>
BaseModule& dma(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciDma<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("buffer_size") );
	return tmp;
}

template<typename vci_param>
BaseModule& icu(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciIcu<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("nirq") );
	return tmp;
}

template<typename vci_param>
BaseModule& locks(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	return
		*new VciLocks<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table") );
}

template<typename vci_param>
BaseModule& simhelper(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	return
		*new VciSimhelper<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table") );
}

template<typename vci_param>
BaseModule& mwmr_controller(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	return
		*new VciMwmrController<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("plaps"),
        args.get<int>("fifo_to_coproc_depth"),
        args.get<int>("fifo_from_coproc_depth"),
        args.get<int>("n_to_coproc"),
        args.get<int>("n_from_coproc"),
        args.get<int>("n_config"),
        args.get<int>("n_status") );
}

template<typename vci_param>
BaseModule& local_crossbar(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	return
		*new VciLocalCrossbar<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("n_initiators"),
        args.get<int>("n_targets") );
}

template<typename vci_param>
BaseModule& simple_crossbar(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	return
		*new VciSimpleCrossbar<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("n_initiators"),
        args.get<int>("n_targets") );
}

#undef tmpl

}

#define register_factory(x) \
template<typename vci_param> \
soclib::common::Factory<soclib::caba::BaseModule> \
VciFactory<vci_param>::x##_factory(vci_param::string(#x), &x<vci_param>)

register_factory(framebuffer);
register_factory(ram);
register_factory(tty);
register_factory(timer);
register_factory(vgmn);
register_factory(xcache);
register_factory(dma);
register_factory(icu);
register_factory(locks);
register_factory(simhelper);
register_factory(mwmr_controller);
register_factory(local_crossbar);
register_factory(simple_crossbar);

}}
