 # 
 #
 # SOCLIB_LGPL_HEADER_BEGIN
 # 
 # This file is part of SoCLib, GNU LGPLv2.1.
 # 
 # SoCLib is free software; you can redistribute it and/or modify it
 # under the terms of the GNU Lesser General Public License as published
 # by the Free Software Foundation; version 2.1 of the License.
 # 
 # SoCLib is distributed in the hope that it will be useful, but
 # WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 # Lesser General Public License for more details.
 # 
 # You should have received a copy of the GNU Lesser General Public
 # License along with SoCLib; if not, write to the Free Software
 # Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 # 02110-1301 USA
 # 
 # SOCLIB_LGPL_HEADER_END
 # 
 # Copyright (C) IRISA/INRIA, 2007
 #         Francois Charot <charot@irisa.fr>
 #
 #  Makefile pour compiling an application targeting the TMS320C62 processor
 #



ARCH=c6x
SOCLIB?=$(shell soclib-cc --getpath)
export SOCLIB

# --- cross compiler ----------------------------------------------------------

CC      := cl6x
AS      := asm6x
LD      := cl6x
OBJDUMP := dis6x

# --- files -------------------------------------------------------------------

BIN  := bin.soft
SRCS := main.c system.c
OBJS := reset.obj main.obj system.obj

# -----------------------------------------------------------------------------

COMMON=$(SOCLIB)/soclib/platform/topcells/common
VPATH=. $(COMMON)

HW_HEADERS=$(SOCLIB)/utils/include

CFLAGS=  -I. -I$(HW_HEADERS) -I$(SOCLIB) -I$(C6X_C_DIR)/include

all: $(BIN)

# --- binary ------------------------------------------------------------------

$(BIN): $(OBJS)
#	$(CC)  -z -c -m mapfile $(C6X_C_DIR)/lib/lnk.cmd  -abs -o $@ $(filter %.obj,$^) -l $(C6X_C_DIR)/lib/rts6200.lib
	$(CC)  -z -c -m mapfile lnk.cmd  -o $@ $(filter %.obj,$^) -l $(C6X_C_DIR)/lib/rts6200.lib
	nm6x -g --defined-only $@ > bin.maps
	$(OBJDUMP)  -i -c $@ > aout.txt 


# --- global rules ------------------------------------------------------------

%.obj : %.c system.h 
	$(CC) -c -mv6200 -pl -ss $(CFLAGS) $< -o $@

reset.obj: reset.asm
	$(AS) $< $@ 

# --- clean -------------------------------------------------------------------

clean :
	-rm -f  *.obj *.s *.maps main.asm system.asm *.rl mapfile aout.txt

# -----------------------------------------------------------------------------
