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

#include "inst_arg.h"
#include "caba_base_module.h"
#include "factory.h"
#include "inst.h"
#include "vci_param.h"

namespace soclib { namespace caba {

template<typename vci_param>
class VciFactory
{
	static soclib::common::Factory framebuffer_factory;
	static soclib::common::Factory ram_factory;
	static soclib::common::Factory tty_factory;
	static soclib::common::Factory timer_factory;
	static soclib::common::Factory locks_factory;
	static soclib::common::Factory vgmn_factory;
	static soclib::common::Factory xcache_factory;
	static soclib::common::Factory dma_factory;
	static soclib::common::Factory icu_factory;
	static soclib::common::Factory simhelper_factory;
	static soclib::common::Factory mwmr_controller_factory;
	static soclib::common::Factory local_crossbar_factory;
	static soclib::common::Factory simple_crossbar_factory;
	static soclib::common::Factory xcache_mipsel_factory;
	static soclib::common::Factory xcache_mipseb_factory;
	static soclib::common::Factory xcache_ppc405_factory;
	static soclib::common::Factory xcache_microblaze_factory;
};

}}

#endif
