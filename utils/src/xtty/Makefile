DESTDIR=../../bin
X11_DIR=/usr/X11R6

BIN=soclib-xtty

include common.mk

soclib-xtty:: xtty.o
soclib-xtty_LIBS=-L$(X11_DIR)/lib -lX11 -lXpm
soclib-xtty_old.o_CFLAGS=-I$(X11_DIR)/include
