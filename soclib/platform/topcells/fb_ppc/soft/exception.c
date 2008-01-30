/*
    This file is part of MutekH.

    MutekH is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MutekH is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MutekH; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

    Copyright Alexandre Becoulet <alexandre.becoulet@lip6.fr> (c) 2006

*/

asm(
    ".section        \".text\",\"ax\",@progbits \n"

    ".globl ppc_boot				\n"
	".type  ppc_boot, @function   \n"
    "ppc_boot:					\n"
	
    "	lis   9, _stack@ha				\n"
    "	la   1, _stack@l(9)				\n"
	"   b  main \n"
	".size ppc_boot, .-ppc_boot \n"
	);

asm(
    ".section        \".ppc_boot\",\"ax\",@progbits			\n"

    "	b   ppc_boot				\n"
	);

