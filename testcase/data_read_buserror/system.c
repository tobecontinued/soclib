#include "system.h"

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	printf("OK, got exception\n");
	exit(0);
}

void interrupt_sys_handler(unsigned int irq)
{
	exit(1);
}

void interrupt_hw_handler(unsigned int irq)
{
	exit(1);
}
