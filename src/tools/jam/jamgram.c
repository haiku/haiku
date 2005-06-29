
/*  A Bison parser, made from jamgram.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	_BANG_t	257
#define	_BANG_EQUALS_t	258
#define	_AMPER_t	259
#define	_AMPERAMPER_t	260
#define	_LPAREN_t	261
#define	_RPAREN_t	262
#define	_PLUS_EQUALS_t	263
#define	_COLON_t	264
#define	_SEMIC_t	265
#define	_LANGLE_t	266
#define	_LANGLE_EQUALS_t	267
#define	_EQUALS_t	268
#define	_RANGLE_t	269
#define	_RANGLE_EQUALS_t	270
#define	_QUESTION_EQUALS_t	271
#define	_LBRACKET_t	272
#define	_RBRACKET_t	273
#define	ACTIONS_t	274
#define	BIND_t	275
#define	BREAK_t	276
#define	CASE_t	277
#define	CONTINUE_t	278
#define	DEFAULT_t	279
#define	ELSE_t	280
#define	EXISTING_t	281
#define	FOR_t	282
#define	IF_t	283
#define	IGNORE_t	284
#define	IN_t	285
#define	INCLUDE_t	286
#define	JUMPTOEOF_t	287
#define	LOCAL_t	288
#define	MAXLINE_t	289
#define	ON_t	290
#define	PIECEMEAL_t	291
#define	QUIETLY_t	292
#define	RETURN_t	293
#define	RULE_t	294
#define	SWITCH_t	295
#define	TOGETHER_t	296
#define	UPDATED_t	297
#define	WHILE_t	298
#define	_LBRACE_t	299
#define	_BAR_t	300
#define	_BARBAR_t	301
#define	_RBRACE_t	302
#define	ARG	303
#define	STRING	304

#line 85 "jamgram.y"

#include "jam.h"

#include "lists.h"
#include "variable.h"
#include "parse.h"
#include "scan.h"
#include "compile.h"
#include "newstr.h"
#include "rules.h"

# define YYMAXDEPTH 10000	/* for OSF and other less endowed yaccs */

# define F0 (LIST *(*)(PARSE *, LOL *, int *))0
# define P0 (PARSE *)0
# define S0 (char *)0

# define pappend( l,r )    	parse_make( compile_append,l,r,P0,S0,S0,0 )
# define pbreak( l,f )     	parse_make( compile_break,l,P0,P0,S0,S0,f )
# define peval( c,l,r )		parse_make( compile_eval,l,r,P0,S0,S0,c )
# define pfor( s,l,r )    	parse_make( compile_foreach,l,r,P0,s,S0,0 )
# define pif( l,r,t )	  	parse_make( compile_if,l,r,t,S0,S0,0 )
# define pincl( l )       	parse_make( compile_include,l,P0,P0,S0,S0,0 )
# define plist( s )	  	parse_make( compile_list,P0,P0,P0,s,S0,0 )
# define plocal( l,r,t )  	parse_make( compile_local,l,r,t,S0,S0,0 )
# define pnull()	  	parse_make( compile_null,P0,P0,P0,S0,S0,0 )
# define pon( l,r )	  	parse_make( compile_on,l,r,P0,S0,S0,0 )
# define prule( a,p )     	parse_make( compile_rule,a,p,P0,S0,S0,0 )
# define prules( l,r )	  	parse_make( compile_rules,l,r,P0,S0,S0,0 )
# define pset( l,r,a ) 	  	parse_make( compile_set,l,r,P0,S0,S0,a )
# define pset1( l,r,t,a )	parse_make( compile_settings,l,r,t,S0,S0,a )
# define psetc( s,l,r )     	parse_make( compile_setcomp,l,r,P0,s,S0,0 )
# define psete( s,l,s1,f ) 	parse_make( compile_setexec,l,P0,P0,s,s1,f )
# define pswitch( l,r )   	parse_make( compile_switch,l,r,P0,S0,S0,0 )
# define pwhile( l,r )   	parse_make( compile_while,l,r,P0,S0,S0,0 )

# define pnode( l,r )    	parse_make( F0,l,r,P0,S0,S0,0 )
# define psnode( s,l )     	parse_make( F0,l,P0,P0,s,S0,0 )

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#ifndef const
#define const
#endif
#endif
#endif



#define	YYFINAL		157
#define	YYFLAG		-32768
#define	YYNTBASE	51

#define YYTRANSLATE(x) ((unsigned)(x) <= 304 ? yytranslate[x] : 71)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     3,     4,     6,     8,    11,    16,    23,    27,
    31,    35,    39,    44,    51,    55,    59,    63,    71,    77,
    83,    91,    97,   104,   108,   109,   110,   120,   122,   124,
   126,   129,   131,   135,   139,   143,   147,   151,   155,   159,
   163,   167,   171,   175,   178,   182,   183,   186,   191,   192,
   196,   198,   200,   204,   206,   207,   210,   212,   213,   218,
   221,   226,   231,   232,   235,   237,   239,   241,   243,   245,
   247,   250,   251
};

static const short yyrhs[] = {    -1,
    53,     0,     0,    53,     0,    54,     0,    54,    53,     0,
    34,    63,    11,    52,     0,    34,    63,    14,    63,    11,
    52,     0,    45,    52,    48,     0,    32,    63,    11,     0,
    33,    63,    11,     0,    65,    62,    11,     0,    65,    57,
    63,    11,     0,    65,    36,    63,    57,    63,    11,     0,
    22,    63,    11,     0,    24,    63,    11,     0,    39,    63,
    11,     0,    28,    49,    31,    63,    45,    52,    48,     0,
    41,    63,    45,    59,    48,     0,    29,    58,    45,    52,
    48,     0,    29,    58,    45,    52,    48,    26,    54,     0,
    44,    58,    45,    52,    48,     0,    40,    49,    61,    45,
    52,    48,     0,    36,    65,    54,     0,     0,     0,    20,
    68,    49,    70,    45,    55,    50,    56,    48,     0,    14,
     0,     9,     0,    17,     0,    25,    14,     0,    65,     0,
    58,    14,    58,     0,    58,     4,    58,     0,    58,    12,
    58,     0,    58,    13,    58,     0,    58,    15,    58,     0,
    58,    16,    58,     0,    58,     5,    58,     0,    58,     6,
    58,     0,    58,    46,    58,     0,    58,    47,    58,     0,
    65,    31,    63,     0,     3,    58,     0,     7,    58,     8,
     0,     0,    60,    59,     0,    23,    49,    10,    52,     0,
     0,    49,    10,    61,     0,    49,     0,    63,     0,    63,
    10,    62,     0,    64,     0,     0,    64,    65,     0,    49,
     0,     0,    18,    66,    67,    19,     0,    65,    62,     0,
    36,    65,    65,    62,     0,    36,    65,    39,    63,     0,
     0,    68,    69,     0,    43,     0,    42,     0,    30,     0,
    38,     0,    37,     0,    27,     0,    35,    49,     0,     0,
    21,    63,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   128,   130,   141,   143,   147,   149,   151,   153,   157,   159,
   161,   163,   165,   167,   169,   171,   173,   175,   177,   179,
   181,   183,   185,   187,   189,   192,   194,   201,   203,   205,
   207,   215,   217,   219,   221,   223,   225,   227,   229,   231,
   233,   235,   237,   239,   241,   251,   253,   257,   266,   268,
   270,   279,   281,   291,   295,   297,   301,   303,   303,   312,
   314,   316,   325,   327,   331,   333,   335,   337,   339,   341,
   343,   352,   354
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","_BANG_t",
"_BANG_EQUALS_t","_AMPER_t","_AMPERAMPER_t","_LPAREN_t","_RPAREN_t","_PLUS_EQUALS_t",
"_COLON_t","_SEMIC_t","_LANGLE_t","_LANGLE_EQUALS_t","_EQUALS_t","_RANGLE_t",
"_RANGLE_EQUALS_t","_QUESTION_EQUALS_t","_LBRACKET_t","_RBRACKET_t","ACTIONS_t",
"BIND_t","BREAK_t","CASE_t","CONTINUE_t","DEFAULT_t","ELSE_t","EXISTING_t","FOR_t",
"IF_t","IGNORE_t","IN_t","INCLUDE_t","JUMPTOEOF_t","LOCAL_t","MAXLINE_t","ON_t",
"PIECEMEAL_t","QUIETLY_t","RETURN_t","RULE_t","SWITCH_t","TOGETHER_t","UPDATED_t",
"WHILE_t","_LBRACE_t","_BAR_t","_BARBAR_t","_RBRACE_t","ARG","STRING","run",
"block","rules","rule","@1","@2","assign","expr","cases","case","params","lol",
"list","listp","arg","@3","func","eflags","eflag","bindlist", NULL
};
#endif

static const short yyr1[] = {     0,
    51,    51,    52,    52,    53,    53,    53,    53,    54,    54,
    54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
    54,    54,    54,    54,    55,    56,    54,    57,    57,    57,
    57,    58,    58,    58,    58,    58,    58,    58,    58,    58,
    58,    58,    58,    58,    58,    59,    59,    60,    61,    61,
    61,    62,    62,    63,    64,    64,    65,    66,    65,    67,
    67,    67,    68,    68,    69,    69,    69,    69,    69,    69,
    69,    70,    70
};

static const short yyr2[] = {     0,
     0,     1,     0,     1,     1,     2,     4,     6,     3,     3,
     3,     3,     4,     6,     3,     3,     3,     7,     5,     5,
     7,     5,     6,     3,     0,     0,     9,     1,     1,     1,
     2,     1,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     2,     3,     0,     2,     4,     0,     3,
     1,     1,     3,     1,     0,     2,     1,     0,     4,     2,
     4,     4,     0,     2,     1,     1,     1,     1,     1,     1,
     2,     0,     2
};

static const short yydefact[] = {     1,
    58,    63,    55,    55,     0,     0,    55,    55,    55,     0,
    55,     0,    55,     0,     3,    57,     2,     5,    55,     0,
     0,     0,    54,     0,     0,     0,     0,     0,    32,     0,
     0,     0,     0,     0,    49,     0,     0,     0,     4,     6,
    29,    28,    30,     0,    55,    55,     0,    52,     0,    55,
     0,    70,    67,     0,    69,    68,    66,    65,    72,    64,
    15,    56,    16,    55,    44,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     3,     0,     0,    55,    10,    11,
     3,    55,    24,    17,    51,     0,    46,     3,     9,    31,
     0,     0,    12,    55,     0,    60,    59,    71,    55,     0,
     0,    45,    34,    39,    40,    35,    36,    33,    37,    38,
     0,    41,    42,    43,     7,     0,    49,     3,     0,     0,
    46,     0,    55,    13,    53,    55,    55,    73,    25,     3,
    20,     3,    50,     0,     0,    19,    47,    22,     0,    62,
    61,     0,     0,     0,     8,    23,     3,    14,    26,    18,
    21,    48,     0,    27,     0,     0,     0
};

static const short yydefgoto[] = {   155,
    38,    39,    18,   142,   153,    46,    28,   120,   121,    86,
    47,    48,    23,    29,    20,    51,    21,    60,   100
};

static const short yypact[] = {   141,
-32768,-32768,-32768,-32768,   -45,    14,-32768,-32768,-32768,     1,
-32768,   -37,-32768,    14,   141,-32768,-32768,   141,   103,     6,
   185,     2,     1,     3,    -5,    14,    14,    25,    -4,    36,
    42,    37,   169,    43,   -13,    15,   121,    11,-32768,-32768,
-32768,-32768,-32768,    47,-32768,-32768,    53,    56,     1,-32768,
    48,-32768,-32768,    19,-32768,-32768,-32768,-32768,    52,-32768,
-32768,-32768,-32768,-32768,-32768,   137,    14,    14,    14,    14,
    14,    14,    14,    14,   141,    14,    14,-32768,-32768,-32768,
   141,-32768,-32768,-32768,    59,    29,    55,   141,-32768,-32768,
   186,    78,-32768,-32768,     7,-32768,-32768,-32768,-32768,    45,
    46,-32768,    71,   142,   142,-32768,-32768,    71,-32768,-32768,
    34,   225,   225,-32768,-32768,    81,   -13,   141,    44,    58,
    55,    61,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   141,
    70,   141,-32768,    62,   101,-32768,-32768,-32768,   102,-32768,
-32768,    65,    68,   169,-32768,-32768,   141,-32768,-32768,-32768,
-32768,-32768,    74,-32768,   119,   123,-32768
};

static const short yypgoto[] = {-32768,
   -53,    16,   -30,-32768,-32768,    38,    31,    10,-32768,    21,
   -42,    -2,-32768,     0,-32768,-32768,-32768,-32768,-32768
};


#define	YYLAST		241


static const short yytable[] = {    19,
    22,    24,    83,    25,    30,    31,    32,    96,    34,    33,
    36,    35,    61,    63,    19,    17,    26,    19,     1,    50,
    27,   111,    62,     1,     1,    64,    78,   115,    67,    68,
    69,     1,    19,    40,   122,    85,    70,    71,    72,    73,
    74,    49,    91,    92,    37,   126,    79,    81,    95,    16,
    82,   125,    80,    84,    16,    16,    65,    66,    89,    87,
    90,   101,    16,    93,   134,    94,    97,    98,   117,    75,
    76,    77,    99,   118,    19,   114,   143,   119,   145,   116,
    19,   131,    70,    71,   141,    73,    74,    19,   124,   129,
   130,   132,   135,   152,   127,   144,   128,   103,   104,   105,
   106,   107,   108,   109,   110,   136,   112,   113,   138,   146,
   147,    41,   148,   151,   149,   150,    42,    19,   156,    43,
   139,   154,   157,   140,    67,    68,    69,    44,   123,    19,
   137,    19,    70,    71,    72,    73,    74,   133,    45,     0,
    67,    68,    69,    19,   102,    67,    19,     0,    70,    71,
    72,    73,    74,    70,    71,    72,    73,    74,     1,     0,
     2,     0,     3,     0,     4,    88,    76,    77,     5,     6,
     0,     0,     7,     8,     9,     0,    10,     0,     0,    11,
    12,    13,    76,    77,    14,    15,     1,     0,     2,    16,
     3,     0,     4,     0,    41,     0,     5,     6,     0,    42,
     7,     8,    43,     0,    10,     0,     0,    11,    12,    13,
    44,    52,    14,    15,    53,     0,     0,    16,     0,    54,
     0,    55,    56,     0,     0,     0,    57,    58,    67,    68,
    69,     0,     0,    59,     0,     0,    70,    71,    72,    73,
    74
};

static const short yycheck[] = {     0,
     3,     4,    33,    49,     7,     8,     9,    50,    11,    10,
    13,    49,    11,    11,    15,     0,     3,    18,    18,    20,
     7,    75,    23,    18,    18,    31,    31,    81,     4,     5,
     6,    18,    33,    18,    88,    49,    12,    13,    14,    15,
    16,    36,    45,    46,    14,    39,    11,    11,    49,    49,
    14,    94,    11,    11,    49,    49,    26,    27,    48,    45,
    14,    64,    49,    11,   118,    10,    19,    49,    10,    45,
    46,    47,    21,    45,    75,    78,   130,    23,   132,    82,
    81,    48,    12,    13,   127,    15,    16,    88,    11,    45,
    45,    11,    49,   147,    95,    26,    99,    67,    68,    69,
    70,    71,    72,    73,    74,    48,    76,    77,    48,    48,
    10,     9,    11,   144,    50,    48,    14,   118,     0,    17,
   123,    48,     0,   126,     4,     5,     6,    25,    91,   130,
   121,   132,    12,    13,    14,    15,    16,   117,    36,    -1,
     4,     5,     6,   144,     8,     4,   147,    -1,    12,    13,
    14,    15,    16,    12,    13,    14,    15,    16,    18,    -1,
    20,    -1,    22,    -1,    24,    45,    46,    47,    28,    29,
    -1,    -1,    32,    33,    34,    -1,    36,    -1,    -1,    39,
    40,    41,    46,    47,    44,    45,    18,    -1,    20,    49,
    22,    -1,    24,    -1,     9,    -1,    28,    29,    -1,    14,
    32,    33,    17,    -1,    36,    -1,    -1,    39,    40,    41,
    25,    27,    44,    45,    30,    -1,    -1,    49,    -1,    35,
    -1,    37,    38,    -1,    -1,    -1,    42,    43,     4,     5,
     6,    -1,    -1,    49,    -1,    -1,    12,    13,    14,    15,
    16
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/bison.simple"
/* This file comes from bison-.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386)) || (defined (__BEOS__) && defined (__MWERKS__))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
#ifndef YYSTACK_USE_ALLOCA
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 2:
#line 131 "jamgram.y"
{ parse_save( yyvsp[0].parse ); ;
    break;}
case 3:
#line 142 "jamgram.y"
{ yyval.parse = pnull(); ;
    break;}
case 4:
#line 144 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; ;
    break;}
case 5:
#line 148 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; ;
    break;}
case 6:
#line 150 "jamgram.y"
{ yyval.parse = prules( yyvsp[-1].parse, yyvsp[0].parse ); ;
    break;}
case 7:
#line 152 "jamgram.y"
{ yyval.parse = plocal( yyvsp[-2].parse, pnull(), yyvsp[0].parse ); ;
    break;}
case 8:
#line 154 "jamgram.y"
{ yyval.parse = plocal( yyvsp[-4].parse, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 9:
#line 158 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; ;
    break;}
case 10:
#line 160 "jamgram.y"
{ yyval.parse = pincl( yyvsp[-1].parse ); ;
    break;}
case 11:
#line 162 "jamgram.y"
{ yyval.parse = pbreak( yyvsp[-1].parse, JMP_EOF ); ;
    break;}
case 12:
#line 164 "jamgram.y"
{ yyval.parse = prule( yyvsp[-2].parse, yyvsp[-1].parse ); ;
    break;}
case 13:
#line 166 "jamgram.y"
{ yyval.parse = pset( yyvsp[-3].parse, yyvsp[-1].parse, yyvsp[-2].number ); ;
    break;}
case 14:
#line 168 "jamgram.y"
{ yyval.parse = pset1( yyvsp[-5].parse, yyvsp[-3].parse, yyvsp[-1].parse, yyvsp[-2].number ); ;
    break;}
case 15:
#line 170 "jamgram.y"
{ yyval.parse = pbreak( yyvsp[-1].parse, JMP_BREAK ); ;
    break;}
case 16:
#line 172 "jamgram.y"
{ yyval.parse = pbreak( yyvsp[-1].parse, JMP_CONTINUE ); ;
    break;}
case 17:
#line 174 "jamgram.y"
{ yyval.parse = pbreak( yyvsp[-1].parse, JMP_RETURN ); ;
    break;}
case 18:
#line 176 "jamgram.y"
{ yyval.parse = pfor( yyvsp[-5].string, yyvsp[-3].parse, yyvsp[-1].parse ); ;
    break;}
case 19:
#line 178 "jamgram.y"
{ yyval.parse = pswitch( yyvsp[-3].parse, yyvsp[-1].parse ); ;
    break;}
case 20:
#line 180 "jamgram.y"
{ yyval.parse = pif( yyvsp[-3].parse, yyvsp[-1].parse, pnull() ); ;
    break;}
case 21:
#line 182 "jamgram.y"
{ yyval.parse = pif( yyvsp[-5].parse, yyvsp[-3].parse, yyvsp[0].parse ); ;
    break;}
case 22:
#line 184 "jamgram.y"
{ yyval.parse = pwhile( yyvsp[-3].parse, yyvsp[-1].parse ); ;
    break;}
case 23:
#line 186 "jamgram.y"
{ yyval.parse = psetc( yyvsp[-4].string, yyvsp[-3].parse, yyvsp[-1].parse ); ;
    break;}
case 24:
#line 188 "jamgram.y"
{ yyval.parse = pon( yyvsp[-1].parse, yyvsp[0].parse ); ;
    break;}
case 25:
#line 190 "jamgram.y"
{ yymode( SCAN_STRING ); ;
    break;}
case 26:
#line 192 "jamgram.y"
{ yymode( SCAN_NORMAL ); ;
    break;}
case 27:
#line 194 "jamgram.y"
{ yyval.parse = psete( yyvsp[-6].string,yyvsp[-5].parse,yyvsp[-2].string,yyvsp[-7].number ); ;
    break;}
case 28:
#line 202 "jamgram.y"
{ yyval.number = VAR_SET; ;
    break;}
case 29:
#line 204 "jamgram.y"
{ yyval.number = VAR_APPEND; ;
    break;}
case 30:
#line 206 "jamgram.y"
{ yyval.number = VAR_DEFAULT; ;
    break;}
case 31:
#line 208 "jamgram.y"
{ yyval.number = VAR_DEFAULT; ;
    break;}
case 32:
#line 216 "jamgram.y"
{ yyval.parse = peval( EXPR_EXISTS, yyvsp[0].parse, pnull() ); ;
    break;}
case 33:
#line 218 "jamgram.y"
{ yyval.parse = peval( EXPR_EQUALS, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 34:
#line 220 "jamgram.y"
{ yyval.parse = peval( EXPR_NOTEQ, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 35:
#line 222 "jamgram.y"
{ yyval.parse = peval( EXPR_LESS, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 36:
#line 224 "jamgram.y"
{ yyval.parse = peval( EXPR_LESSEQ, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 37:
#line 226 "jamgram.y"
{ yyval.parse = peval( EXPR_MORE, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 38:
#line 228 "jamgram.y"
{ yyval.parse = peval( EXPR_MOREEQ, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 39:
#line 230 "jamgram.y"
{ yyval.parse = peval( EXPR_AND, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 40:
#line 232 "jamgram.y"
{ yyval.parse = peval( EXPR_AND, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 41:
#line 234 "jamgram.y"
{ yyval.parse = peval( EXPR_OR, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 42:
#line 236 "jamgram.y"
{ yyval.parse = peval( EXPR_OR, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 43:
#line 238 "jamgram.y"
{ yyval.parse = peval( EXPR_IN, yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 44:
#line 240 "jamgram.y"
{ yyval.parse = peval( EXPR_NOT, yyvsp[0].parse, pnull() ); ;
    break;}
case 45:
#line 242 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; ;
    break;}
case 46:
#line 252 "jamgram.y"
{ yyval.parse = P0; ;
    break;}
case 47:
#line 254 "jamgram.y"
{ yyval.parse = pnode( yyvsp[-1].parse, yyvsp[0].parse ); ;
    break;}
case 48:
#line 258 "jamgram.y"
{ yyval.parse = psnode( yyvsp[-2].string, yyvsp[0].parse ); ;
    break;}
case 49:
#line 267 "jamgram.y"
{ yyval.parse = P0; ;
    break;}
case 50:
#line 269 "jamgram.y"
{ yyval.parse = psnode( yyvsp[-2].string, yyvsp[0].parse ); ;
    break;}
case 51:
#line 271 "jamgram.y"
{ yyval.parse = psnode( yyvsp[0].string, P0 ); ;
    break;}
case 52:
#line 280 "jamgram.y"
{ yyval.parse = pnode( P0, yyvsp[0].parse ); ;
    break;}
case 53:
#line 282 "jamgram.y"
{ yyval.parse = pnode( yyvsp[0].parse, yyvsp[-2].parse ); ;
    break;}
case 54:
#line 292 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; yymode( SCAN_NORMAL ); ;
    break;}
case 55:
#line 296 "jamgram.y"
{ yyval.parse = pnull(); yymode( SCAN_PUNCT ); ;
    break;}
case 56:
#line 298 "jamgram.y"
{ yyval.parse = pappend( yyvsp[-1].parse, yyvsp[0].parse ); ;
    break;}
case 57:
#line 302 "jamgram.y"
{ yyval.parse = plist( yyvsp[0].string ); ;
    break;}
case 58:
#line 303 "jamgram.y"
{ yymode( SCAN_NORMAL ); ;
    break;}
case 59:
#line 304 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; ;
    break;}
case 60:
#line 313 "jamgram.y"
{ yyval.parse = prule( yyvsp[-1].parse, yyvsp[0].parse ); ;
    break;}
case 61:
#line 315 "jamgram.y"
{ yyval.parse = pon( yyvsp[-2].parse, prule( yyvsp[-1].parse, yyvsp[0].parse ) ); ;
    break;}
case 62:
#line 317 "jamgram.y"
{ yyval.parse = pon( yyvsp[-2].parse, yyvsp[0].parse ); ;
    break;}
case 63:
#line 326 "jamgram.y"
{ yyval.number = 0; ;
    break;}
case 64:
#line 328 "jamgram.y"
{ yyval.number = yyvsp[-1].number | yyvsp[0].number; ;
    break;}
case 65:
#line 332 "jamgram.y"
{ yyval.number = RULE_UPDATED; ;
    break;}
case 66:
#line 334 "jamgram.y"
{ yyval.number = RULE_TOGETHER; ;
    break;}
case 67:
#line 336 "jamgram.y"
{ yyval.number = RULE_IGNORE; ;
    break;}
case 68:
#line 338 "jamgram.y"
{ yyval.number = RULE_QUIETLY; ;
    break;}
case 69:
#line 340 "jamgram.y"
{ yyval.number = RULE_PIECEMEAL; ;
    break;}
case 70:
#line 342 "jamgram.y"
{ yyval.number = RULE_EXISTING; ;
    break;}
case 71:
#line 344 "jamgram.y"
{ yyval.number = atoi( yyvsp[0].string ) * RULE_MAXLINE; ;
    break;}
case 72:
#line 353 "jamgram.y"
{ yyval.parse = pnull(); ;
    break;}
case 73:
#line 355 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
#ifndef YYSTACK_USE_ALLOCA
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
#ifndef YYSTACK_USE_ALLOCA
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
#endif
    }
  return 1;
}
#line 359 "jamgram.y"
