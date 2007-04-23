/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 */

extern "C" {
#include <bfd.h>
}

#include <algorithm>
#include "common/exception.h"
#include "common/elf_loader.h"

namespace soclib { namespace common {

namespace elf_hacks {

struct desc_t {
	void *buffer;
	uintptr_t address;
	size_t length;
};

static void elf_do_load(bfd *exec, asection *sect, PTR descptr)
{
	desc_t *desc = (desc_t *)descptr;
    bfd_byte *data;

	if ( ! (sect->flags & SEC_LOAD) )
		return;

    uintptr_t lma = sect->lma;

	if ( lma >= (desc->address+desc->length) ||
		 desc->address >= (lma+sect->size) )
		return;

	uintptr_t src_delta = 0, dst_delta = 0;

	if ( desc->address < lma ) {
		dst_delta = lma - desc->address;
	} else {
		src_delta = desc->address - lma;
	}
	size_t length = std::min<size_t>( desc->length - dst_delta,
									  sect->size - src_delta );
    if ( length < sect->size - src_delta )
        std::cerr
            << "Warning: Truncating section "<<sect->name
            << " when loading at 0x"<< std::hex << desc->address << std::endl;

    if (!bfd_malloc_and_get_section(exec, sect, &data))
        throw soclib::exception::RunTimeError(std::string("Unable to get section: ") + sect->name);

	memcpy( (void*)((uintptr_t)(desc->buffer)+dst_delta), data+src_delta, length );
    free(data);
}

} // elf_hacks

int ElfLoader::s_refcount = 0;

ElfLoader::ElfLoader( const std::string &filename )
	: m_filename(filename)
{
	if ( s_refcount == 0 )
		bfd_init();
	++s_refcount;

	struct bfd *m_bfd = bfd_openr(m_filename.c_str(), NULL);
	if ( !(bool)m_bfd )
		throw soclib::exception::RunTimeError(
            std::string("Cant open binary image ")+m_filename);
	
	if ( !bfd_check_format(m_bfd, bfd_object)
		 && !(m_bfd->flags & EXEC_P))
		throw soclib::exception::RunTimeError(
            std::string("Invalid ELF format in image ")+m_filename);
	m_bfd_ptr = (void*)m_bfd;
}

void ElfLoader::load( void *buffer, uintptr_t address, size_t length )
{
	struct bfd* m_bfd = (struct bfd*)m_bfd_ptr;
	elf_hacks::desc_t desc;

	desc.buffer = buffer;
	desc.address = address;
	desc.length = length;

	bfd_map_over_sections(m_bfd, elf_hacks::elf_do_load, &desc);
}

ElfLoader::~ElfLoader()
{
    struct bfd* m_bfd = (struct bfd*)m_bfd_ptr;
	if ( m_bfd )
		bfd_close(m_bfd);

// 	if ( --s_refcount == 0 )
// 		bfd_fini();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

