%{
#include "globals.h"

static std::string topcell_quoted;

%}

%option nounput noyywrap prefix="topcell_"

%x quoted

QUOTE [\"']

%%

<quoted>{


{QUOTE}						{
							  topcell_lval.string = new std::string(topcell_quoted);
                              topcell_column += yyleng;
							  BEGIN(INITIAL);
							  return QNAME;
                            }

.							{
                              topcell_quoted += yytext;
                              topcell_column += yyleng;
                            }


}

<INITIAL>{
#[^\n]*

new							{ topcell_column += yyleng; return NEW; }
param						{ topcell_column += yyleng; return PARAM; }
conn						{ topcell_column += yyleng; return CONN; }
segment						{ topcell_column += yyleng; return SEGMENT; }
uncached					{ topcell_column += yyleng; return UNCACHED; }
cached						{ topcell_column += yyleng; return CACHED; }
to							{ topcell_column += yyleng; return TO; }
id							{ topcell_column += yyleng; return ID; }
"vci_param"					{ topcell_column += yyleng; return VCI_PARAM; }
"mapping table"				{ topcell_column += yyleng; return MAPPING_TABLE; }

{QUOTE}                     { topcell_column += yyleng; topcell_quoted = ""; BEGIN(quoted); }

0[xX][[:digit:]a-fA-F]+		{
                              topcell_lval.numeral = strtoll(yytext, 0, 16);
                              topcell_column += yyleng; return HEX;
                            }

[[:digit:]]+				{
                              topcell_lval.numeral = strtoll(yytext, 0, 0);
                              topcell_column += yyleng; return NUMERAL;
                            }

caba						{
                              topcell_lval.mode = soclib::common::MODE_CABA;
                              topcell_column += yyleng; return MODE;
                            }

tlmt						{
                              topcell_lval.mode = soclib::common::MODE_TLMT;
                              topcell_column += yyleng; return MODE;
                            }

[[:alpha:]][[:alnum:]_]*		{
                              topcell_lval.string = new std::string(yytext);
                              topcell_column += yyleng; return NAME;
                            }


[[:alpha:]][[:alnum:]_\[\]]*	{
                              topcell_lval.string = new std::string(yytext);
                              topcell_column += yyleng; return SIGNAME;
                            }
[ \t]+                   { topcell_column += yyleng; }
[\n]+                    { topcell_line += yyleng; topcell_column = 0; }
.                        { topcell_column += yyleng; return *yytext; }

}

%%

