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
     IF = 258,
     THEN = 259,
     ELSE = 260,
     ELIF = 261,
     FI = 262,
     CASE = 263,
     ESAC = 264,
     FOR = 265,
     SELECT = 266,
     WHILE = 267,
     UNTIL = 268,
     DO = 269,
     DONE = 270,
     FUNCTION = 271,
     COPROC = 272,
     COND_START = 273,
     COND_END = 274,
     COND_ERROR = 275,
     IN = 276,
     BANG = 277,
     TIME = 278,
     TIMEOPT = 279,
     WORD = 280,
     ASSIGNMENT_WORD = 281,
     NUMBER = 282,
     ARITH_CMD = 283,
     ARITH_FOR_EXPRS = 284,
     COND_CMD = 285,
     AND_AND = 286,
     OR_OR = 287,
     GREATER_GREATER = 288,
     LESS_LESS = 289,
     LESS_AND = 290,
     LESS_LESS_LESS = 291,
     GREATER_AND = 292,
     SEMI_SEMI = 293,
     SEMI_AND = 294,
     SEMI_SEMI_AND = 295,
     LESS_LESS_MINUS = 296,
     AND_GREATER = 297,
     AND_GREATER_GREATER = 298,
     LESS_GREATER = 299,
     GREATER_BAR = 300,
     BAR_AND = 301,
     yacc_EOF = 302
   };
#endif
/* Tokens.  */
#define IF 258
#define THEN 259
#define ELSE 260
#define ELIF 261
#define FI 262
#define CASE 263
#define ESAC 264
#define FOR 265
#define SELECT 266
#define WHILE 267
#define UNTIL 268
#define DO 269
#define DONE 270
#define FUNCTION 271
#define COPROC 272
#define COND_START 273
#define COND_END 274
#define COND_ERROR 275
#define IN 276
#define BANG 277
#define TIME 278
#define TIMEOPT 279
#define WORD 280
#define ASSIGNMENT_WORD 281
#define NUMBER 282
#define ARITH_CMD 283
#define ARITH_FOR_EXPRS 284
#define COND_CMD 285
#define AND_AND 286
#define OR_OR 287
#define GREATER_GREATER 288
#define LESS_LESS 289
#define LESS_AND 290
#define LESS_LESS_LESS 291
#define GREATER_AND 292
#define SEMI_SEMI 293
#define SEMI_AND 294
#define SEMI_SEMI_AND 295
#define LESS_LESS_MINUS 296
#define AND_GREATER 297
#define AND_GREATER_GREATER 298
#define LESS_GREATER 299
#define GREATER_BAR 300
#define BAR_AND 301
#define yacc_EOF 302




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 316 "/Users/chet/src/bash/src/parse.y"
{
  WORD_DESC *word;		/* the word that we read. */
  int number;			/* the number that we read. */
  WORD_LIST *word_list;
  COMMAND *command;
  REDIRECT *redirect;
  ELEMENT element;
  PATTERN_LIST *pattern;
}
/* Line 1489 of yacc.c.  */
#line 153 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

