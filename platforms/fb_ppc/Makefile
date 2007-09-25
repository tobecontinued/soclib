PLATFORM_DESC=platform_desc
SOCLIB_CC=soclib-cc
SOCLIB:=$(shell soclib-cc --getpath)

default: test_soclib simulation.x a.out

test_soclib:
	@test -z "$(SOCLIB)" && (\
	echo "You must have soclib-cc in your $$PATH" ; exit 1 ) || exit 0
	@test ! -z "$(SOCLIB)"

simulation.x: $(PLATFORM_DESC)
	$(SOCLIB_CC) -p $(PLATFORM_DESC)

clean:
	$(SOCLIB_CC) -p $(PLATFORM_DESC) -x
	$(MAKE) -C fb_tester clean
	rm -rf repos

a.out:
	$(MAKE) -C fb_tester

.PHONY: a.out
