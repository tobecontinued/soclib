TESTDIR?=.
include $(TESTDIR)/test.mk

RESULT=test.res
COMMON=$(TESTDIR)/../common
BASE=$(TESTDIR)/..
TESTER_DIR=$(shell soclib-cc --getpath)/platforms/tester
TESTER=$(TESTER_DIR)/simulation.x

ifeq ($(words $(ARCH)),1)

VPATH=$(TESTDIR) $(COMMON)

SOCLIB:=$(shell soclib-cc --getpath)

SOFT_IMAGE=bin.soft

include $(SOCLIB)/etc/soft_flags.mk

CC_PREFIX=$($(ARCH)_CC_PREFIX)
CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -O2 -I. -I$(COMMON) -I$(TESTER_DIR) $(shell soclib-cc --getflags=cflags) $(ADD_CFLAGS) $($(ARCH)_CFLAGS)

all: $(RESULT)

$(SOFT_IMAGE): $(OBJS) $(COMMON)/ldscript
	$(LD) -T $(COMMON)/ldscript $($(ARCH)_LDFLAGS) -o $@ $(OBJS)

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS)

$(RESULT): $(SOFT_IMAGE) $(TESTER) $(COMMON)/Makefile
	$(TESTER) $(ARCH) $(SOFT_IMAGE) > test.out \
		|| (r=$$? ; cat test.out ; mv test.out test.fail ; echo "Test failed in $$PWD, errorlevel: $$r" ; exit 1)
	cat test.out
	mv test.out $@

$(TESTER):
	make -C $(TESTER_DIR) $(patsubst $(TESTER_DIR)/%,%,$(TESTER))

.PHONY: $(TESTER)

else

TESTDIR=../

all: $(RESULT)

$(RESULT): $(patsubst %,%/$(RESULT),$(ARCH))
	cat $^ > $@

.PHONY: $(RESULT)

clean:
	rm -rf $(ARCH) $(RESULT)

define onetest

$(1)/$(RESULT): $(1)
	make -C $(1) -f $(COMMON)/Makefile TESTDIR=.. ARCH=$(1)

$(1):
	mkdir -p $(1)

.PHONY: $(1)/$(RESULT)

endef

$(foreach bla,$(ARCH),$(eval $(call onetest,$(bla))))

endif
