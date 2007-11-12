PLATFORM_DESC=platform_desc
SOCLIB_CC=soclib-cc
SOCLIB:=$(shell soclib-cc --getpath)

export ARCH

ifeq ($(NO_SOFT),)
SOFT=soft/bin.soft
endif

default: test_soclib simulation.x $(SOFT)

ifeq ($(NO_SOFT),)

.PHONY: $(SOFT)

$(SOFT):
	$(MAKE) -C soft bin.soft

endif

test_soclib:
	@test -z "$(SOCLIB)" && (\
	echo "You must have soclib-cc in your $$PATH" ; exit 1 ) || exit 0
	@test ! -z "$(SOCLIB)"

simulation.x: $(PLATFORM_DESC)
	$(SOCLIB_CC) -p $(PLATFORM_DESC) -o $@

ifeq ($(origin SIMULATION_ARGS),undefined)
test:
	@echo "No arguments to simulation, cant simulate anything"

else
test: simulation.x $(SOFT)
	SOCLIB_TTY=TERM ./simulation.x $(SIMULATION_ARGS) < /dev/null

endif

clean: soft_clean
	$(SOCLIB_CC) -p $(PLATFORM_DESC) -x -o $@
	rm -rf repos

soft_clean:
ifeq ($(NO_SOFT),)
	$(MAKE) -C soft clean
endif

a.out:
	$(MAKE) -C soft

.PHONY: a.out simulation.x
