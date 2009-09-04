/*****************************************************************************

  timer.c -- Function dealing with the timer 


Authors:
Pierre-Edouard BEAUCAMPS, THALES COM - AAL, 2009

Copyright (C) THALES & Martin Fiedler All rights reserved.

This code is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

This code is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this code (see file COPYING).  If not, see
<http://www.gnu.org/licenses/>.

This License does not grant permission to use the name of the copyright
owner, except only as required above for reproducing the content of
the copyright notice.
 *****************************************************************************/

#include "timer.h"

/****************************************************************************
  Global variables and structures
 ****************************************************************************/
extern struct device_s timer_dev;


/*************************************
  Return Timer Value
 *************************************/
uint32_t get_timer_value()
{
#if defined(CONFIG_DRIVER_TIMER_EMU)
	return (uint32_t) timer_emu_getvalue(&timer_dev, 0);
#elif defined(CONFIG_DRIVER_TIMER_SOCLIB)
	return (uint32_t) timer_soclib_getvalue(&timer_dev, 0);
#else
	return -1;
#endif
}

/*************************************
  Set Timer Value
 *************************************/
void set_timer_value(uint32_t value)
{
#if defined(CONFIG_DRIVER_TIMER_EMU)
	timer_emu_setvalue(&timer_dev, 0, value);
#elif defined(CONFIG_DRIVER_TIMER_SOCLIB)
	timer_soclib_setvalue(&timer_dev, 0, value);
#endif
}
