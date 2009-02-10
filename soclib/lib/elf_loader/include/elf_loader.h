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
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_ELF_LOADER_H_
#define SOCLIB_ELF_LOADER_H_

#include <string>
#include <vector>
#include <map>
#include "stdint.h"

namespace soclib { namespace common {

class ElfSectionData;
class ElfLoader;

class ElfSection
{
    friend class ElfLoader;

    ElfSectionData *m_data;
    uint32_t m_flags;
    std::string m_name;
    uintptr_t m_vma;
    uintptr_t m_lma;
    size_t m_size;

public:
    bool load_overlap_in_buffer( void *buffer,
                                 uintptr_t buffer_base_address,
                                 uintptr_t buffer_size ) const;
    void get_data( void *buffer ) const;

    bool has_data() const;

    bool flag_load() const;
    bool flag_read_only() const;
    bool flag_code() const;
    bool flag_data() const;
    const std::string& name() const;
    uintptr_t vma() const;
    uintptr_t lma() const;
    size_t size() const;

    ElfSection( const ElfSection & );
    ~ElfSection();
    ElfSection &operator=( const ElfSection & );
    ElfSection();

    void print( std::ostream &o ) const;

    friend std::ostream &operator<<( std::ostream &o, const ElfSection &s )
    {
        s.print(o);
        return o;
    }

private:
    ElfSection( const std::string &name,
                uintptr_t vma, uintptr_t lma,
                uint32_t flags,
                size_t data_size, void *given_data_ptr );
};

class ElfLoader
{
	static int s_refcount;
public:
    typedef std::vector<ElfSection> section_list_t;

private:
	std::string m_filename;
	void *m_bfd_ptr;
    section_list_t m_sections;
    uintptr_t m_mask;
    std::map<uintptr_t, std::pair<size_t, std::string> > m_symbol_table;

    void read_symbols();

    static void add_section_cb( void *bfd, void *sect, void *thisptr );
    void add_section( void *bfd, void *sect );

public:
    std::vector<ElfSection> sections() const;

	ElfLoader( const ElfLoader &ref );
	ElfLoader( const std::string &filename );
	void load( void *buffer, uintptr_t address, size_t length );
	~ElfLoader();

    std::string arch() const;

    void print( std::ostream &o ) const;

    std::string get_symbol( uintptr_t addr ) const;
    uintptr_t get_symbol_addr( const std::string & ) const;

    inline const std::string & filename() const
    {
        return m_filename;
    }

    friend std::ostream &operator << (std::ostream &o, const ElfLoader &el)
    {
        el.print(o);
        return o;
    }
};

}}

#endif /* SOCLIB_ELF_LOADER_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

