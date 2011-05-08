
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
     tDIMENSION = 260,
     tBOOLEAN = 261,
     tSTRING = 262,
     tWORD = 263,
     tGSWORD = 264,
     CURRENT_PRINTER = 265,
     SHOW_ALL_PAPER_SIZES = 266,
     PRINTER = 267,
     DESTINATION = 268,
     SCALING = 269,
     ORIENTATION = 270,
     AUTOSIZE_ROLL_PAPER = 271,
     UNIT = 272,
     DRIVER = 273,
     LEFT = 274,
     TOP = 275,
     CUSTOM_PAGE_WIDTH = 276,
     CUSTOM_PAGE_HEIGHT = 277,
     OUTPUT_TYPE = 278,
     PRINTRC_HDR = 279,
     PARAMETER = 280,
     QUEUE_NAME = 281,
     OUTPUT_FILENAME = 282,
     EXTRA_PRINTER_OPTIONS = 283,
     CUSTOM_COMMAND = 284,
     COMMAND_TYPE = 285,
     GLOBAL_SETTINGS = 286,
     GLOBAL = 287,
     END_GLOBAL_SETTINGS = 288,
     pINT = 289,
     pSTRING_LIST = 290,
     pFILE = 291,
     pDOUBLE = 292,
     pDIMENSION = 293,
     pBOOLEAN = 294,
     pCURVE = 295
   };
#endif
/* Tokens.  */
#define tINT 258
#define tDOUBLE 259
#define tDIMENSION 260
#define tBOOLEAN 261
#define tSTRING 262
#define tWORD 263
#define tGSWORD 264
#define CURRENT_PRINTER 265
#define SHOW_ALL_PAPER_SIZES 266
#define PRINTER 267
#define DESTINATION 268
#define SCALING 269
#define ORIENTATION 270
#define AUTOSIZE_ROLL_PAPER 271
#define UNIT 272
#define DRIVER 273
#define LEFT 274
#define TOP 275
#define CUSTOM_PAGE_WIDTH 276
#define CUSTOM_PAGE_HEIGHT 277
#define OUTPUT_TYPE 278
#define PRINTRC_HDR 279
#define PARAMETER 280
#define QUEUE_NAME 281
#define OUTPUT_FILENAME 282
#define EXTRA_PRINTER_OPTIONS 283
#define CUSTOM_COMMAND 284
#define COMMAND_TYPE 285
#define GLOBAL_SETTINGS 286
#define GLOBAL 287
#define END_GLOBAL_SETTINGS 288
#define pINT 289
#define pSTRING_LIST 290
#define pFILE 291
#define pDOUBLE 292
#define pDIMENSION 293
#define pBOOLEAN 294
#define pCURVE 295




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


