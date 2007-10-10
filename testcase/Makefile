SOCLIB:=$(shell soclib-cc --getpath)

TESTER_DIR=$(SOCLIB)/platforms/tester
TESTER=$(TESTER_DIR)/system.x

TESTS=$(shell echo */test.mk)

export TESTER_DIR TESTER

all: tests

$(TESTER):
	make -C $(TESTER_DIR)

clean:
	rm -f $(TESTS:.mk=.res)
	for d in $(patsubst %/test.mk,%,$(TESTS)) ; do \
		$(MAKE) -C $$d -f ../common/Makefile clean || exit 1 ; \
	done

tests: $(TESTER) $(TESTS:.mk=.res)

%/test.res:
	make -C `dirname $@` -f ../common/Makefile test.res

.PHONY: $(TESTER)
