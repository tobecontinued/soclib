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
* Copyright (c) Lab-STICC, UBS
 *         Caaliph Andriamisaina <andriami@univ-ubs.fr>, 2008
 *
 * Based on previous works by Sebastien Tregaro, 2005
 */

#include "system.h"
#include "soclib/timer.h"

void uputs(const char *str)
{
	while (*str)
		putc(*str++);
}

void puti(const int i)
{
	if ( i>10 )
		puti(i/10);
	putc(i%10+'0');
/* 	int j = i&0xf; */

/* 	if ( i == 0 ) { */
/* 		putc('0'); */
/* 		return; */
/* 	} */

/* 	if ( i>j ) */
/* 		puti(i>>4); */
/* 	if ( j>9 ) */
/* 		putc(j-10+'a'); */
/* 	else */
/* 		putc(j+'0'); */
}

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	uputs(__FUNCTION__);
	putc(' ');
	puti(type);
	putc('\n');
	while(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	uputs(__FUNCTION__);
	putc('\n');
}

void interrupt_hw_handler(unsigned int irq)
{
	int i;
	int *p=(int*)0xB0200000;
	int *q0=(int*)0xD0200000;	//adresse de base MailBox
	int *q1=(int*)0xD0200010;	//adresse de base MailBox

/* 	uputs(__FUNCTION__); */
/* 	puti(irq); */
/* 	putc('\n'); */

	for (i=0; i<8;++i) {
		if (irq&1)
			break;
		irq>>=1;
	}

	switch (i) {
	case 0:   /* SwIt 0 */
		break;
	case 1:   /* SwIt 1 */
		break;
	case 2:   /* It 0   */
		if(procnum()==0) 
			uputs("It n°0 proc 0");
		else 
			uputs("It n°0 proc 1");

 		*(p+3+(procnum()<<2))=0;
		putc('\n');
		break;           
	case 3:   /* It 1   */
		if(procnum()==0){
			uputs("MailBox MIPS 0");
			*(q0+3)=0;	// remise a zero de l'irq 
		}
		else if(procnum()==1){
			uputs("MailBox MIPS 1");
			*(q1+3)=0;	// remise a zero de l'irq 
		}
                putc('\n');
                break;
	case 4:   /* It 2   */
		break;           
	case 5:   /* It 3   */
		break;           
	case 6:   /* It 4   */
		break;           
	case 7:   /* It 5   */
		break;
	default:
		break;
	}

}
