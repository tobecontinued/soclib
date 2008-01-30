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

#include "soclib/timer.h"
#include "system.h"

#include "../segmentation.h"

static const int period[4] = {10000};

int main(void)
{
	uint8_t base = 0;

	uputs("Hello from processor ");
	putc(procnum()+'0');
	putc('\n');
	
	while(1) {
		uint8_t *fb = FB_BASE;
		uint32_t x, y;

		for (x=0; x<FB_HEIGHT; ++x) {
			uputs("Filling Y ");
			puti(x);
			putc('\n');
			
			uint8_t lum = (base<<7)+x;
			for (y=0; y<FB_WIDTH; ++y) {
				*fb++ = lum++;
			}
		}

		for (x=0; x<FB_HEIGHT; ++x) {
			uputs("Filling C ");
			puti(x);
			putc('\n');
			
			uint8_t lum = (base<<2)+x;
			for (y=0; y<FB_WIDTH/2; ++y) {
				*fb++ = lum--;
			}
			fb += FB_WIDTH/2;
		}
		++base;
	}
	return 0;
}
