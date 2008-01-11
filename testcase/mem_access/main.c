#include "system.h"
#include "stdio.h"

#include "soclib/simhelper.h"

#include "segmentation.h"

struct data_test {
	uint32_t i32;
	uint16_t i16[2];
	uint8_t i8[4];
};

volatile struct data_test uncached __attribute__((section (".unc")));
volatile struct data_test cached;

void test(volatile struct data_test* d)
{
	d->i32 = 0x01020304;
	d->i16[0] = 0x0506;
	d->i16[1] = 0x0708;
	d->i8[0] = 0x09;
	d->i8[1] = 0x0a;
	d->i8[2] = 0x0b;
	d->i8[3] = 0x0c;

	assert(d->i32 == 0x01020304);
	assert(d->i16[0] == 0x0506);
	assert(d->i16[1] == 0x0708);
	assert(d->i8[0] == 0x09);
	assert(d->i8[1] == 0x0a);
	assert(d->i8[2] == 0x0b);
	assert(d->i8[3] == 0x0c);
}

int main(void)
{
	test(&uncached);
	test(&cached);

	exit(0);
	while(1)
		;
}
