%{
/*
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#include "globals.h"

static std::string topcell_quoted;

%}

%option nounput noyywrap prefix="topcell_"

%x quoted incl

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

<incl>[ \t]*      /* eat the whitespace */
<incl>[^ \t\n]+   { /* got the include file name */
        yyin = fopen( yytext, "r" );

        if ( ! yyin )
            throw soclib::exception::RunTimeError(std::string("Cant find file `")+yytext+"'");

        yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));

        BEGIN(INITIAL);
        }

<<EOF>> {
           yypop_buffer_state();

           if ( !YY_CURRENT_BUFFER )
               yyterminate();
        }

<INITIAL>{
#[^\n]*

include             BEGIN(incl);
set							{ topcell_column += yyleng; return SET; }
default						{ topcell_column += yyleng; return DEFAULT; }
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

\$[[:alpha:]][[:alnum:]_\[\]]*	{
                              topcell_lval.string = new std::string(yytext+1);
                              topcell_column += yyleng; return VAR;
                            }

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

