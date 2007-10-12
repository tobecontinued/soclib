PLATFORM_DESC=platform_desc
SOCLIB_CC=soclib-cc
SOCLIB:=$(shell soclib-cc --getpath)

export ARCH

ifeq ($(NO_SOFT),)
SOFT=a.out
endif

default: test_soclib simulation.x $(SOFT)

test_soclib:
	@test -z "$(SOCLIB)" && (\
	echo "You must have soclib-cc in your $$PATH" ; exit 1 ) || exit 0
	@test ! -z "$(SOCLIB)"

simulation.x: $(PLATFORM_DESC)
	$(SOCLIB_CC) -p $(PLATFORM_DESC) -o $@

clean:
	$(SOCLIB_CC) -p $(PLATFORM_DESC) -x -o $@
ifeq ($(NO_SOFT),)
	$(MAKE) -C soft clean
endif
	rm -rf repos

a.out:
	$(MAKE) -C soft

.PHONY: a.out simulation.x
