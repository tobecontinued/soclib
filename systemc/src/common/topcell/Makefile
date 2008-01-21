
PARSER_SOURCES=topcell_lexer.cc topcell_parser.cc
SOCLIB=$(shell soclib-cc --getpath)
HEADER_PLACE=$(SOCLIB)/systemc/include/common/topcell
PARSER_HEADERS=$(HEADER_PLACE)/topcell_lexer.hh \
 $(HEADER_PLACE)/topcell_parser.hh

src: $(PARSER_SOURCES) $(PARSER_HEADERS)

$(HEADER_PLACE)/topcell_lexer.hh: topcell_lexer.hh
	mv $< $@

$(HEADER_PLACE)/topcell_parser.hh: topcell_parser.hh
	mv $< $@

topcell_lexer.cc: topcell_parser.hh
topcell_parser.cc: topcell_lexer.hh

%_lexer.hh %_lexer.cc: %.l
	flex -o $*_lexer.cc --header-file=$*_lexer.hh $<

%_parser.cc %_parser.hh: %.yy
	bison -o $*_parser.cc -d $<

clean:
	rm -f $(OBJS) *_*.hh $(PARSER_SOURCES) $(PARSER_HEADERS)
