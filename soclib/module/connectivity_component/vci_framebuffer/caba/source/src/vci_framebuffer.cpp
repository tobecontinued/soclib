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
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 */

#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/vci_framebuffer.h"
#include "soclib_endian.h"

namespace soclib {
namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciFrameBuffer<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    if (addr>m_fb_controller.m_width*m_fb_controller.m_height*2)
        return false;

    int index = addr / vci_param::B;
    uint32_t *tab = m_fb_controller.surface();
	unsigned int cur = tab[index];
    uint32_t mask = 0;

    if ( be & 1 )
        mask |= 0x000000ff;
    if ( be & 2 )
        mask |= 0x0000ff00;
    if ( be & 4 )
        mask |= 0x00ff0000;
    if ( be & 8 )
        mask |= 0xff000000;
    
    tab[index] = (cur & ~mask) | (data & mask);

	m_defered_timeout = 30;
    return true;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    if (addr>m_fb_controller.m_width*m_fb_controller.m_height*2)
        return false;

	data = m_fb_controller.surface()[addr / vci_param::B];
	return true;
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_fsm.reset();
		return;
	}
	switch ( m_defered_timeout ) {
	case 0:
		break;
	case 1:
		m_fb_controller.update();
	default:
		--m_defered_timeout;
	}
	
	m_vci_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();
}

tmpl(/**/)::VciFrameBuffer(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
    unsigned long width,
    unsigned long height)
	: caba::BaseModule(name),
	  m_vci_fsm(p_vci, mt.getSegmentList(index), 2),
      m_fb_controller((const char *)name, width, height),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci")
{
	m_defered_timeout = 0;

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();
	
	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();

	m_vci_fsm.on_read_write(on_read, on_write);

    portRegister("clk", p_clk);
    portRegister("resetn", p_resetn);
    portRegister("vci", p_vci);
}

tmpl(/**/)::~VciFrameBuffer()
{
}

tmpl(void)::trace(sc_trace_file &tf, const std::string base_name, unsigned int what)
{

}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

