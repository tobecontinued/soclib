/*
 *
 * SOCLIB_GPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU GPLv2.
 * 
 * SoCLib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 * 
 * SOCLIB_GPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#include "system.h"


//this asm function is the same as the ANSI C one written below. It is paste from the disassembled code of the C function generated by GCC
//When this function is used instead of the C one and when you replace the line "ldrb    r3, [fp, #-13]" by " mov    r3,r0", the function works fine.
//Then if you add this instruction " ldrb    r5, [fp, #-13]" (which does nothing useful) before this one " strb    r3, [r2]", the function doesn't work correctly although there is no reason.
//Note that this function works when used on the Unisim simulator so the bug must be in the wrapper

//When the function doesn't work (with the ldrb instruction), the PC is set to 0 on the last instruction of the function. And at the address 0, there is the putc function. So the print of the same character start again...

/*
asm(
".text								\n"
".global putc							\n"
"putc:								\n"
" mov     ip, sp						\n"
" stmdb   sp!, {fp, ip, lr, pc}					\n"
" sub     fp, ip, #4     					\n"
" sub     sp, sp, #8      					\n"
" mov     r3, r0						\n"
" strb    r3, [fp, #-13]					\n"
" mov     r3, #-1073741824        				\n"
" add     r3, r3, #2097152        				\n"
" str     r3, [fp, #-20]					\n"
" ldr     r2, [fp, #-20]					\n"
" ldrb    r3, [fp, #-13]					\n"
" strb    r3, [r2]						\n"
" sub     sp, fp, #12     					\n"
" ldmia   sp, {fp, sp, pc}					\n"
);*/


void putc(const char x)
{
	char* addr = (void*)TTY_BASE;
	*addr = x;
}



char getc(void)
{
	return soclib_io_read8(
		base(TTY),
		TTY_READ);
}

void puti(const int i)
{

	int r = i&0xf;

	if ( i>0xf )
		puti(i>>4);
	if ( r < 10 )
		putc(r+'0');
	else
		putc(r+'a'-10);
}

void uputs(const char *str)
{
	while (*str)
		putc(*str++);
}

void simpleAdd(void) {

	short arf = 3;
	short pof = 9;
	short mwe;

	mwe = arf*3+arf+pof;

}
