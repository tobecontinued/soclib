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
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 */

extern "C" {
#include <bfd.h>

/*
To: "Robert Norton" <rnorton at broadcom dot com>
Cc: binutils at sourceware dot org
Subject: Re: BFD and Elf Symbol Size
From: Ian Lance Taylor <iant at google dot com>
Date: 19 Oct 2007 08:29:27 -0700
In-Reply-To: <B0D822BFECD50F4991F2516EA50F273C030480A5 at NT-IRVA-0752 dot brcm dot ad dot broadcom dot com>
Message-ID: <m33aw72kdk.fsf@localhost.localdomain>

"Robert Norton" <rnorton@broadcom.com> writes:

> Is there any way to get the size of an elf symbol via the BFD?

In general I would have to recommend against using BFD as a general
purpose library.

That said, sure, you can do anything.  In this case,

#include "elf-bfd.h"

and then

((elf_symbol_type *) symbol)->internal_elf_sym.st_size;

Ian
*/

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
#include "static_init_code.h"
#include "loader.h"

namespace soclib { namespace common { namespace {

struct elf_state
{
    uintptr_t mask;
	Loader *loader;
};

void add_section_cb(bfd *bfd, asection *sect, void *state_)
{
	struct elf_state *state = (struct elf_state*)state_;

    if ( !(sect->flags & SEC_ALLOC) )
        return;

    bfd_byte *blob = 0;
    if ( sect->flags & SEC_LOAD && sect->size ) {
        int ret = bfd_malloc_and_get_section(bfd, sect, &blob);
        if ( !ret || !blob )
            throw soclib::exception::RunTimeError(
                std::string("BFD failed: ") + bfd_errmsg(bfd_get_error()));
    }

    uint32_t flags = 0;
    if ( sect->flags & SEC_LOAD ) flags |= BinaryFileSection::FLAG_LOAD;
    if ( sect->flags & SEC_READONLY ) flags |= BinaryFileSection::FLAG_READONLY;
    if ( sect->flags & SEC_CODE ) flags |= BinaryFileSection::FLAG_CODE;
    if ( sect->flags & SEC_DATA ) flags |= BinaryFileSection::FLAG_DATA;

    state->loader->addSection(
		BinaryFileSection(
			sect->name,
			sect->vma & state->mask, sect->lma & state->mask,
			flags,
			sect->size, blob ));
}

void read_symbols(struct bfd *abfd, struct elf_state *state)
{
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

        uintptr_t addr = ((s->section->lma & state->mask) + s->value);
        state->loader->addSymbol(BinaryFileSymbol( s->name, addr, symsize ));
    }
    free(symbol_table);
}

bool elf_load( const std::string &filename, Loader &loader )
{
	static int s_refcount = 0;
	if ( s_refcount == 0 )
		bfd_init();
	++s_refcount;

	struct elf_state state;
	state.loader = &loader;

	struct bfd *a_bfd = bfd_openr(filename.c_str(), "elf32-little");
    if ( a_bfd == NULL )
        a_bfd = bfd_openr(filename.c_str(), "elf32-big");
	if ( a_bfd == NULL )
		throw soclib::exception::RunTimeError(
            std::string("Cant open binary image ")+filename);
	
	if ( !bfd_check_format(a_bfd, bfd_object)
		 && !(a_bfd->flags & EXEC_P))
		throw soclib::exception::RunTimeError(
            std::string("Invalid ELF format in image ")+filename);

    state.mask = (uintptr_t)-1;
    if ( (size_t)bfd_get_arch_size(a_bfd) < sizeof(uintptr_t)*8 )
        state.mask = ((uintptr_t)1<<bfd_get_arch_size(a_bfd))-1;

	bfd_map_over_sections(a_bfd,
                          &add_section_cb,
                          &state);

    read_symbols(a_bfd, &state);

	if ( a_bfd )
		bfd_close(a_bfd);

	return true;
}

STATIC_INIT_CODE(
	Loader::register_loader("elf", elf_load);
)

}}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

