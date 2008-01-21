#include "caba/target/vci_dma.h"
#include "caba/target/vci_fd_access.h"
#include "caba/target/vci_framebuffer.h"
#include "caba/target/vci_icu.h"
#include "caba/target/vci_locks.h"
#include "caba/target/vci_multi_ram.h"
#include "caba/target/vci_multi_tty.h"
#include "caba/target/vci_simhelper.h"
#include "caba/target/vci_timer.h"
#include "caba/interconnect/vci_vgmn.h"
#include "caba/initiator/vci_xcache.h"
#include "caba/inst/vci_factory.h"

namespace soclib { namespace caba {

#define tmpl(t) template<typename vci_param> t VciFactory<vci_param>

tmpl(BaseModule&)::framebuffer(
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

tmpl(BaseModule&)::ram(
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

tmpl(BaseModule&)::tty(
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

tmpl(BaseModule&)::timer(
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

tmpl(BaseModule&)::vgmn(
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

tmpl(BaseModule&)::xcache(
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

tmpl(BaseModule&)::dma(
    const std::string &name,
    ::soclib::common::inst::InstArg &args,
    ::soclib::common::inst::InstArg &env )
{
	BaseModule &tmp =
		*new VciDma<vci_param>(
        name.c_str(),
        args.get<soclib::common::IntTab>("_vci_id"),
        env.get<MappingTable>("mapping_table"),
        args.get<int>("buffer_size") );
	return tmp;
}

tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 fb_factory(vci_param::string("framebuffer"), &framebuffer);
tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 ram_factory(vci_param::string("ram"), &ram);
tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 tty_factory(vci_param::string("tty"), &tty);
tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 timer_factory(vci_param::string("timer"), &timer);
tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 vgmn_factory(vci_param::string("vgmn"), &vgmn);
tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 xcache_factory(vci_param::string("xcache"), &xcache);
tmpl(soclib::common::Factory<soclib::caba::BaseModule>)::
 dma_factory(vci_param::string("dma"), &dma);

}}
