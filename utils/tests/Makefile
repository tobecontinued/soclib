ALL_TESTS=$(wildcard *.test)
ALL_OBJS=$(ALL_TESTS:.test=.o)
und=$(subst :,_,$(1))
get=$($(call und,$(1))_$(2))

all_no_xfail:

all: all_with_xfail

define test_decl

XFAIL:=
ARGS:=
MODULE:=

include $(1).test

$(2)_MODULE:=$$(MODULE)
$(2)_ARGS:=$$(foreach a,$$(ARGS), -a $$(a))
$(2)_XFAIL:=$$(XFAIL)
$(1)_XFAIL:=$$(XFAIL)
$(2)_DASH_IF_XFAIL:=$$(if $$(XFAIL),-,)

$(2).o: $(subst :,\:,$(1)).test

endef

$(eval $(foreach mkf,$(ALL_TESTS:.test=),$(call test_decl,$(mkf),$(call und,$(mkf)))))

clean:
	rm -f $(ALL_OBJS)

%.info: %.test
	@echo '$<'
	@echo 'MODULE: $(call get,$(*),MODULE)'
	@echo 'ARGS: $(call get,$(*),ARGS)'
	@echo 'XFAIL: $(call get,$(*),XFAIL)'
	@echo 'DASH_IF_XFAIL: $(call get,$(*),DASH_IF_XFAIL)'

%.o: %.test
	@soclib-cc $(SOCLIB_CC_ADD_ARGS) -o $@ -q -1 $(call get,$(*),MODULE) $(call get,$(*),ARGS)

all_no_xfail: $(foreach base,$(ALL_OBJS:.o=),$(if $(call get,$(base),XFAIL),,$(base).o))

all_with_xfail: $(ALL_OBJS)
