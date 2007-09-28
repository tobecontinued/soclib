#include "soclib/timer.h"
#include "system.h"

#include "../segmentation.h"

static const int period[4] = {10000, 11000, 12000, 13000};

int main(void)
{
	const int cpu = procnum();

	uputs("Hello from processor ");
	putc(procnum()+'0');
	putc('\n');
	
	uputs("Setting period ");
	puti(period[cpu]);
	putc('\n');

	soclib_io_set(
		base(TIMER),
		procnum()*TIMER_SPAN+TIMER_PERIOD,
		period[cpu]);
	soclib_io_set(
		base(TIMER),
		procnum()*TIMER_SPAN+TIMER_MODE,
		TIMER_RUNNING|TIMER_IRQ_ENABLED);
	
//	*(int *)0 = 42;
	while (1);
	return 0;
}
