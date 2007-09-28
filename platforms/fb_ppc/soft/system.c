#include "system.h"
#include "soclib/timer.h"

void uputs(const char *str)
{
	while (*str)
		putc(*str++);
}

void puti(const int i)
{
/* 	if ( i>9 ) */
/* 		puti(i/10); */
/* 	putc(i%10+'0'); */
	int r = i&0xf;

	if ( i>0xf )
		puti(i>>4);
	if ( r < 10 )
		putc(r+'0');
	else
		putc(r+'a'-10);
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
	while(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	uputs(__FUNCTION__);
	putc('\n');
}

void interrupt_hw_handler(unsigned int irq)
{
	int i,ti;

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
		soclib_io_set(
			base(TIMER),
			procnum()*TIMER_SPAN+TIMER_RESETIRQ,
			0);

		uputs("Intr on cpu ");
		puti(procnum());
		uputs(" at ");
		ti = soclib_io_get(
			base(TIMER),
			procnum()*TIMER_SPAN+TIMER_VALUE);
//		puti(get_cp0(9));
		puti(ti);
		putc('\n');
		putc('\n');
		break;           
	case 3:   /* It 1   */
		uputs("Got key ");
		puti(getc());
		putc('\n');
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
