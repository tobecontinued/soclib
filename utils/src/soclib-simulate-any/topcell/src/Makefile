
PARSER_SOURCES=topcell_lexer.cpp topcell_parser.cpp
HEADER_PLACE=../include
PARSER_HEADERS=$(HEADER_PLACE)/topcell_lexer.hh \
 $(HEADER_PLACE)/topcell_parser.hh

src: $(PARSER_SOURCES) $(PARSER_HEADERS)

$(HEADER_PLACE)/topcell_lexer.hh: topcell_lexer.hh
	mv $< $@

$(HEADER_PLACE)/topcell_parser.hh: topcell_parser.hh
	mv $< $@

topcell_lexer.cpp: topcell_parser.hh
topcell_parser.cpp: topcell_lexer.hh

%_lexer.hh %_lexer.cpp: %.l
	flex -o $*_lexer.cpp --header-file=$*_lexer.hh $<

%_parser.cpp %_parser.hh: %.yy
	bison -o $*_parser.cpp -d $<
	-mv $*_parser.hpp $*_parser.hh

clean:
	rm -f $(OBJS) *_*.hh $(PARSER_SOURCES) $(PARSER_HEADERS)
