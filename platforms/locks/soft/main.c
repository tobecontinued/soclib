#include "soclib/timer.h"
#include "system.h"

#include "../segmentation.h"

extern uint32_t lock;

int main(void)
{
	const int cpu = procnum();
	int i;

	printf("Cpu %x booted\n", cpu);

	for (i=0; i<100; ++i) {
		lock_lock(&lock);
		printf("Hello from cpu %x\n", cpu);
		lock_unlock(&lock);
	}
	if ( cpu ) {
		while(1);
	} else {
		for (i=0; i<1000; ++i)
			;
		exit(0);
	}
}
