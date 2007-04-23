SUBDIRS=xtty fb_screen
BINS=xtty/soclib-xtty fb_screen/soclib-fb
DEST=../../bin

all:: default
	@true

install:: all
	cp $(BINS) $(DEST)

uninstall::
	for i in $(BINS) ; do \
		rm -f $(DEST)/$$(basename $$i) ; \
	done

%:
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i $@ ; \
	done
