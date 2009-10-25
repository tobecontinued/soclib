/*
 *  This file implements a benchmark timer using a soclib timer.
 *
 *  NOTE: On the simulator, the count directly reflects instructions.
 *
 *  COPYRIGHT (c) 1989-2000.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id: timer.c,v 1.10 2008/09/05 04:59:23 ralf Exp $
 *
 *  Modified by Alexandre Becoulet for the SoCLib BSP
 */

#include <assert.h>
#include <bsp.h>
#include <soclib_timer.h>

bool benchmark_timer_find_average_overhead;

void benchmark_timer_initialize(void)
{
  SOCLIB_TIMER_WRITE( SOCLIB_TIMER_BASE, SOCLIB_TIMER_REG_MODE, SOCLIB_TIMER_REG_MODE_EN );
  SOCLIB_TIMER_WRITE( SOCLIB_TIMER_BASE, SOCLIB_TIMER_REG_VALUE, 0 );
}

int benchmark_timer_read(void)
{
  uint32_t          total = SOCLIB_TIMER_READ( SOCLIB_TIMER_BASE, SOCLIB_TIMER_REG_VALUE );

  return total;          /* in one microsecond units */
}

void benchmark_timer_disable_subtracting_average_overhead( bool find_flag )
{
}

