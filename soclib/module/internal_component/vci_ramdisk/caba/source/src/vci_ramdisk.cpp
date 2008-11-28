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
 *
 * Maintainers: joel
 */

#include "vci_ramdisk.h"
#include "soclib_endian.h"

namespace soclib {
namespace caba {

using namespace soclib;

#define tmpl(x) template<typename vci_param> x VciRamDisk<vci_param>

tmpl(/**/)::VciRamDisk(
	sc_module_name insname,
	const IntTab &index,
	const MappingTable &mt,
    const std::string &filename
    )
	: caba::BaseModule(insname),
	  m_vci_fsm(p_vci, mt.getSegmentList(index)),
      m_filename(filename),
      p_resetn("resetn"),
      p_clk("clk"),
      p_vci("vci")
{
    assert(m_vci_fsm.nbSegments() == 1 && "RamDisk has only one segment");
	m_vci_fsm.on_read_write(on_read, on_write);
	
	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();
	
	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
	
	size_t word_size = vci_param::B; // B is VCI's cell size
    m_contents = new ram_t[(m_vci_fsm.getSize(0)+word_size-1)/word_size];
}

tmpl(/**/)::~VciRamDisk()
{
    delete [] m_contents;
}

tmpl(void)::reload()
{
    FILE* diskimg;
    if ((diskimg = fopen(m_filename.c_str(), "r")) == NULL)
        throw soclib::exception::RunTimeError(
            std::string("Cant open binary image ")+m_filename);
    
    size_t disksize;
    fseek(diskimg, 0, SEEK_END);
    disksize = ftell(diskimg);
    fseek(diskimg, 0, SEEK_SET);

    if (m_vci_fsm.getSize(0) < disksize)
        throw soclib::exception::RunTimeError(
            std::string("Segment too small for binary image ")+m_filename);

    size_t res;
    res = fread(&m_contents[0], 1, disksize, diskimg);

    if (res != disksize)
        throw soclib::exception::RunTimeError(
            std::string("Error while reading binary image ")+m_filename);

    fclose(diskimg);
}

tmpl(void)::reset()
{
    memset(&m_contents[0], 0, m_vci_fsm.getSize(0));
    m_cpt_read = 0;
    m_cpt_write = 0;
    m_cpt_idle = 0;
}

tmpl(bool)::on_write(size_t seg, vci_addr_t addr, vci_data_t data, int be)
{
    int index = addr / vci_param::B;
    ram_t *tab = m_contents;
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
    m_cpt_write++;

    return true;
}

tmpl(bool)::on_read(size_t seg, vci_addr_t addr, vci_data_t &data )
{
	data = m_contents[addr / vci_param::B];
    m_cpt_read++;
	return true;
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_fsm.reset();
		reset();
		reload();
		return;
	}
	m_vci_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_fsm.genMoore();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

