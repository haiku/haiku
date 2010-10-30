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
     tINT = 258,
     tDOUBLE = 259,
     tSTRING = 260,
     COLOR = 261,
     GAMMA = 262,
     LEVEL = 263,
     STEPS = 264,
     INK_LIMIT = 265,
     PRINTER = 266,
     PARAMETER = 267,
     PARAMETER_INT = 268,
     PARAMETER_BOOL = 269,
     PARAMETER_FLOAT = 270,
     PARAMETER_CURVE = 271,
     DENSITY = 272,
     TOP = 273,
     LEFT = 274,
     HSIZE = 275,
     VSIZE = 276,
     BLACKLINE = 277,
     NOSCALE = 278,
     PATTERN = 279,
     XPATTERN = 280,
     EXTENDED = 281,
     IMAGE = 282,
     GRID = 283,
     SEMI = 284,
     CHANNEL = 285,
     CMYK = 286,
     KCMY = 287,
     RGB = 288,
     CMY = 289,
     GRAY = 290,
     WHITE = 291,
     MODE = 292,
     PAGESIZE = 293,
     MESSAGE = 294,
     OUTPUT = 295,
     START_JOB = 296,
     END_JOB = 297,
     END = 298
   };
#endif
/* Tokens.  */
#define tINT 258
#define tDOUBLE 259
#define tSTRING 260
#define COLOR 261
#define GAMMA 262
#define LEVEL 263
#define STEPS 264
#define INK_LIMIT 265
#define PRINTER 266
#define PARAMETER 267
#define PARAMETER_INT 268
#define PARAMETER_BOOL 269
#define PARAMETER_FLOAT 270
#define PARAMETER_CURVE 271
#define DENSITY 272
#define TOP 273
#define LEFT 274
#define HSIZE 275
#define VSIZE 276
#define BLACKLINE 277
#define NOSCALE 278
#define PATTERN 279
#define XPATTERN 280
#define EXTENDED 281
#define IMAGE 282
#define GRID 283
#define SEMI 284
#define CHANNEL 285
#define CMYK 286
#define KCMY 287
#define RGB 288
#define CMY 289
#define GRAY 290
#define WHITE 291
#define MODE 292
#define PAGESIZE 293
#define MESSAGE 294
#define OUTPUT 295
#define START_JOB 296
#define END_JOB 297
#define END 298




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

