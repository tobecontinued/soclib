/*
 *  Instantiate the clock driver shell.
 *
 *  COPYRIGHT (c) 1989-2006.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id: clockdrv.c,v 1.8 2006/11/17 22:41:41 joel Exp $
 *
 *  Modified by Alexandre Becoulet for the SoCLib BSP
 */

#include <rtems.h>
#include <bsp.h>
#include <soclib_timer.h>



#define CLOCK_DRIVER_USE_FAST_IDLE

#define Clock_driver_support_at_tick() \
  SOCLIB_TIMER_WRITE( SOCLIB_CLOCK_BASE, SOCLIB_TIMER_REG_IRQ, 0 );

/*
 *  500000 clicks per tick ISR is HIGHLY arbitrary
 */

#define CLICKS 500000

#define Clock_driver_support_install_isr( _new, _old )			\
  do {									\
    uint32_t   _clicks = CLICKS;					\
    _old = set_vector( _new, CLOCK_VECTOR, 1 );				\
    SOCLIB_TIMER_WRITE( SOCLIB_CLOCK_BASE, SOCLIB_TIMER_REG_VALUE, 0 );	\
    SOCLIB_TIMER_WRITE( SOCLIB_CLOCK_BASE, SOCLIB_TIMER_REG_PERIOD, _clicks ); \
    SOCLIB_TIMER_WRITE( SOCLIB_CLOCK_BASE, SOCLIB_TIMER_REG_MODE,	\
			SOCLIB_TIMER_REG_MODE_IRQEN | SOCLIB_TIMER_REG_MODE_EN ); \
									\
  } while(0)

#define Clock_driver_support_initialize_hardware()

#define Clock_driver_support_shutdown_hardware() \
  SOCLIB_TIMER_WRITE( SOCLIB_CLOCK_BASE, SOCLIB_TIMER_REG_MODE, 0 );

#include "../../../shared/clockdrv_shell.c"

