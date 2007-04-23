#include "soclib/timer.h"
#include "user.h"

#include "../segmentation.h"

void itoa(int v,char *s)
{
	int i;
	int t=10000000;
	int ratio;
	int cpt=0;

	for (i=0;i<8;i++)
		s[i]=0;

	while (t>0)
	{
		ratio=v/t;
		s[cpt++]=(char)ratio+'0';
		v=v-ratio*t;
		t=t/10;
	}
}

int main0(void)
{
   int *p=(int*)(TIMER_BASE + 0x0);

   uputs_adr(TTY_BASE | (procnum() << 4)  ,"Hello from processor 0 \n \n");
   p[TIMER_PERIOD] = 10000;
   p[TIMER_MODE] = TIMER_RUNNING|TIMER_IRQ_ENABLED;
   while (1);
   return 0;
}

int main1(void)
{
   int *p=(int*)(TIMER_BASE + 0x10);

   uputs_adr(TTY_BASE | (procnum() << 4) ,"Hello from processor 1 \n \n");
   p[TIMER_PERIOD] = 11000;
   p[TIMER_MODE] = TIMER_RUNNING|TIMER_IRQ_ENABLED;
   while (1);
   return 0;
}

int main2(void)
{
   int *p=(int*)(TIMER_BASE + 0x20);

   uputs_adr(TTY_BASE | (procnum() << 4) ,"Hello from processor 2 \n \n");
   p[TIMER_PERIOD] = 12000;
   p[TIMER_MODE] = TIMER_RUNNING|TIMER_IRQ_ENABLED;
   while (1);
   return 0;
}

int main3(void)
{
   int *p=(int*)(TIMER_BASE + 0x30);

   uputs_adr(TTY_BASE | (procnum() << 4)  ,"Hello from processor 3 \n \n");
   p[TIMER_PERIOD] = 13000;
   p[TIMER_MODE] = TIMER_RUNNING|TIMER_IRQ_ENABLED;
   while (1);
   return 0;
}

