#include "soclib/timer.h"

#include "../segmentation.h"

#include "uputs.h"

int main(void)
{
   uputs(TTY_BASE, "Hello world!\n");
   while(1);
   return 0;
}

