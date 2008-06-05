/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEW = 258,
     CONN = 259,
     PARAM = 260,
     TO = 261,
     ID = 262,
     SEGMENT = 263,
     CACHED = 264,
     UNCACHED = 265,
     VCI_PARAM = 266,
     MAPPING_TABLE = 267,
     SET = 268,
     DEFAULT = 269,
     SIGNAME = 270,
     NAME = 271,
     QNAME = 272,
     VAR = 273,
     NUMERAL = 274,
     HEX = 275,
     MODE = 276
   };
#endif
/* Tokens.  */
#define NEW 258
#define CONN 259
#define PARAM 260
#define TO 261
#define ID 262
#define SEGMENT 263
#define CACHED 264
#define UNCACHED 265
#define VCI_PARAM 266
#define MAPPING_TABLE 267
#define SET 268
#define DEFAULT 269
#define SIGNAME 270
#define NAME 271
#define QNAME 272
#define VAR 273
#define NUMERAL 274
#define HEX 275
#define MODE 276




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 46 "topcell.yy"
{
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
/* Line 1489 of yacc.c.  */
#line 106 "topcell_parser.hpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE topcell_lval;

