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

#include "soclib/dma.h"
#include "system.h"

#include "../segmentation.h"

int main(void)
{
	uint8_t offset = 0;
	char _fb[FB_SIZE];

	puts("Hello from processor ");
	putchar(procnum()+'0');
	putchar('\n');
	
	while(1) {
		uint32_t x, y;
		char *fb = _fb;

		for (x=0; x<FB_HEIGHT; ++x) {
			puts("Filling Y ");
			puti(x);
			putchar('\n');
			
			uint8_t lum = (offset<<7)+x;
			for (y=0; y<FB_WIDTH; ++y) {
				*fb++ = lum++;
			}
		}

		for (x=0; x<FB_HEIGHT; ++x) {
			puts("Filling C ");
			puti(x);
			putchar('\n');
			
			uint8_t lum = (offset<<2)+x;
			for (y=0; y<FB_WIDTH/2; ++y) {
				*fb++ = lum--;
			}
			fb += FB_WIDTH/2;
		}
		soclib_io_set( base(DMA), DMA_DST, FB_BASE );
		soclib_io_set( base(DMA), DMA_SRC, _fb );
		soclib_io_set( base(DMA), DMA_LEN, FB_SIZE );
		++offset;
	}
	return 0;
}
