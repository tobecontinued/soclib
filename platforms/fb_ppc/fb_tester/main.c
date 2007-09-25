#include "soclib/timer.h"
#include "system.h"

#include "../segmentation.h"

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
