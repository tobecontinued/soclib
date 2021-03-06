/*****************************************************************************
  Filename : soclib_addresses.h

  Authors:
  Fabien Colas-Bigey THALES COM - AAL, 2009
  Pierre-Edouard BEAUCAMPS, THALES COM - AAL, 2009

  Copyright (C) THALES COMMUNICATIONS
 
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
#ifndef __SOCLIB_ADDRESSES__
#define __SOCLIB_ADDRESSES__

#define CACHABILITY_MASK 0xf0000000

// CPU Specific segments
#define SEG_BOOT_ADDR	0xbfc00000
#define SEG_BOOT_SIZE	0x00001000

#define SEG_ROM_ADDR    0x50000000
#define SEG_ROM_SIZE    0x00100000

// Uncached segments
#define SEG_TTY_ADDR	0xd0200000
#define SEG_TTY_SIZE	0x00000040

#define SEG_TEXT_ADDR	0x7f400000
#define SEG_TEXT_SIZE	0x00100000

#define SEG_ICU_ADDR	0xd2200000
#define SEG_ICU_SIZE	0x00001000

#define SEG_DISK_ADDR   0x68200000
#define SEG_DISK_SIZE   0x02000000

#define SEG_BUFF_ADDR   0x10200000
#define SEG_BUFF_SIZE   0x00200000

#define SEG_TIMER_ADDR	0x11220000
#define SEG_TIMER_SIZE	0x00000100

#define	SEG_SIMHELPER_ADDR	0xd4200000
#define	SEG_SIMHELPER_SIZE	0x00000010

#endif
