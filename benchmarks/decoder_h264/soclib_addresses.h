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

// Uncached segments
#define DSX_SEGMENT_ICU_ADDR		0x30200000
#define DSX_SEGMENT_ICU_SIZE		0x00000020
#define DSX_SEGMENT_TTY_ADDR		0x70200000
#define DSX_SEGMENT_TTY_SIZE		0x00000010
#define DSX_SEGMENT_TIMER_ADDR		0x11220000
#define DSX_SEGMENT_TIMER_SIZE		0x00000100
#define DSX_SEGMENT_SEM_ADDR		0x40200000
#define DSX_SEGMENT_SEM_SIZE		0x00100000
#define DSX_SEGMENT_FB_ADDR		0x10200000
#define DSX_SEGMENT_FB_SIZE		0x00200000
#define DSX_SEGMENT_DMA_ADDR		0x13200000
#define DSX_SEGMENT_DMA_SIZE		0x00000020
#define DSX_SEGMENT_RAMDISK_ADDR	0x68200000
#define DSX_SEGMENT_RAMDISK_SIZE	0x00100000
#define DSX_SEGMENT_DATA_UNCACHED_ADDR	0x62400000
#define DSX_SEGMENT_DATA_UNCACHED_SIZE	0x04000000


// Cached segments
#define DSX_SEGMENT_TEXT_ADDR		0x9f000000
#define DSX_SEGMENT_TEXT_SIZE		0x00100000
#define DSX_SEGMENT_EXCEP_ADDR		0x80000000
#define DSX_SEGMENT_EXCEP_SIZE		0x00001000
#define DSX_SEGMENT_DATA_CACHED_ADDR	0x8f000000
#define DSX_SEGMENT_DATA_CACHED_SIZE	0x00100000

#ifdef CONFIG_CPU_MIPS
	#define DSX_SEGMENT_RESET_ADDR	0xbfc00000
	#define DSX_SEGMENT_RESET_SIZE	0x00000080
#endif

#ifdef CONFIG_CPU_ARM
	#define DSX_SEGMENT_RESET_ADDR	0x00000000
	#define DSX_SEGMENT_RESET_SIZE	0x00001000
#endif

#ifdef CONFIG_CPU_PPC
	#define DSX_SEGMENT_RESET_ADDR	0xffffff80
	#define DSX_SEGMENT_RESET_SIZE	0x00000080
#endif


// Virtual Memory
#define	LOCAL_DATA0_BASE		0x20000000
#define	LOCAL_DATA0_SIZE		0x00001000 /* 4 096 */
#define	LOCAL_DATA1_BASE		0x20010000
#define	LOCAL_DATA1_SIZE		0x00001000 /* 4 096 */
#define	LOCAL_DATA2_BASE		0x20020000
#define	LOCAL_DATA2_SIZE		0x00001000 /* 4 096 */
#define	LOCAL_DATA3_BASE		0x20030000
#define	LOCAL_DATA3_SIZE		0x00001000 /* 4 096 */


// Page Table
#define PTD_ADDR			0x50200000
#define PTE_ADDR			0x50202000	// tty
#define IPTE_ADDR			0x50203000  	// instruction
#define TPTE_ADDR			0x50201000	// timer
#define	TAB_SIZE			0x00010000	/* 65 536 */
