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
#include "vci_xcache_wrapper.h"
#include "mips.h"
#include "ppc405.h"
#include "arm.h"
#include "microblaze.h"
#include "mips32.h"
#include "ississ2.h"
#include "gdbserver.h"
#include "vci_factory.h"

namespace soclib {

namespace common {
namespace inst {

register_signal_for_port_with_t(typename word_t,
								soclib::caba::FifoOutput<word_t>,
								soclib::caba::FifoSignals<word_t>);
register_signal_for_port_with_t(typename word_t,
								soclib::caba::FifoInput<word_t>,
								soclib::caba::FifoSignals<word_t>);
register_signal_for_port_with_t(typename vci_param,
								soclib::caba::VciInitiator<vci_param>,
								soclib::caba::VciSignals<vci_param>);
register_signal_for_port_with_t(typename vci_param,
								soclib::caba::VciTarget<vci_param>,
								soclib::caba::VciSignals<vci_param>);

}}

namespace caba {

namespace {

template<typename vci_param>
ModuleHolder& framebuffer(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciFrameBuffer<vci_param> *module =
		new VciFrameBuffer<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("width"),
		args.get<int>("height") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	return *mh;
}

template<typename vci_param>
ModuleHolder& ram(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciRam<vci_param> *module;
	if ( args.has("loads") ) {
		soclib::common::Loader el(args.get<std::string>("loads"));
		module = new VciRam<vci_param>(
			name.c_str(),
			args.get<soclib::common::IntTab>("_vci_id"),
			env.get<MappingTable>("mapping_table"),
			el );
	} else {
		module = new VciRam<vci_param>(
			name.c_str(),
			args.get<soclib::common::IntTab>("_vci_id"),
			env.get<MappingTable>("mapping_table") );
	}
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	return *mh;
}

template<typename vci_param>
ModuleHolder& tty(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciMultiTty<vci_param> *module =
		new VciMultiTty<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<std::vector<std::string> >("names") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	mh->portRegisterN("irq", module->p_irq, args.get<std::vector<std::string> >("names").size());
	return *mh;
}

template<typename vci_param>
ModuleHolder& timer(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciTimer<vci_param> *module =
		new VciTimer<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("nirq") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	mh->portRegisterN("irq", module->p_irq, args.get<int>("nirq"));
	return *mh;
}

template<typename vci_param>
ModuleHolder& vgmn(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    soclib::common::inst::InstArg &env )
{
	VciVgmn<vci_param> *module =
		new VciVgmn<vci_param>(
        name.c_str(),
        env.get<soclib::common::MappingTable>("mapping_table"),
        args.get<int>("n_initiators"),
        args.get<int>("n_targets"),
        args.get<int>("min_latency"),
        args.get<int>("fifo_size") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegisterN("to_target", module->p_to_target, args.get<int>("n_targets"));
	mh->portRegisterN("to_initiator", module->p_to_initiator, args.get<int>("n_initiators"));
	return *mh;
}

template<typename vci_param>
ModuleHolder& dma(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciDma<vci_param> *module =
		new VciDma<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("buffer_size") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci_target", module->p_vci_target);
	mh->portRegister("vci_initiator", module->p_vci_initiator);
	mh->portRegister("irq", module->p_irq);
	return *mh;
}

template<typename vci_param>
ModuleHolder& icu(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciIcu<vci_param> *module =
		new VciIcu<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("nirq") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	mh->portRegister("irq", module->p_irq);
	mh->portRegisterN("irq_in", module->p_irq_in, args.get<int>("nirq"));
	return *mh;
}

template<typename vci_param>
ModuleHolder& locks(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciLocks<vci_param> *module =
		new VciLocks<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	return *mh;
}

template<typename vci_param>
ModuleHolder& simhelper(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciSimhelper<vci_param> *module =
		new VciSimhelper<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	return *mh;
}

template<typename vci_param>
ModuleHolder& mwmr_controller(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciMwmrController<vci_param> *module =
		new VciMwmrController<vci_param>(
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
        args.get<int>("n_status"),
        (bool)args.get<int>("use_llsc") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci_target", module->p_vci_target);
	mh->portRegister("vci_initiator", module->p_vci_initiator);
	mh->portRegisterN("from_coproc", module->p_from_coproc, args.get<int>("n_from_coproc"));
	mh->portRegisterN("to_coproc", module->p_to_coproc, args.get<int>("n_to_coproc"));
	mh->portRegisterN("config", module->p_config, args.get<int>("n_config"));
	mh->portRegisterN("status", module->p_status, args.get<int>("n_status"));
	return *mh;
}

template<typename vci_param>
ModuleHolder& local_crossbar(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciLocalCrossbar<vci_param> *module =
		new VciLocalCrossbar<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("n_initiators"),
        args.get<int>("n_targets") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("target_to_up", module->p_target_to_up);
	mh->portRegister("initiator_to_up", module->p_initiator_to_up);
	mh->portRegisterN("to_target", module->p_to_target, args.get<int>("n_targets"));
	mh->portRegisterN("to_initiator", module->p_to_initiator, args.get<int>("n_initiators"));
	return *mh;
}

template<typename vci_param>
ModuleHolder& simple_crossbar(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	VciSimpleCrossbar<vci_param> *module =
		new VciSimpleCrossbar<vci_param>(
        name.c_str(),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("n_initiators"),
        args.get<int>("n_targets") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegisterN("to_target", module->p_to_target, args.get<int>("n_targets"));
	mh->portRegisterN("to_initiator", module->p_to_initiator, args.get<int>("n_initiators"));
	return *mh;
}

template<typename vci_param, typename iss_t>
ModuleHolder& xcache_cpu(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef VciXcacheWrapper<vci_param, iss_t> mod_t;
	mod_t *module =
		new mod_t(
        name.c_str(),
		args.get<int>("ident"),
        env.get<MappingTable>("mapping_table"),
        args.get<soclib::common::IntTab>("_vci_id"),
        args.get<int>("icache_ways"),
        args.get<int>("icache_sets"),
        args.get<int>("icache_words"),
        args.get<int>("dcache_ways"),
        args.get<int>("dcache_sets"),
        args.get<int>("dcache_words") );
	ModuleHolder *mh = new ModuleHolder(module);
	mh->portRegister("clk", module->p_clk);
	mh->portRegister("resetn", module->p_resetn);
	mh->portRegister("vci", module->p_vci);
	mh->portRegisterN("irq", module->p_irq, iss_t::n_irq);
	return *mh;
}

template<typename vci_param>
ModuleHolder& xcache_mipsel(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef soclib::common::IssIss2< ::soclib::common::MipsElIss> iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

template<typename vci_param>
ModuleHolder& xcache_mipseb(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef soclib::common::IssIss2< ::soclib::common::MipsEbIss> iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

template<typename vci_param>
ModuleHolder& xcache_mips32el(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef ::soclib::common::Mips32ElIss iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

template<typename vci_param>
ModuleHolder& xcache_mips32eb(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef ::soclib::common::Mips32EbIss iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

template<typename vci_param>
ModuleHolder& xcache_ppc405(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef ::soclib::common::Ppc405Iss iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

template<typename vci_param>
ModuleHolder& xcache_microblaze(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef soclib::common::IssIss2< ::soclib::common::MicroBlazeIss> iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

template<typename vci_param>
ModuleHolder& xcache_arm(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	typedef soclib::common::ArmIss iss_t;
	if ( args.has("with_gdb") && args.get<int>("with_gdb")  )
		return xcache_cpu<vci_param, soclib::common::GdbServer<iss_t> >
			(name, args, env);
	else
		return xcache_cpu<vci_param, iss_t>(name, args, env);
}

#undef tmpl

}

#define register_factory(x) \
template<typename vci_param> \
soclib::common::Factory \
VciFactory<vci_param>::x##_factory(vci_param::string(#x), &x<vci_param>)

register_factory(framebuffer);
register_factory(ram);
register_factory(tty);
register_factory(timer);
register_factory(vgmn);
register_factory(dma);
register_factory(icu);
register_factory(locks);
register_factory(simhelper);
register_factory(mwmr_controller);
register_factory(local_crossbar);
register_factory(simple_crossbar);

register_factory(xcache_mipsel);
register_factory(xcache_mipseb);
register_factory(xcache_mips32el);
register_factory(xcache_mips32eb);
register_factory(xcache_ppc405);
register_factory(xcache_microblaze);
register_factory(xcache_arm);


}}
