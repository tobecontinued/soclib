#include "../segmentation.h"

#include "user.h"

void SwitchOnIt(int it)
{
char s[10];
int i,ti;
int *p=(int*)TIMER_BASE;

   /* Identify the active interrupt of highest priority */
   for (i=0; i<8; i++)
      if (it&(1<<i))
         break;

   switch (i) {
      case 0:   /* SwIt 0 */
         break;
      case 1:   /* SwIt 1 */
         break;
      case 2:   /* It 0   */
         uputs_adr(TTY_BASE | (procnum() << 0x4)  ,"timer interrupt\n");
	 ti=*(p+0);
	 itoa(ti,s);
	 uputs_adr(TTY_BASE | (procnum() << 0x4)  ,"Interrupt acknowledged at cycle :\n");
	 uputs_adr(TTY_BASE | (procnum() << 0x4)  ,s);
	 uputs_adr(TTY_BASE | (procnum() << 0x4)  ,"\n\n");
	 *(p+3+(procnum()<<2))=0;				// remise a zero de l'irq
         break;           
      case 3:   /* It 1   */
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
