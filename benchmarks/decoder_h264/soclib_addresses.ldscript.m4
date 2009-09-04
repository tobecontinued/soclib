#/*****************************************************************************
#  Filename : soclib_addresses.ldscript.m4 
#
#  Authors:
#  Fabien Colas-Bigey THALES COM - AAL, 2009
#  Pierre-Edouard BEAUCAMPS, THALES COM - AAL, 2009
#
#  Copyright (C) THALES COMMUNICATIONS
# 
#  This code is free software: you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation, either version 3 of the License, or (at your
#  option) any later version.
#   
#  This code is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
#  for more details.
#   
#  You should have received a copy of the GNU General Public License along
#  with this code (see file COPYING).  If not, see
#  <http://www.gnu.org/licenses/>.
#  
#  This License does not grant permission to use the name of the copyright
#  owner, except only as required above for reproducing the content of
#  the copyright notice.
#*****************************************************************************/
dsx_segment_icu_start = 0x30200000;
dsx_segment_icu_end = 0x30200020;
dsx_segment_tty_start = 0x70200000;
dsx_segment_tty_end = 0x70200010;
dsx_segment_fb_start = 0x10200000;
dsx_segment_fb_end = 0x10400000;
dsx_segment_ramdisk_start = 0x68200000;
dsx_segment_ramdisk_end = 0x68300000;

dsx_segment_text_start = 0x9f000000;
dsx_segment_text_end = 0x9f100000;
dsx_segment_data_cached_start = 0x8f000000;
dsx_segment_data_cached_end = 0x8f100000;
dsx_segment_data_uncached_start = 0x62400000;
dsx_segment_data_uncached_end = 0x66400000;

m4_ifelse(CONFIG_CPU_MIPS, defined, `
dsx_segment_reset_start = 0xbfc00000;
dsx_segment_reset_end =   0xbfc00080;
')

m4_ifelse(CONFIG_CPU_PPC, defined, `
dsx_segment_reset_start = 0xffffff80;
dsx_segment_reset_end =   0x100000000;
')

m4_ifelse(CONFIG_CPU_ARM, defined, `
dsx_segment_reset_start = 0x000000000;
dsx_segment_reset_end =   0x000001000;
')

dsx_segment_excep_start = 0x80000000;
dsx_segment_excep_end = 0x80020000;
dsx_segment_sem_start = 0x40200000;
dsx_segment_sem_end = 0x40300000;
