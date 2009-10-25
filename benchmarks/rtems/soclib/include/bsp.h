/*  bsp.h
 *
 *  This include file contains some definitions specific to the
 *  SocLib simulator.
 *
 *  COPYRIGHT (c) 1989-2000.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id: bsp.h,v 1.17 2007/12/11 15:48:53 joel Exp $
 *
 *  Modified by Alexandre Becoulet for the SoCLib BSP
 */

#ifndef _BSP_H
#define _BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bspopts.h>

#include <rtems.h>
#include <rtems/iosupp.h>
#include <rtems/console.h>
#include <rtems/clockdrv.h>
#include <rtems/score/cpu.h>

/* Constants */

#define SOCLIB_ICU_BASE		0xb0c00200

#define SOCLIB_CLOCK_BASE	0xa0c00100
#define SOCLIB_TIMER_BASE	( SOCLIB_CLOCK_BASE + 16 )
#define CLOCK_VECTOR		0

#define TTY_SOCLIB_BASE		0x90c00000
#define TTY_VECTOR		2

/* functions */

void bsp_cleanup( void );

rtems_isr_entry set_vector(
  rtems_isr_entry, rtems_vector_number, int );

extern inline unsigned ld_le32(volatile uint32_t *addr)
{
#if __MIPSEB__
  return CPU_swap_u32(*addr);
#else
  return *addr;
#endif
}

static inline void st_le32(volatile uint32_t *addr, unsigned val)
{
#if __MIPSEB__
  *addr = CPU_swap_u32(val);
#else
  *addr = val;
#endif
}

#ifdef __cplusplus
}
#endif

#endif
