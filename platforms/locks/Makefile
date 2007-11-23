ARCH=mipsel
SIMULATION_ARGS=100000
include ../all.mk

post_test:
	for i in 0 1 2 3 ; do \
		count="$$(grep -c 'Hello from cpu '$$i $(TEST_OUTPUT))" ; \
		test $${count} = 100 || ( \
			echo "Got $${count} greetings from cpu $${i}, expected 100" ; \
			exit 1 ) ; \
	done
