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

#include "soclib/timer.h"
#include "system.h"

#include "../segmentation.h"


int main(void) 
{		
	const int cpu = procnum();
	int *q0=(int*)0xD0200010;
	int *p0=(int*)0xB0200000;	
	int *q1=(int*)0xD0200000;
	int *p1=(int*)0xB0200010;
	
	if(cpu==0) {
		uputs(">>>>>>>>>> MIPS_0 <<<<<<<<<< ");        	
	        putc('\n');
		*(p0+2) = 15000; 		// Initalize timer and run
	   	*(p0+1) = 3;

		*(q0+1) = 0x1234;	//envoi donnée
		*(q0) = 0xABCD;
	} else if(cpu==1) {
		uputs(">>>>>>>>>> MIPS_1 <<<<<<<<<< ");           
                putc('\n');
                *(p1+2) = 10000;                 // Initalize timer and run
                *(p1+1) = 3;

		*(q1+1) = 0x2099;	//envoi donnée
		*(q1) = 0x2BCD;
	}

	while(1);	
	return 0;
}
