/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 *
 * This file is part of SoCLib, GNU LGPLv2.1.
 *
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 *
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 *
 */

#include "soclib/timer.h"
#include "system.h"
#include "stdio.h"

#include "../segmentation.h"

static const int period[4] = {10000, 11000, 12000, 13000};

static int max_interrupts = 80;

void irq_handler(int irq)
{
  uint32_t ti;

  int left = atomic_add(&max_interrupts, -1);

  ti = soclib_io_get(
		     base(TIMER),
		     procnum()*TIMER_SPAN+TIMER_VALUE);
  printf("IRQ %d received at cycle %d on cpu %d %d interrupts to go\n\n", irq, ti, procnum(), left);
  soclib_io_set(
		base(TIMER),
		procnum()*TIMER_SPAN+TIMER_RESETIRQ,
		0);

  if ( !left )
    exit(0);

}

int main(void)
{
  const int cpu = procnum();

  printf("hello From Nios II processor %d\n", procnum());

  set_irq_handler(irq_handler);
  enable_hw_irq(0);
  irq_enable();

  soclib_io_set(
		base(TIMER),
		procnum()*TIMER_SPAN+TIMER_PERIOD,
		period[cpu]);
  soclib_io_set(
		base(TIMER),
		procnum()*TIMER_SPAN+TIMER_MODE,
		TIMER_RUNNING|TIMER_IRQ_ENABLED);

  while (1);
  pause();
  return 0;
}
