#include "system.h"
#include "stdio.h"

#include "soclib/simhelper.h"

#include "segmentation.h"

void main(void)
{
	int i;
	for (i=0; i<10000; ++i)
		__asm__ __volatile__("nop");
#if defined(__mips__)
	asm volatile("syscall\n\tnop");
#elif defined(__PPC__)
	asm volatile("sc");
#elif defined(__MICROBLAZE__)
    asm volatile("bralid r0, 0x8\n\tnop");
#else
#error Unsupported arch
#endif

	exit(1);
}
