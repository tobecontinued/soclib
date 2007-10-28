#include "soclib/dma.h"
#include "system.h"

#include "../segmentation.h"

int main(void)
{
	uint8_t offset = 0;
	char _fb[FB_SIZE];

	uputs("Hello from processor ");
	putc(procnum()+'0');
	putc('\n');
	
	while(1) {
		uint32_t x, y;
		char *fb = _fb;

		for (x=0; x<FB_HEIGHT; ++x) {
			uputs("Filling Y ");
			puti(x);
			putc('\n');
			
			uint8_t lum = (offset<<7)+x;
			for (y=0; y<FB_WIDTH; ++y) {
				*fb++ = lum++;
			}
		}

		for (x=0; x<FB_HEIGHT; ++x) {
			uputs("Filling C ");
			puti(x);
			putc('\n');
			
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
