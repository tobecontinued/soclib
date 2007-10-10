#include "soclib/simhelper.h"
#include "system.h"

void uputs(const char *str)
{
	while (*str)
		putc(*str++);
}

void puti(const int i)
{
	if ( i>10 )
		puti(i/10);
	putc(i%10+'0');
}

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	uputs(__FUNCTION__);
	putc(' ');
	puti(type);
	putc('\n');

	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_END_WITH_RETVAL,
		2);

	while(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	uputs(__FUNCTION__);
	putc('\n');

	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_END_WITH_RETVAL,
		0);

	while(1);
}

void interrupt_hw_handler(unsigned int irq)
{
	int i;

	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_END_WITH_RETVAL,
		3);

/* 	uputs(__FUNCTION__); */
/* 	puti(irq); */
/* 	putc('\n'); */

	for (i=0; i<8;++i) {
		if (irq&1)
			break;
		irq>>=1;
	}

	switch (i) {
	case 0:   /* SwIt 0 */
		break;
	case 1:   /* SwIt 1 */
		break;
	case 2:   /* It 0   */
		break;           
	case 4:   /* It 2   */
		break;           
	case 5:   /* It 3   */
		break;           
	case 6:   /* It 4   */
		break;           
	case 7:   /* It 5   */
		break;
	default:
		break;
	}

}
