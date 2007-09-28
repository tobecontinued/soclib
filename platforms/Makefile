subdirs=fb fb_ppc pi timer_4mips timer_4ppc

default:

%:
	for i in $(subdirs) ; do \
		make -C "$$i" $@ ; \
	done
