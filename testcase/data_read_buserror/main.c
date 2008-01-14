#include "system.h"
#include "stdio.h"

#include "soclib/simhelper.h"

#include "segmentation.h"

void main(void)
{
	int i;
	for (i=0; i<10000; ++i)
		__asm__ __volatile__("nop");

	uint32_t *bla = 0xdeadbee0;
	*bla = 0x2a;

	for (i=0; i<10000; ++i)
		__asm__ __volatile__("nop");
	exit(1);
}
