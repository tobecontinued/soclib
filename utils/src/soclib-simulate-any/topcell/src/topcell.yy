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
#include "topcell_lexer.hh"
#include "vci_param.h"

int yyerror(char *msg)
{
    std::cout
        << "Error at " << topcell_line
        << ":" << topcell_column
        << " " << msg << std::endl;
    return 0;
}

%}

%name-prefix="topcell_"
%error-verbose

%union {
  bool cacheable;
  bool forced;
  uint32_t numeral;
  std::string *string;
  soclib::common::IntTab *int_tab;
  soclib::common::topcell::Spec *spec;
  soclib::common::topcell::Conn *connection;
  std::vector<int> *int_vector;
  std::vector<std::string> *name_list;
  soclib::common::inst::InstArg *spec_list;
  soclib::common::topcell::Segment *segment;
  enum soclib::common::inst_mode_e mode;
}

%token NEW CONN PARAM TO ID SEGMENT CACHED UNCACHED
%token VCI_PARAM
%token MAPPING_TABLE

%token <forced> SET DEFAULT
%type <forced> setter

%token <string> SIGNAME NAME QNAME VAR
%destructor { delete $$; } SIGNAME NAME QNAME VAR

%token <numeral> NUMERAL HEX
%token <mode> MODE

%type <cacheable> cacheability

%type <numeral> anynum numnovar

%type  <string> anyname portname namenovar
%destructor { delete $$; } anyname portname namenovar

%type  <int_tab> int_tab
%destructor { delete $$; } int_tab

%type  <int_vector> numerals
%destructor { delete $$; } numerals

%type  <spec> spec parameter
%destructor { delete $$; } spec parameter

%type  <segment> segdecl
%destructor { delete $$; } segdecl

%type  <connection> connection
%destructor { delete $$; } connection

%type  <name_list> name_list array sigspec
%destructor { delete $$; } name_list array sigspec

%type  <spec_list> spec_list
%destructor { delete $$; } spec_list

%start base

%%

base : settings vci_params maptab module_inst_list ;

settings : /* */
         | settings setting
         ;

setting : setter NAME '=' numnovar {
                           soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                           if ( $1 || !env.has(*$2) ) env.add(*$2, (int)$4); delete $2;
                          }
        | setter NAME '=' array   {
                           soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                           if ( $1 || !env.has(*$2) ) env.add(*$2, *$4); delete $4; delete $2;
                          }
        | setter NAME '=' int_tab {
                           soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                           if ( $1 || !env.has(*$2) ) env.add(*$2, *$4); delete $4; delete $2;
                          }
        | setter NAME '=' namenovar {
                           soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                           if ( $1 || !env.has(*$2) ) env.add(*$2, *$4); delete $4; delete $2;
                          }
        | setter NAME '=' VAR {
                           soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                           env.add(*$2, env.get(*$4)); delete $4; delete $2;
                          }
        ;

setter : SET { $$ = true; }
       | DEFAULT { $$ = false; }
       ;

maptab : MAPPING_TABLE int_tab int_tab anynum
        {
         soclib::common::inst::InstArg &env = topcell_parser_retval->env();
         env.add( "mapping_table",
           soclib::common::MappingTable(
             env.get<int>("addr_size"), *$2, *$3, $4 ) );
         delete $2;
         delete $3;
        }
       ;

vci_params : VCI_PARAM '<' anynum ',' anynum ','
  anynum ',' anynum ',' anynum ',' anynum ','
  anynum ',' anynum ',' anynum ',' anynum '>'
             {
               soclib::common::inst::InstArg &env = topcell_parser_retval->env();
               env.add( "vci_param", *new std::string(
                 soclib::caba::VciParamsString(
                 $3, $5, $7, $9, $11, $13, $15, $17, $19, $21 ) ) );
               env.add( "addr_size", *new int($7) );
               env.add( "srcid_size", *new int($15) );
             }
           ;

module_inst_list :
                 | module_inst_list module_inst
                 ;

module_inst : NEW MODE anyname QNAME spec_list
              {
               topcell_parser_retval->prepare($2, *$3, *$4, $5);
               delete $3;
               delete $4;
              }
            ;

spec_list :                { $$ = topcell_parser_retval->new_args(); }
          | spec_list spec { $2->setIn(*$1); $$=$1; delete $2; }
          ;

spec : PARAM parameter { $$ = $2; }
     | CONN connection { $$ = $2; }
     | ID int_tab      { $$ = new soclib::common::topcell::Id(*$2); delete $2; }
     | SEGMENT segdecl { $$ = $2; }
     ;

parameter   : NAME '=' numnovar { $$ = new soclib::common::topcell::Param<int>(*$1, $3); delete $1; }
            | NAME '=' array   { $$ = new soclib::common::topcell::Param<std::vector<std::string> >(*$1, *$3); delete $3; delete $1; }
            | NAME '=' int_tab { $$ = new soclib::common::topcell::Param<soclib::common::IntTab>(*$1, *$3); delete $3; delete $1; }
            | NAME '=' namenovar { $$ = new soclib::common::topcell::Param<std::string>(*$1, *$3); delete $3; delete $1; }
            | NAME '=' VAR {
                  soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                  soclib::common::inst::InstArgBaseItem &_item = env.get(*$3);
				  typedef soclib::common::inst::InstArgItem<int> int_item_t;
				  typedef soclib::common::inst::InstArgItem<std::string> str_item_t;
				  typedef soclib::common::inst::InstArgItem<std::vector<std::string> > str_vect_item_t;
				  typedef soclib::common::inst::InstArgItem<soclib::common::IntTab> inttab_item_t;
				  if ( int_item_t *item = dynamic_cast<int_item_t*>(&_item) )
				    $$ = new soclib::common::topcell::Param<int>(*$1, **item);
				  else if ( str_item_t *item = dynamic_cast<str_item_t*>(&_item) )
				    $$ = new soclib::common::topcell::Param<std::string>(*$1, **item);
				  else if ( str_vect_item_t *item = dynamic_cast<str_vect_item_t*>(&_item) )
				    $$ = new soclib::common::topcell::Param<std::vector<std::string> >(*$1, **item);
				  else if ( inttab_item_t *item = dynamic_cast<inttab_item_t*>(&_item) ) {
				    $$ = new soclib::common::topcell::Param<soclib::common::IntTab>(*$1, **item);
				  } else throw soclib::exception::RunTimeError(
				  	std::string("Trying to get an incompatible type when getting variable `")
					+(*$1)+"'");
                  delete $1;
                  }
            ;

segdecl : QNAME anynum anynum cacheability { $$ = new soclib::common::topcell::Segment( *$1, $2, $3, $4 ); delete $1; }
        ;

cacheability : CACHED { $$ = true; }
             | UNCACHED { $$ = false; }
             ;

int_tab     : '(' numerals ')' { $2->push_back(-1); $$ = new soclib::common::IntTab(*$2); delete $2; }
            ;

numerals    : anynum                    { $$ = new std::vector<int>(); $$->push_back($1); }
            | numerals ',' anynum       { $1->push_back($3); $$=$1; }
            ;

array       : '[' name_list ']'         { $$ = $2; }
            ;

name_list    : anyname                     { $$ = new std::vector<std::string>(); $$->push_back(*$1); delete $1; }
             | name_list ',' anyname       { $1->push_back(*$3); $$=$1; delete $3; }
             ;

connection  : portname TO sigspec { $$ = new soclib::common::topcell::Conn(*$1, (*$3)[0], (*$3)[1]); delete $1; delete $3; }
            ;

anyname : namenovar { $$ = $1; }
        | VAR     {
                   soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                   $$ = new std::string(env.get<std::string>(*$1));
                  }
        ;

anynum : numnovar { $$ = $1 }
       | VAR     {
                   soclib::common::inst::InstArg &env = topcell_parser_retval->env();
                   $$ = env.get<int>(*$1);
                 }

namenovar : SIGNAME { $$ = $1; }
        | NAME    { $$ = $1; }
        | QNAME   { $$ = $1; }
        ;

numnovar : NUMERAL { $$ = $1 }
       | HEX     { $$ = $1 }
	   ;

portname : anyname  { $$ = $1; }
         ;

sigspec     : NAME '.' portname { $$ = new std::vector<std::string>(); $$->push_back(*$1); $$->push_back(*$3); delete $1; delete $3; }
            ;

%%
