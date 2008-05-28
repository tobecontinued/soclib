
ARCH=mipsel
export ARCH

all: soft/bin.soft

soft/bin.soft:
	$(MAKE) -C soft bin.soft

.PHONY: soft/bin.soft

soft_clean:
	$(MAKE) -C soft clean

clean: soft_clean

test:
	./mwmr.netlist sim_max_cycles=100000
