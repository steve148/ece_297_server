/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     HOSTTOK = 258,
     PORTTOK = 259,
     USERNAMETOK = 260,
     PASSWORDTOK = 261,
     TABLETOK = 262,
     DASH = 263,
     END_OF_FILE = 264,
     STORAGEPOLICYTOK = 265,
     DATADIRECTORYTOK = 266,
     INMEMORYTOK = 267,
     ONDISKTOK = 268,
     CONCURRENCYTOK = 269,
     COMMA = 270,
     COLON = 271,
     NEWLINE = 272,
     INTTOK = 273,
     CHARTOK = 274,
     CBRACKET = 275,
     STRING = 276,
     INTEGERTOK = 277,
     PASSWORD = 278,
     HOSTNAME = 279,
     DATA = 280
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 36 "parser.y"

char *stringVal;
char *passwordVal;
char *hostVal;
char *dataVal;
int intVal;



/* Line 2068 of yacc.c  */
#line 85 "parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


