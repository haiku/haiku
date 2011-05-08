
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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
     SIZE_MODE = 275,
     RELATIVE = 276,
     PT = 277,
     IN = 278,
     MM = 279,
     HSIZE = 280,
     VSIZE = 281,
     BLACKLINE = 282,
     NOSCALE = 283,
     PATTERN = 284,
     XPATTERN = 285,
     EXTENDED = 286,
     IMAGE = 287,
     GRID = 288,
     SEMI = 289,
     CHANNEL = 290,
     CMYK = 291,
     KCMY = 292,
     RGB = 293,
     CMY = 294,
     GRAY = 295,
     WHITE = 296,
     MODE = 297,
     PAGESIZE = 298,
     MESSAGE = 299,
     OUTPUT = 300,
     START_JOB = 301,
     END_JOB = 302,
     END = 303
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
#define SIZE_MODE 275
#define RELATIVE 276
#define PT 277
#define IN 278
#define MM 279
#define HSIZE 280
#define VSIZE 281
#define BLACKLINE 282
#define NOSCALE 283
#define PATTERN 284
#define XPATTERN 285
#define EXTENDED 286
#define IMAGE 287
#define GRID 288
#define SEMI 289
#define CHANNEL 290
#define CMYK 291
#define KCMY 292
#define RGB 293
#define CMY 294
#define GRAY 295
#define WHITE 296
#define MODE 297
#define PAGESIZE 298
#define MESSAGE 299
#define OUTPUT 300
#define START_JOB 301
#define END_JOB 302
#define END 303




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


