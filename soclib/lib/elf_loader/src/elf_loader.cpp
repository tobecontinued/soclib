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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 */

extern "C" {
#include <bfd.h>

// To: "Robert Norton" <rnorton at broadcom dot com>
// Cc: binutils at sourceware dot org
// Subject: Re: BFD and Elf Symbol Size
// From: Ian Lance Taylor <iant at google dot com>
// Date: 19 Oct 2007 08:29:27 -0700
// In-Reply-To: <B0D822BFECD50F4991F2516EA50F273C030480A5 at NT-IRVA-0752 dot brcm dot ad dot broadcom dot com>
// Message-ID: <m33aw72kdk.fsf@localhost.localdomain>
// 
// "Robert Norton" <rnorton@broadcom.com> writes:
// 
// > Is there any way to get the size of an elf symbol via the BFD?
// 
// In general I would have to recommend against using BFD as a general
// purpose library.
// 
// That said, sure, you can do anything.  In this case,
// 
// #include "elf-bfd.h"
// 
// and then
// 
// ((elf_symbol_type *) symbol)->internal_elf_sym.st_size;
// 
// Ian

// Unfortunately, elf-bfd.h is not a distributed header, therefore
// i'll copy here the only needed parts of the struct.

struct elf_internal_sym {
    bfd_vma	st_value;		/* Value of the symbol */
    bfd_vma	st_size;		/* Associated symbol size */
    // Other fields I skipped
};

typedef struct
{
    asymbol symbol;
    struct elf_internal_sym internal_elf_sym;
    // Other fields I skipped
} elf_symbol_type;

}

#include <algorithm>
#include <string.h>
#include <sstream>
#include <cassert>

#include "exception.h"
#include "elf_loader.h"

namespace soclib { namespace common {

int ElfLoader::s_refcount = 0;

void ElfLoader::add_section_cb(void *bfd, void *_sect, void *thisptr)
{
    static_cast<ElfLoader*>(thisptr)->add_section(bfd, _sect);
}

void ElfLoader::add_section(void *_bfd, void *_sect)
{
    bfd *a_bfd = (bfd*)_bfd;
    asection *sect = (asection*)_sect;

    if ( !(sect->flags & SEC_ALLOC) )
        return;

    bfd_byte *blob = 0;
    if ( sect->flags & SEC_LOAD ) {
        int ret = bfd_malloc_and_get_section(a_bfd, sect, &blob);
        assert(ret && blob && "BFD failed");
    }

    m_sections.push_back(ElfSection(
                             sect->name,
                             sect->vma & m_mask, sect->lma & m_mask,
                             sect->flags,
                             sect->size, blob ));
}

void ElfLoader::read_symbols()
{
    struct bfd *abfd = (struct bfd*)m_bfd_ptr;
    long storage_needed;
    asymbol **symbol_table;
    long number_of_symbols;
    long i;

    storage_needed = bfd_get_symtab_upper_bound (abfd);

    if (storage_needed <= 0)
        return;

    symbol_table = (asymbol**)malloc (storage_needed);
    number_of_symbols =
        bfd_canonicalize_symtab (abfd, symbol_table);

    if (number_of_symbols < 0)
        return;

    for (i = 0; i < number_of_symbols; i++) {
        asymbol *s = symbol_table[i];

        size_t symsize = ((elf_symbol_type *) s)->internal_elf_sym.st_size;

        if ( ! (s->flags & (BSF_FUNCTION|BSF_LOCAL|BSF_GLOBAL)) )
            continue;

        uintptr_t addr = ((s->section->lma & m_mask) + s->value);
        m_symbol_table[addr] = std::pair<size_t, std::string>(symsize, std::string(s->name));
    }
    free(symbol_table);
}

ElfLoader::ElfLoader( const std::string &filename )
	: m_filename(filename)
{
	if ( s_refcount == 0 )
		bfd_init();
	++s_refcount;

	struct bfd *a_bfd = bfd_openr(m_filename.c_str(), NULL);
	if ( !(bool)a_bfd )
		throw soclib::exception::RunTimeError(
            std::string("Cant open binary image ")+m_filename);
	
	if ( !bfd_check_format(a_bfd, bfd_object)
		 && !(a_bfd->flags & EXEC_P))
		throw soclib::exception::RunTimeError(
            std::string("Invalid ELF format in image ")+m_filename);
	m_bfd_ptr = (void*)a_bfd;

    m_mask = (uintptr_t)-1;
    if ( (size_t)bfd_get_arch_size(a_bfd) < sizeof(uintptr_t)*8 )
        m_mask = ((uintptr_t)1<<bfd_get_arch_size(a_bfd))-1;

    m_sections.reserve(a_bfd->section_count);

	bfd_map_over_sections(a_bfd,
                          (void(*)(bfd*, asection*, void*))
                          &ElfLoader::add_section_cb,
                          this);

    read_symbols();
}

ElfLoader::ElfLoader( const ElfLoader &ref )
	: m_filename(ref.m_filename),
      m_sections(ref.m_sections),
      m_mask(ref.m_mask),
      m_symbol_table(ref.m_symbol_table)
{
	if ( s_refcount == 0 )
		bfd_init();
	++s_refcount;

	struct bfd *a_bfd = bfd_openr(m_filename.c_str(), NULL);
	if ( !(bool)a_bfd )
		throw soclib::exception::RunTimeError(
            std::string("Cant open binary image ")+m_filename);
	
	if ( !bfd_check_format(a_bfd, bfd_object)
		 && !(a_bfd->flags & EXEC_P))
		throw soclib::exception::RunTimeError(
            std::string("Invalid ELF format in image ")+m_filename);
	m_bfd_ptr = (void*)a_bfd;
}

void ElfLoader::load( void *buffer, uintptr_t address, size_t length )
{
    std::cout
        << std::showbase << std::hex
        << "Loading at " << address
        << " size " << std::dec << length
        << ": ";
    bool one = false;
    for ( section_list_t::const_iterator i = m_sections.begin();
          i != m_sections.end();
          ++i ) {
        if ( i->load_overlap_in_buffer( buffer, address, length ) ) {
            one = true;
            std::cout << i->name() << " ";
        }
    }
    if ( !one )
        std::cout << "nothing";
    std::cout << std::endl;
}

void ElfLoader::print( std::ostream &o ) const
{
    struct bfd* a_bfd = (struct bfd*)m_bfd_ptr;
	o << "<ElfLoader " << m_filename << std::endl
      << " target: " << arch() << std::endl
      << " endianness: " << a_bfd->xvec->byteorder << std::endl;
    for ( section_list_t::const_iterator i = m_sections.begin();
          i != m_sections.end();
          ++i )
        o << " " << *i << std::endl;
    o << ">" << std::endl;
}

std::string ElfLoader::arch() const
{
    struct bfd *a_bfd = (struct bfd*)m_bfd_ptr;
    enum bfd_architecture arch = bfd_get_arch (a_bfd);
    std::cout << "Arch " << (int)arch << std::endl;
    switch(arch) {
    case bfd_arch_mips:
        return "mipsel";
    case bfd_arch_powerpc:
        return "powerpc";
    default:
        return "unknown";
    }
}

ElfLoader::~ElfLoader()
{
    struct bfd* a_bfd = (struct bfd*)m_bfd_ptr;
	if ( a_bfd )
		bfd_close(a_bfd);

// 	if ( --s_refcount == 0 )
// 		bfd_fini();
}

std::vector<ElfSection> ElfLoader::sections() const
{
    return m_sections;
}

std::string ElfLoader::get_symbol( uintptr_t addr ) const
{
    addr &= m_mask;
    std::map<uintptr_t, std::pair<size_t, std::string> >::const_iterator i =
        m_symbol_table.lower_bound( addr );

    if ( i != m_symbol_table.end()
         && i != m_symbol_table.begin()
         && i->first > addr )
        --i;

    std::ostringstream o;
    o << "[@" << std::showbase << std::hex
      << addr << ": ";
    if ( i == m_symbol_table.end() || ( i->second.first != 0 && i->first+i->second.first <= addr ) ) {
        o << "<unknown>";
    } else {
        ptrdiff_t d = addr - i->first;
        o << '(' << i->second.second << " + " << d << ')';
    }
    o << "]";
    return o.str();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

