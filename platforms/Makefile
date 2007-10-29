subdirs=$(patsubst %/platform_desc,%,$(shell echo */platform_desc))

default:

%:
	for i in $(subdirs) ; do \
		make -C "$$i" $@ ; \
	done
