SOCLIB:=$(shell soclib-cc --getpath)

TESTER_DIR=$(SOCLIB)/platforms/tester
TESTER=$(TESTER_DIR)/system.x

TESTS=$(shell echo */test.mk)
TESTRES=$(TESTS:.mk=.res)

export TESTER_DIR TESTER

all: tester

tester:
	make -C $(TESTER_DIR)

%:
	@for d in $(patsubst %/test.mk,%,$(TESTS)) ; do \
		$(MAKE) -C $$d -f ../common/Makefile $@ || exit 1 ; \
	done
