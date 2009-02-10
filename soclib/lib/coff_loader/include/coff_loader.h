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
 * Loading of a coff binary file in a ram
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (C) IRISA/INRIA, 2007-2008
 *         Francois Charot <charot@irisa.fr>
 *
 * 
 * coff_loader.h : COFF file reader
 * 
 * 
 */

#ifndef SOCLIB_COFF_LOADER_H_
#define SOCLIB_COFF_LOADER_H_

#include <string>
#include <stdint.h>

#define DEBUG_COFF 0

#define COFF_MAGIC_2 0302
#define ISCOFF(x) ((x) == COFF_MAGIC_2)

#define COFF_SCNTYPE_TEXT 0x520
#define COFF_SCNTYPE_DATA 0x40
#define COFF_SCNTYPE_DATA1 0x280
#define COFF_SCNTYPE_BSS  0x080

namespace soclib {
namespace common {

class CoffLoader {

	char * coff_buffer;
	char * file_ptr;
	char * file_ptr_saved, * file_ptr_sections;
	bool byte_swapped;
	int nbOfSections;

	struct coff_file_header {
		uint16_t f_magic; /* magic number                         */
		uint16_t f_nscns; /* number of sections                   */
		uint32_t f_timdat; /* time & date stamp                    */
		uint32_t f_symptr; /* file pointer to symtab               */
		uint32_t f_nsyms; /* number of symtab entries             */
		uint16_t f_opthdr; /* sizeof(optional hdr)                 */
		uint16_t f_flags; /* flags                                */
		uint16_t f_target_id; /* target architecture id               */
	};

	struct coff_file_optional_header {
		uint16_t magic; /* type of file    */
		uint16_t vstamp; /* version stamp                        */
		uint32_t tsize; /* text size in bytes, padded to FW bdry*/
		uint32_t dsize; /* initialized data "  "                */
		uint32_t bsize; /* uninitialized data "   "             */
		uint32_t entrypt; /* entry pt.                            */
		uint32_t text_start; /* base of text used for this file      */
		uint32_t data_start; /* base of data used for this file      */
	};

	struct coff_file_section_header {
		char s_name[8]; /* osection name      		       */
		uint32_t s_paddr; /* physical address                    */
		uint32_t s_vaddr; /* virtual address                     */
		uint32_t s_size; /* section size                        */
		uint32_t s_scnptr; /* file ptr to raw data for section    */
		uint32_t s_relptr; /* file ptr to relocation              */
		uint32_t s_lnnoptr; /* file ptr to line numbers            */
		uint32_t s_nreloc; /* number of relocation entries        */
		uint32_t s_nlnno; /* number of line number entries       */
		uint32_t s_flags; /* flags                               */
		uint16_t s_reserved; /* reserved 2 bytes                    */
		uint16_t s_page; /* memory page id                      */
	};

	std::string m_filename;

public:
	CoffLoader(const CoffLoader &ref);
	CoffLoader(const std::string &filename);
	~CoffLoader();

	void load(void *buffer, uintptr_t address, size_t length);

	void print(std::ostream &o) const;
	void analyseCoffFileForDebug();

	friend std::ostream &operator <<(std::ostream &o, const CoffLoader &coff) {
		coff.print(o);
		return o;
	}
private:
	void process_headers();

};

}
}

#endif /* SOCLIB_COFF_LOADER_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
