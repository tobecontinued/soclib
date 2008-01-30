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

#ifndef CABA_VCI_FACTORY_H
#define CABA_VCI_FACTORY_H

#include "topcell/topcell_data.h"
#include "topcell/topcell.h"
#include "inst/inst_arg.h"
#include "caba/caba_base_module.h"
#include "inst/factory.h"
#include "caba/inst/inst.h"
#include "vci_param.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciFactory
{
	static soclib::common::Factory<soclib::caba::BaseModule> framebuffer_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> ram_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> tty_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> timer_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> locks_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> vgmn_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> xcache_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> dma_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> icu_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> simhelper_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> mwmr_controller_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> local_crossbar_factory;
	static soclib::common::Factory<soclib::caba::BaseModule> simple_crossbar_factory;
};

}}

#endif
