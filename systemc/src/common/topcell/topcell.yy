%{
#include "globals.h"
#include "common/topcell/topcell_lexer.hh"
#include "caba/interface/vci_param.h"

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

%token <string> SIGNAME NAME QNAME
%destructor { delete $$; } SIGNAME NAME QNAME

%token <numeral> NUMERAL HEX
%token <mode> MODE

%type <cacheable> cacheability

%type  <string> anyname portname
%destructor { delete $$; } anyname portname

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

base : vci_params maptab module_inst_list ;

maptab : MAPPING_TABLE int_tab int_tab HEX
	    {
		 soclib::common::inst::InstArg &env = topcell_parser_retval->env();
		 env.add( "mapping_table",
           soclib::common::MappingTable(
             env.get<int>("addr_size"), *$2, *$3, $4 ) );
		 delete $2;
		 delete $3;
		}
	   ;

vci_params : VCI_PARAM '<' NUMERAL ',' NUMERAL ','
  NUMERAL ',' NUMERAL ',' NUMERAL ',' NUMERAL ','
  NUMERAL ',' NUMERAL ',' NUMERAL ',' NUMERAL '>'
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

module_inst : NEW MODE NAME QNAME spec_list
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

parameter   : NAME '=' NUMERAL { $$ = new soclib::common::topcell::Param<int>(*$1, $3); delete $1; }
            | NAME '=' array   { $$ = new soclib::common::topcell::Param<std::vector<std::string> >(*$1, *$3); delete $3; delete $1; }
            | NAME '=' int_tab { $$ = new soclib::common::topcell::Param<soclib::common::IntTab>(*$1, *$3); delete $3; delete $1; }
            | NAME '=' anyname { $$ = new soclib::common::topcell::Param<std::string>(*$1, *$3); delete $3; delete $1; }
            ;

segdecl : QNAME HEX HEX cacheability { $$ = new soclib::common::topcell::Segment( *$1, $2, $3, $4 ); delete $1; }
        ;

cacheability : CACHED { $$ = true; }
             | UNCACHED { $$ = false; }
             ;

int_tab     : '(' numerals ')' { $2->push_back(-1); $$ = new soclib::common::IntTab(*$2); delete $2; }
            ;

numerals    : NUMERAL					{ $$ = new std::vector<int>(); $$->push_back($1); }
            | numerals ',' NUMERAL		{ $1->push_back($3); $$=$1; }
            ;

array       : '[' name_list ']'			{ $$ = $2; }
            ;

name_list    : anyname                     { $$ = new std::vector<std::string>(); $$->push_back(*$1); delete $1; }
             | name_list ',' anyname       { $1->push_back(*$3); $$=$1; delete $3; }
             ;

connection  : portname TO sigspec { $$ = new soclib::common::topcell::Conn(*$1, (*$3)[0], (*$3)[1]); delete $1; delete $3; }
            ;

anyname : SIGNAME { $$ = $1; }
        | NAME    { $$ = $1; }
        | QNAME    { $$ = $1; }
        ;

portname : anyname  { $$ = $1; }
         ;

sigspec     : NAME '.' portname { $$ = new std::vector<std::string>(); $$->push_back(*$1); $$->push_back(*$3); delete $1; delete $3; }
            ;

%%
