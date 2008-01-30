/*\
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MutekH; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Author: Frédéric Pétrot <Frederic.Petrot@imag.fr>
\*/

#include "stdint.h"
#include "soclib/timer.h"
#include "system.h"

static const int period[4] = {10000, 11000, 12000, 13000};

int main(void)
{
   const int cpu = procnum();

   uputs("Hello from processor ");
   puti(cpu);
   putc('\n');
   
#if 0
   soclib_io_set(
      base(TIMER),
      procnum()*TIMER_SPAN+TIMER_PERIOD,
      uint32_swap(period[cpu]));
   soclib_io_set(
      base(TIMER),
      procnum()*TIMER_SPAN+TIMER_MODE,
      uint32_swap(TIMER_RUNNING|TIMER_IRQ_ENABLED));
#else
   soclib_io_set(
      base(TIMER),
      procnum()*TIMER_SPAN+TIMER_PERIOD,
      period[cpu]);
   soclib_io_set(
      base(TIMER),
      procnum()*TIMER_SPAN+TIMER_MODE,
      TIMER_RUNNING|TIMER_IRQ_ENABLED);
#endif
   
   while (1)
      putc('.');
   return 0;
}


/*\
 * All processors share the same stack until here
\*/
void __start(void)
{
   extern unsigned int _edata, _end; /* Known 4 words alignment */
   unsigned int *p;
#if 0
   static volatile int wait = 1;
   /* Clear bss just in case some function requires it ! */
   if (procnum() == 0) {
      for (p = &_edata; p < &_end; p++)
         *p = 0;
      wait = 0;
   } else {
      while (wait) {
         puti(procnum());
         putc(' ');
      }
   }
#endif

   /* Setting a different stack per processor */
   set_r1(-procnum() * 1024);
   main();
   while (1);
}
