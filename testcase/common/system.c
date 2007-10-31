#include "system.h"

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	printf("%s %d\n", __FUNCTION__, type);
	exit(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	printf("%s %d\n", __FUNCTION__, irq);
	exit(1);
}

void interrupt_hw_handler(unsigned int irq)
{
	printf("%s %d\n", __FUNCTION__, irq);
	exit(1);
}
