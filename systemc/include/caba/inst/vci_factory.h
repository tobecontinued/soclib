// -*- c++ -*-
#ifndef CABA_VCI_FACTORY_H
#define CABA_VCI_FACTORY_H

#include "common/topcell/topcell_data.h"
#include "common/topcell/topcell.h"
#include "common/inst/inst_arg.h"
#include "caba/util/base_module.h"
#include "common/inst/factory.h"
#include "caba/inst/inst.h"
#include "caba/interface/vci_param.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciFactory
{
	typedef typename soclib::common::Factory<soclib::caba::BaseModule>::factory_func_t factory_func_t;
	static factory_func_t framebuffer;
	static factory_func_t ram;
	static factory_func_t tty;
	static factory_func_t timer;
	static factory_func_t vgmn;
	static factory_func_t xcache;
	static factory_func_t dma;
	static factory_func_t icu;
	static soclib::common::Factory<soclib::caba::BaseModule> fb_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> ram_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> tty_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> timer_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> vgmn_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> xcache_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> dma_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> icu_factory;
};

}}

#endif
