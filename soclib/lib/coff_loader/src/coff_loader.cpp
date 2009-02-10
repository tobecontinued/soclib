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
 * coff_loader.cpp : COFF file reader
 * 
 */

#include <algorithm>
#include "exception.h"
#include "soclib_endian.h"
#include "coff_loader.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace soclib {
namespace common {

CoffLoader::CoffLoader(const std::string &filename) :
	m_filename(filename) {
	struct stat file_stat;
	std::ifstream coff_file;
	std::ifstream::pos_type size_coff_file;

	// open the file for reading
	coff_file.open(m_filename.c_str(), std::ios::in|std::ios::ate);
	if (!coff_file.is_open()) {
		throw soclib::exception::RunTimeError(std::string("Cant open binary image ")+m_filename+
				std::string(" for reading "));
	}
	// is it a non empty file ? look at the size of the binary file
	if (stat(m_filename.c_str(), &file_stat)!= 0) {
		throw soclib::exception::RunTimeError(std::string("Cant open binary image ")+m_filename+
				std::string(" for getting file size "));
	}

	// reading of the complete binary file
	size_coff_file = coff_file.tellg();
	coff_buffer = new char [size_coff_file];
	coff_file.seekg(0, std::ios::beg);
	coff_file.read(coff_buffer, size_coff_file);
	coff_file.close();
	file_ptr = coff_buffer;
#if DEBUG_COFF
	std::cout << "Target binary " << m_filename << " read in " << size_coff_file << " bytes" << std::endl;
#endif
}

CoffLoader::CoffLoader(const CoffLoader &ref) :
	m_filename(ref.m_filename) {
	struct stat file_stat;
	std::ifstream coff_file;
	std::ifstream::pos_type size_coff_file;

	// open the file for reading
	coff_file.open(m_filename.c_str(), std::ios::in|std::ios::ate);
	if (!coff_file.is_open()) {
		throw soclib::exception::RunTimeError(std::string("Cant open binary image ")+m_filename+
				std::string(" for reading "));
	}
	// is it a non empty file ? look at the size of the binary file
	if (stat(m_filename.c_str(), &file_stat)!= 0) {
		throw soclib::exception::RunTimeError(std::string("Cant open binary image ")+m_filename+
				std::string(" for getting file size "));
	}

	// reading of the complete binary file
	size_coff_file = coff_file.tellg();
	coff_buffer = new char [size_coff_file];
	coff_file.seekg(0, std::ios::beg);
	coff_file.read(coff_buffer, size_coff_file);
	coff_file.close();
	file_ptr = coff_buffer;
#if DEBUG_COFF
	std::cout << "Target binary " << m_filename << " read in " << size_coff_file << " bytes" << std::endl;
#endif
	process_headers();
}

void CoffLoader::process_headers() {
	coff_file_header * file_header;
	coff_file_optional_header * file_optional_header;

	file_header = (coff_file_header *) file_ptr;
	byte_swapped = false;

#if DEBUG_COFF
	std::cout << "File header" << std::endl;
	std::cout << "	Magic number: " << std::hex << std::showbase << file_header->f_magic << std::dec << std::endl <<
	"	number of sections: " << file_header->f_nscns << std::endl <<
	"	time & date: " << file_header->f_timdat << std::endl <<
	"	file pointer to symtab: " << std::hex << std::showbase << file_header->f_symptr << std::dec << std::endl <<
	"	number of symtab entries: " << file_header->f_nsyms << std::endl <<
	"	size of opthdr: " << file_header->f_opthdr << std::endl <<
	"	flags: " << std::hex << std::showbase << file_header->f_flags << std::dec << std::endl;
#endif

	// check magic number 
	if (!ISCOFF(file_header->f_magic)) {
		soclib::endian::uint16_swap(file_header->f_magic);
		if (!ISCOFF(file_header->f_magic)) {
			std::ostringstream oss;
			oss << file_header->f_magic;
			std::string magic = oss.str();
			throw soclib::exception::RunTimeError(std::string("Bad magic number ")+magic+
					std::string(" in target binary ")+m_filename+
					std::string(" (not an executable)") );
		}
		byte_swapped = true;

		// swap the rest of the header
		soclib::endian::uint16_swap(file_header->f_nscns);
		soclib::endian::uint32_swap(file_header->f_timdat);
		soclib::endian::uint32_swap(file_header->f_symptr);
		soclib::endian::uint32_swap(file_header->f_nsyms);
		soclib::endian::uint16_swap(file_header->f_opthdr);
		soclib::endian::uint16_swap(file_header->f_flags);
		soclib::endian::uint16_swap(file_header->f_target_id);
	}

	// goto the optional header
	// location of file_optional_header does not rely on sizeof(coff_file_header)
	// we only have to read 22 bytes
	file_ptr = file_ptr + 22;
	file_optional_header = (coff_file_optional_header *) file_ptr;

	// swap bytes in the file_optional_header if necessary
	if (byte_swapped) {
		soclib::endian::uint16_swap(file_optional_header->magic);
		soclib::endian::uint16_swap(file_optional_header->vstamp);
		soclib::endian::uint32_swap(file_optional_header->tsize);
		soclib::endian::uint32_swap(file_optional_header->dsize);
		soclib::endian::uint32_swap(file_optional_header->bsize);
		soclib::endian::uint32_swap(file_optional_header->entrypt);
		soclib::endian::uint32_swap(file_optional_header->text_start);
		soclib::endian::uint32_swap(file_optional_header->data_start);
	}

#if DEBUG_COFF
	std::cout << "Byte swapped: " << byte_swapped << std::endl;
	std::cout << "Optional File header" << std::endl;
	std::cout << "	Type of file: " << std::hex << std::showbase << file_optional_header->magic << std::dec << std::endl <<
	"	version stamp: " << file_optional_header->vstamp << std::endl <<
	"	text size (in bytes): " << file_optional_header->tsize << std::endl <<
	"	initialized data: " << file_optional_header->dsize << std::dec << std::endl <<
	"	uinitialized data : " << file_optional_header->bsize << std::endl <<
	"	entry pt: " << file_optional_header->entrypt << std::endl <<
	"	base of text used for this file: " << std::hex << std::showbase << file_optional_header->text_start << std::dec << std::endl <<
	"	base of data used for this file: " << std::hex << std::showbase << file_optional_header->data_start << std::dec << std::endl;
#endif

#if DEBUG_COFF
	std::cout << "Processing " << file_header->f_nscns << " sections" << std::endl;
#endif

	// ready to iterate through the section headers and then the sections (see load)
	file_ptr = file_ptr + sizeof(coff_file_optional_header);
	file_ptr_saved = file_ptr;
	file_ptr_sections = file_ptr;
	nbOfSections = file_header->f_nscns;
}

void CoffLoader::load(void *buffer, uintptr_t address, size_t length) {
	coff_file_section_header * file_section_header;

#if DEBUG_COFF
	std::cout << "buffer:  " << buffer << " address:  " << std::hex << std::showbase << address <<
	" length: " << length << std::dec << std::endl;
#endif

#if DEBUG_COFF
	std::cout << "Processing " << nbOfSections << " sections" << std::endl;
#endif

	file_ptr = file_ptr_sections;

	for (int section = 0; section < nbOfSections; section++) {

		file_section_header = (coff_file_section_header *) file_ptr;
		file_ptr += sizeof(coff_file_section_header);
		file_ptr_saved = file_ptr;

		file_ptr = coff_buffer + file_section_header->s_scnptr;

		/* swap bytes in the file_section_header if necessary, except s_name */
		if (byte_swapped) {
			soclib::endian::uint32_swap(file_section_header->s_paddr);
			soclib::endian::uint32_swap(file_section_header->s_vaddr);
			soclib::endian::uint32_swap(file_section_header->s_size);
			soclib::endian::uint32_swap(file_section_header->s_scnptr);
			soclib::endian::uint32_swap(file_section_header->s_relptr);
			soclib::endian::uint32_swap(file_section_header->s_lnnoptr);
			soclib::endian::uint32_swap(file_section_header->s_nreloc);
			soclib::endian::uint32_swap(file_section_header->s_nlnno);
			soclib::endian::uint32_swap(file_section_header->s_flags);
			soclib::endian::uint16_swap(file_section_header->s_reserved);
			soclib::endian::uint16_swap(file_section_header->s_page);
		}

#if DEBUG_COFF
		std::cout << "section " << section << std::endl;
		std::cout << "	Section header" << std::endl <<
		"		name: " << std::string(file_section_header->s_name) << std::endl <<
		"		physical address: " << std::hex << std::showbase << file_section_header->s_paddr << std::endl <<
		"		virtual address: " << file_section_header->s_vaddr << std::endl << std::dec <<
		"		section size: " << file_section_header->s_size << std::endl <<
		"		file ptr ro raw data for section: " << std::hex << std::showbase << file_section_header->s_scnptr << std::endl <<
		"		file ptr to relocation: " << file_section_header->s_relptr << std::endl <<
		"		file ptr to line numbers: " << file_section_header->s_lnnoptr << std::endl << std::dec <<
		"		number of relocation entries: " << file_section_header->s_nreloc << std::endl <<
		"		number of line number entries: " << file_section_header->s_nlnno << std::endl <<
		"		flags: " << std::hex << std::showbase << file_section_header->s_flags << std::dec << std::endl;
#endif	
		// is it a relevant section xundo
		if (((file_section_header->s_flags & COFF_SCNTYPE_TEXT)
				|| (file_section_header->s_flags & COFF_SCNTYPE_DATA1)
				|| (file_section_header->s_flags & COFF_SCNTYPE_DATA))
				&& (file_section_header->s_flags != COFF_SCNTYPE_BSS)
				&& file_section_header->s_paddr >= address
				&& file_section_header->s_paddr < address+length) {

#if DEBUG_COFF
			std::cout << "section " << section << std::endl;
			std::cout << "	Section header" << std::endl <<
			"		name: " << std::string(file_section_header->s_name) << std::endl <<
			"		physical address: " << std::hex << std::showbase << file_section_header->s_paddr << std::endl <<
			"		virtual address: " << file_section_header->s_vaddr << std::endl << std::dec <<
			"		section size: " << file_section_header->s_size << std::endl <<
			"		file ptr ro raw data for section: " << std::hex << std::showbase << file_section_header->s_scnptr << std::endl <<
			"		file ptr to relocation: " << file_section_header->s_relptr << std::endl <<
			"		file ptr to line numbers: " << file_section_header->s_lnnoptr << std::endl << std::dec <<
			"		number of relocation entries: " << file_section_header->s_nreloc << std::endl <<
			"		number of line number entries: " << file_section_header->s_nlnno << std::endl <<
			"		flags: " << std::hex << std::showbase << file_section_header->s_flags << std::dec << std::endl;
#endif		

			if (file_section_header->s_size > 0) {
#if DEBUG_COFF
				std::cout << "Copies the values of " << file_section_header->s_size <<
				" bytes from the location pointed by " << (void *)file_ptr << " to the memory block pointed by " << (void *)buffer << std::endl;
#endif
				for (unsigned int j = 0; j < file_section_header->s_size; j+=4) {
					uint8_t a, b, c, d;
					uint32_t data;
					a = *(file_ptr+j);
					b = *(file_ptr+j+1);
					c = *(file_ptr+j+2);
					d = *(file_ptr+j+3);
					data = (d << 24) | (c << 16) | (b << 8) | (a);
#if DEBUG_COFF
					std::cout << "	" << std::hex << std::showbase << "(" << file_section_header->s_vaddr + j << ") " << data << std::dec << std::endl;
#endif
				}
				memcpy((void *)((uintptr_t)buffer+(file_section_header->s_paddr
						-address)), (void *)(uintptr_t)(file_ptr),
						file_section_header->s_size);
			}
		}
		file_ptr = file_ptr_saved;
	}
}

void CoffLoader::print(std::ostream &o) const {
	coff_file_header * file_header;
	file_header = (coff_file_header *) file_ptr;

	o << "<CoffLoader " << m_filename << std::endl << " Magic number: "
			<< std::hex << std::showbase << file_header->f_magic << std::dec
			<< "\n Number of sections: " << file_header->f_nscns
			<< "\n Target: " << file_header->f_target_id << ">" << std::endl;
}

void CoffLoader::analyseCoffFileForDebug() {
	bool byte_swapped = false;
	coff_file_header * file_header;
	coff_file_optional_header * file_optional_header;
	coff_file_section_header * file_section_header;

	file_header = (coff_file_header *) file_ptr;

#if DEBUG_COFF
	std::cout << "File header" << std::endl;
	std::cout << "	Magic number: " << std::hex << std::showbase << file_header->f_magic << std::dec << std::endl <<
	"	number of sections: " << file_header->f_nscns << std::endl <<
	"	time & date: " << file_header->f_timdat << std::endl <<
	"	file pointer to symtab: " << std::hex << std::showbase << file_header->f_symptr << std::dec << std::endl <<
	"	number of symtab entries: " << file_header->f_nsyms << std::endl <<
	"	size of opthdr: " << file_header->f_opthdr << std::endl <<
	"	flags: " << std::hex << std::showbase << file_header->f_flags << std::dec << std::endl;
#endif

	// check magic number 
	if (!ISCOFF(file_header->f_magic)) {
		soclib::endian::uint16_swap(file_header->f_magic);
		if (!ISCOFF(file_header->f_magic)) {
			std::ostringstream oss;
			oss << file_header->f_magic;
			std::string magic = oss.str();
			throw soclib::exception::RunTimeError(std::string("Bad magic number ")+magic+
					std::string(" in target binary ")+m_filename+
					std::string(" (not an executable)") );
		}
		byte_swapped = true;

		// swap the rest of the header
		soclib::endian::uint16_swap(file_header->f_nscns);
		soclib::endian::uint32_swap(file_header->f_timdat);
		soclib::endian::uint32_swap(file_header->f_symptr);
		soclib::endian::uint32_swap(file_header->f_nsyms);
		soclib::endian::uint16_swap(file_header->f_opthdr);
		soclib::endian::uint16_swap(file_header->f_flags);
		soclib::endian::uint16_swap(file_header->f_target_id);
	}

	// goto the optional header
	// location of file_optional_header does not rely on sizeof(coff_file_header)
	// we only have to read 22 bytes
	file_ptr = file_ptr + 22;
	file_optional_header = (coff_file_optional_header *) file_ptr;

	// swap bytes in the file_optional_header if necessary
	if (byte_swapped) {
		soclib::endian::uint16_swap(file_optional_header->magic);
		soclib::endian::uint16_swap(file_optional_header->vstamp);
		soclib::endian::uint32_swap(file_optional_header->tsize);
		soclib::endian::uint32_swap(file_optional_header->dsize);
		soclib::endian::uint32_swap(file_optional_header->bsize);
		soclib::endian::uint32_swap(file_optional_header->entrypt);
		soclib::endian::uint32_swap(file_optional_header->text_start);
		soclib::endian::uint32_swap(file_optional_header->data_start);
	}

#if DEBUG_COFF
	std::cout << "Byte swapped: " << byte_swapped << std::endl;
	std::cout << "Optional File header" << std::endl;
	std::cout << "	Type of file: " << std::hex << std::showbase << file_optional_header->magic << std::dec << std::endl <<
	"	version stamp: " << file_optional_header->vstamp << std::endl <<
	"	text size (in bytes): " << file_optional_header->tsize << std::endl <<
	"	initialized data: " << file_optional_header->dsize << std::dec << std::endl <<
	"	uinitialized data : " << file_optional_header->bsize << std::endl <<
	"	entry pt: " << file_optional_header->entrypt << std::endl <<
	"	base of text used for this file: " << std::hex << std::showbase << file_optional_header->text_start << std::dec << std::endl <<
	"	base of data used for this file: " << std::hex << std::showbase << file_optional_header->data_start << std::dec << std::endl;
#endif

#if DEBUG_COFF
	std::cout << "Processing " << file_header->f_nscns << " sections" << std::endl;
#endif

	// iterate through the section headers and then the sections
	file_ptr = file_ptr + sizeof(coff_file_optional_header);
	file_ptr_saved = file_ptr;

	for (int i = 0; i < file_header->f_nscns; i++) {
		file_ptr = file_ptr_saved;
		file_section_header = (coff_file_section_header *) file_ptr;
		file_ptr += sizeof(coff_file_section_header);
		file_ptr_saved = file_ptr;

		/* swap bytes in the file_section_header if necessary, except s_name */
		if (byte_swapped) {
			soclib::endian::uint32_swap(file_section_header->s_paddr);
			soclib::endian::uint32_swap(file_section_header->s_vaddr);
			soclib::endian::uint32_swap(file_section_header->s_size);
			soclib::endian::uint32_swap(file_section_header->s_scnptr);
			soclib::endian::uint32_swap(file_section_header->s_relptr);
			soclib::endian::uint32_swap(file_section_header->s_lnnoptr);
			soclib::endian::uint32_swap(file_section_header->s_nreloc);
			soclib::endian::uint32_swap(file_section_header->s_nlnno);
			soclib::endian::uint32_swap(file_section_header->s_flags);
			soclib::endian::uint16_swap(file_section_header->s_reserved);
			soclib::endian::uint16_swap(file_section_header->s_page);
		}

#if DEBUG_COFF
		std::cout << "	Section header" << std::endl <<
		"		name: " << std::string(file_section_header->s_name) << std::endl <<
		"		physical address: " << std::hex << std::showbase << file_section_header->s_paddr << std::endl <<
		"		virtual address: " << file_section_header->s_vaddr << std::endl << std::dec <<
		"		section size: " << file_section_header->s_size << std::endl <<
		"		file ptr to raw data for section: " << std::hex << std::showbase << file_section_header->s_scnptr << std::endl <<
		"		file ptr to relocation: " << file_section_header->s_relptr << std::endl <<
		"		file ptr to line numbers: " << file_section_header->s_lnnoptr << std::endl << std::dec <<
		"		number of relocation entries: " << file_section_header->s_nreloc << std::endl <<
		"		number of line number entries: " << file_section_header->s_nlnno << std::endl <<
		"		flags: " << std::hex << std::showbase << file_section_header->s_flags << std::dec << std::endl;
#endif

		if (file_section_header->s_flags & COFF_SCNTYPE_TEXT) {
#if DEBUG_COFF
			std::cout << "	Section " << std::string(file_section_header->s_name) << " starting at " <<
			std::hex << std::showbase << file_section_header->s_vaddr << " with " << std::dec <<
			file_section_header->s_size << " bytes " << std::endl;
#endif
			file_ptr = coff_buffer + file_section_header->s_scnptr;
			if (file_section_header->s_size > 0) {
#if DEBUG_COFF
				std::cout << "	Instructions " << std::endl;
#endif				
				for (unsigned int j = 0; j < file_section_header->s_size; j+=4) {
					uint8_t a, b, c, d;
					uint32_t data;
					a = *(file_ptr+j);
					b = *(file_ptr+j+1);
					c = *(file_ptr+j+2);
					d = *(file_ptr+j+3);
					data = (d << 24) | (c << 16) | (b << 8) | (a);
#if DEBUG_COFF
					std::cout << "	" << std::hex << std::showbase << "(" << file_section_header->s_vaddr + j << ") " << data << std::dec << std::endl;
					//	      soclib::tms320c6x::instruction_print(data);

#endif
				}
			}
		} else if (file_section_header->s_flags & COFF_SCNTYPE_DATA) {
			if (file_section_header->s_size > 0) {
#if DEBUG_COFF
				std::cout << "Section " << file_section_header->s_name << " starting at " <<
				std::hex << std::showbase << file_section_header->s_vaddr << " with " << std::dec <<
				file_section_header->s_size << " bytes " << std::endl;
#endif
				file_ptr = coff_buffer + file_section_header->s_scnptr;
#if DEBUG_COFF
				std::cout << "	Data " << std::endl;
#endif
				for (unsigned int j = 0; j < file_section_header->s_size; j+=4) {
					uint8_t a, b, c, d;
					uint32_t data;
					a = *(file_ptr+j);
					b = *(file_ptr+j+1);
					c = *(file_ptr+j+2);
					d = *(file_ptr+j+3);
					data = (d << 24) | (c << 16) | (b << 8) | (a);
#if DEBUG_COFF
					std::cout << "	" << std::hex << std::showbase << "(" << file_section_header->s_vaddr + j << ") " << std::dec << data << std::endl;
#endif
				}
			}
		} else {
#if DEBUG_COFF
			std::cout << "	Section " << file_section_header->s_name << " ignored" << std::endl;
#endif
		}
	}
}

CoffLoader::~CoffLoader() {

}

}
}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
