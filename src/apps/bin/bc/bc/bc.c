
/*  A Bison parser, made from bc.y
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	ENDOFLINE	257
#define	AND	258
#define	OR	259
#define	NOT	260
#define	STRING	261
#define	NAME	262
#define	NUMBER	263
#define	ASSIGN_OP	264
#define	REL_OP	265
#define	INCR_DECR	266
#define	Define	267
#define	Break	268
#define	Quit	269
#define	Length	270
#define	Return	271
#define	For	272
#define	If	273
#define	While	274
#define	Sqrt	275
#define	Else	276
#define	Scale	277
#define	Ibase	278
#define	Obase	279
#define	Auto	280
#define	Read	281
#define	Warranty	282
#define	Halt	283
#define	Last	284
#define	Continue	285
#define	Print	286
#define	Limits	287
#define	UNARY_MINUS	288
#define	HistoryVar	289

#line 1 "bc.y"

/* bc.y: The grammar for a POSIX compatable bc processor with some
         extensions to the language. */

/*  This file is part of GNU bc.
    Copyright (C) 1991, 1992, 1993, 1994, 1997 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License , or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to:
      The Free Software Foundation, Inc.
      59 Temple Place, Suite 330
      Boston, MA 02111 USA

    You may contact the author by:
       e-mail:  philnelson@acm.org
      us-mail:  Philip A. Nelson
                Computer Science Department, 9062
                Western Washington University
                Bellingham, WA 98226-9062
       
*************************************************************************/

#include "bcdefs.h"
#include "global.h"
#include "proto.h"

#line 40 "bc.y"
typedef union {
	char	 *s_value;
	char	  c_value;
	int	  i_value;
	arg_list *a_value;
       } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		185
#define	YYFLAG		-32768
#define	YYNTBASE	50

#define YYTRANSLATE(x) ((unsigned)(x) <= 289 ? yytranslate[x] : 84)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,    40,     2,     2,    43,
    44,    38,    36,    47,    37,     2,    39,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    42,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    48,     2,    49,    41,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    45,     2,    46,     2,     2,     2,     2,     2,
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
    27,    28,    29,    30,    31,    32,    33,    34,    35
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     7,     9,    12,    13,    15,    16,    18,
    22,    25,    26,    28,    31,    35,    38,    42,    44,    47,
    49,    51,    53,    55,    57,    59,    61,    63,    66,    67,
    68,    69,    70,    85,    86,    95,    96,    97,   106,   110,
   111,   115,   117,   121,   123,   125,   126,   127,   132,   133,
   146,   147,   149,   150,   154,   158,   160,   164,   169,   173,
   179,   186,   187,   189,   191,   195,   199,   205,   206,   208,
   209,   211,   212,   217,   218,   223,   224,   229,   232,   236,
   240,   244,   248,   252,   256,   260,   263,   265,   267,   271,
   276,   279,   282,   287,   292,   297,   301,   303,   308,   310,
   312,   314,   316,   318,   319,   321
};

static const short yyrhs[] = {    -1,
    50,    51,     0,    53,     3,     0,    69,     0,     1,     3,
     0,     0,     3,     0,     0,    55,     0,    53,    42,    55,
     0,    53,    42,     0,     0,    55,     0,    54,     3,     0,
    54,     3,    55,     0,    54,    42,     0,    54,    42,    56,
     0,    56,     0,     1,    56,     0,    28,     0,    33,     0,
    78,     0,     7,     0,    14,     0,    31,     0,    15,     0,
    29,     0,    17,    77,     0,     0,     0,     0,     0,    18,
    57,    43,    76,    42,    58,    76,    42,    59,    76,    44,
    60,    52,    56,     0,     0,    19,    43,    78,    44,    61,
    52,    56,    67,     0,     0,     0,    20,    62,    43,    78,
    63,    44,    52,    56,     0,    45,    54,    46,     0,     0,
    32,    64,    65,     0,    66,     0,    66,    47,    65,     0,
     7,     0,    78,     0,     0,     0,    22,    68,    52,    56,
     0,     0,    13,     8,    43,    71,    44,    52,    45,    83,
    72,    70,    54,    46,     0,     0,    73,     0,     0,    26,
    73,     3,     0,    26,    73,    42,     0,     8,     0,     8,
    48,    49,     0,    38,     8,    48,    49,     0,    73,    47,
     8,     0,    73,    47,     8,    48,    49,     0,    73,    47,
    38,     8,    48,    49,     0,     0,    75,     0,    78,     0,
     8,    48,    49,     0,    75,    47,    78,     0,    75,    47,
     8,    48,    49,     0,     0,    78,     0,     0,    78,     0,
     0,    82,    10,    79,    78,     0,     0,    78,     4,    80,
    78,     0,     0,    78,     5,    81,    78,     0,     6,    78,
     0,    78,    11,    78,     0,    78,    36,    78,     0,    78,
    37,    78,     0,    78,    38,    78,     0,    78,    39,    78,
     0,    78,    40,    78,     0,    78,    41,    78,     0,    37,
    78,     0,    82,     0,     9,     0,    43,    78,    44,     0,
     8,    43,    74,    44,     0,    12,    82,     0,    82,    12,
     0,    16,    43,    78,    44,     0,    21,    43,    78,    44,
     0,    23,    43,    78,    44,     0,    27,    43,    44,     0,
     8,     0,     8,    48,    78,    49,     0,    24,     0,    25,
     0,    23,     0,    35,     0,    30,     0,     0,     3,     0,
    83,     3,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   107,   116,   118,   120,   122,   128,   129,   132,   134,   135,
   136,   138,   140,   141,   142,   143,   144,   146,   147,   150,
   152,   154,   163,   170,   180,   191,   193,   195,   197,   202,
   212,   223,   233,   241,   248,   254,   260,   267,   273,   275,
   278,   279,   280,   282,   288,   291,   292,   300,   301,   315,
   321,   323,   325,   327,   329,   332,   334,   336,   338,   340,
   342,   345,   347,   349,   354,   360,   365,   381,   386,   388,
   393,   401,   413,   428,   436,   441,   449,   457,   463,   491,
   496,   501,   506,   511,   516,   521,   526,   535,   551,   553,
   569,   588,   611,   613,   615,   617,   623,   625,   630,   632,
   634,   636,   640,   647,   648,   649
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","ENDOFLINE",
"AND","OR","NOT","STRING","NAME","NUMBER","ASSIGN_OP","REL_OP","INCR_DECR","Define",
"Break","Quit","Length","Return","For","If","While","Sqrt","Else","Scale","Ibase",
"Obase","Auto","Read","Warranty","Halt","Last","Continue","Print","Limits","UNARY_MINUS",
"HistoryVar","'+'","'-'","'*'","'/'","'%'","'^'","';'","'('","')'","'{'","'}'",
"','","'['","']'","program","input_item","opt_newline","semicolon_list","statement_list",
"statement_or_error","statement","@1","@2","@3","@4","@5","@6","@7","@8","print_list",
"print_element","opt_else","@9","function","@10","opt_parameter_list","opt_auto_define_list",
"define_list","opt_argument_list","argument_list","opt_expression","return_expression",
"expression","@11","@12","@13","named_expression","required_eol", NULL
};
#endif

static const short yyr1[] = {     0,
    50,    50,    51,    51,    51,    52,    52,    53,    53,    53,
    53,    54,    54,    54,    54,    54,    54,    55,    55,    56,
    56,    56,    56,    56,    56,    56,    56,    56,    57,    58,
    59,    60,    56,    61,    56,    62,    63,    56,    56,    64,
    56,    65,    65,    66,    66,    67,    68,    67,    70,    69,
    71,    71,    72,    72,    72,    73,    73,    73,    73,    73,
    73,    74,    74,    75,    75,    75,    75,    76,    76,    77,
    77,    79,    78,    80,    78,    81,    78,    78,    78,    78,
    78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
    78,    78,    78,    78,    78,    78,    82,    82,    82,    82,
    82,    82,    82,    83,    83,    83
};

static const short yyr2[] = {     0,
     0,     2,     2,     1,     2,     0,     1,     0,     1,     3,
     2,     0,     1,     2,     3,     2,     3,     1,     2,     1,
     1,     1,     1,     1,     1,     1,     1,     2,     0,     0,
     0,     0,    14,     0,     8,     0,     0,     8,     3,     0,
     3,     1,     3,     1,     1,     0,     0,     4,     0,    12,
     0,     1,     0,     3,     3,     1,     3,     4,     3,     5,
     6,     0,     1,     1,     3,     3,     5,     0,     1,     0,
     1,     0,     4,     0,     4,     0,     4,     2,     3,     3,
     3,     3,     3,     3,     3,     2,     1,     1,     3,     4,
     2,     2,     4,     4,     4,     3,     1,     4,     1,     1,
     1,     1,     1,     0,     1,     2
};

static const short yydefact[] = {     1,
     0,     0,     0,    23,    97,    88,     0,     0,    24,    26,
     0,    70,    29,     0,    36,     0,   101,    99,   100,     0,
    20,    27,   103,    25,    40,    21,   102,     0,     0,     0,
     2,     0,     9,    18,     4,    22,    87,     5,    19,    78,
    62,     0,    97,   101,    91,     0,     0,    28,    71,     0,
     0,     0,     0,     0,     0,     0,    86,     0,     0,     0,
    13,     3,     0,    74,    76,     0,     0,     0,     0,     0,
     0,     0,    72,    92,    97,     0,    63,    64,     0,    51,
     0,    68,     0,     0,     0,     0,    96,    44,    41,    42,
    45,    89,     0,    16,    39,    10,     0,     0,    79,    80,
    81,    82,    83,    84,    85,     0,     0,    90,     0,    98,
    56,     0,     0,    52,    93,     0,    69,    34,    37,    94,
    95,     0,    15,    17,    75,    77,    73,    65,    97,    66,
     0,     0,     6,     0,    30,     6,     0,    43,     0,    57,
     0,     7,     0,    59,     0,    68,     0,     6,    67,    58,
   104,     0,     0,     0,    46,     0,   105,    53,    60,     0,
    31,    47,    35,    38,   106,     0,    49,    61,    68,     6,
     0,     0,     0,     0,    54,    55,     0,    32,    48,    50,
     6,     0,    33,     0,     0
};

static const short yydefgoto[] = {     1,
    31,   143,    32,    60,    61,    34,    50,   146,   169,   181,
   136,    52,   137,    56,    89,    90,   163,   170,    35,   172,
   113,   167,   114,    76,    77,   116,    48,    36,   106,    97,
    98,    37,   158
};

static const short yypact[] = {-32768,
   170,   375,   567,-32768,   -25,-32768,    -3,     7,-32768,-32768,
   -32,   567,-32768,   -29,-32768,   -26,    -7,-32768,-32768,    -5,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   567,   567,   213,
-32768,    16,-32768,-32768,-32768,   642,    14,-32768,-32768,    63,
   597,   567,    -9,-32768,-32768,    18,   567,-32768,   642,    19,
   567,    20,   567,   567,    15,   537,-32768,   122,   505,     3,
-32768,-32768,   305,-32768,-32768,   567,   567,   567,   567,   567,
   567,   567,-32768,-32768,   -18,    21,    26,   642,    39,    -4,
   410,   567,   419,   567,   428,   466,-32768,-32768,-32768,    36,
   642,-32768,   259,   505,-32768,-32768,   567,   567,   404,   316,
   316,    44,    44,    44,    44,   567,   107,-32768,   627,-32768,
    -8,    79,    45,    43,-32768,    49,   642,-32768,   642,-32768,
-32768,   537,-32768,-32768,    63,   652,   404,-32768,    38,   642,
    46,    48,    90,    -1,-32768,    90,    61,-32768,   337,-32768,
    59,-32768,    65,    64,   103,   567,   505,    90,-32768,-32768,
   111,    68,    70,    78,    99,   505,-32768,     5,-32768,    75,
-32768,-32768,-32768,-32768,-32768,    -4,-32768,-32768,   567,    90,
    13,   213,    81,   505,-32768,-32768,     6,-32768,-32768,-32768,
    90,   505,-32768,   129,-32768
};

static const short yypgoto[] = {-32768,
-32768,  -135,-32768,   -37,     1,    -2,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,    25,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   -30,-32768,-32768,  -136,-32768,     0,-32768,-32768,
-32768,   131,-32768
};


#define	YYLAST		693


static const short yytable[] = {    39,
   147,    33,    40,   111,    43,    93,   144,   165,    93,   154,
    47,    49,   156,    51,    46,   175,    53,    41,    62,    44,
    18,    19,    42,    73,    41,    74,    23,    57,    58,   107,
   166,    27,   173,   112,   174,    54,   145,    55,    42,   131,
    78,    79,    64,    65,    94,   182,    81,    94,    95,    66,
    83,   180,    85,    86,   176,    91,    39,    63,    87,   134,
    80,    82,    84,    96,   108,    99,   100,   101,   102,   103,
   104,   105,   109,    66,    67,    68,    69,    70,    71,    72,
    41,   117,   122,   119,    72,   139,   132,   110,   133,   134,
   135,   124,   142,   123,   140,   141,   125,   126,    67,    68,
    69,    70,    71,    72,   148,   127,    79,   150,   130,   151,
   153,   152,     3,   157,     5,     6,   159,   160,     7,   161,
   162,    91,    11,   168,   178,    64,    65,    16,   185,    17,
    18,    19,    66,    20,   177,   171,    23,    45,    79,     0,
     0,    27,     0,    28,   155,   117,   138,     0,     0,    29,
     0,     0,     0,   164,     0,   128,     0,    67,    68,    69,
    70,    71,    72,     0,     0,    92,     0,     0,   117,   184,
     2,   179,    -8,     0,     0,     3,     4,     5,     6,   183,
     0,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,     0,    17,    18,    19,     0,    20,    21,    22,    23,
    24,    25,    26,     0,    27,     0,    28,     0,     0,     0,
     0,    -8,    29,    59,    30,   -12,     0,     0,     3,     4,
     5,     6,     0,     0,     7,     0,     9,    10,    11,    12,
    13,    14,    15,    16,     0,    17,    18,    19,     0,    20,
    21,    22,    23,    24,    25,    26,     0,    27,     0,    28,
     0,     0,     0,     0,   -12,    29,     0,    30,   -12,    59,
     0,   -14,     0,     0,     3,     4,     5,     6,     0,     0,
     7,     0,     9,    10,    11,    12,    13,    14,    15,    16,
     0,    17,    18,    19,     0,    20,    21,    22,    23,    24,
    25,    26,     0,    27,     0,    28,     0,     0,     0,     0,
   -14,    29,     0,    30,   -14,    59,     0,   -11,     0,     0,
     3,     4,     5,     6,     0,     0,     7,     0,     9,    10,
    11,    12,    13,    14,    15,    16,     0,    17,    18,    19,
     0,    20,    21,    22,    23,    24,    25,    26,     0,    27,
     0,    28,     3,     0,     5,     6,   -11,    29,     7,    30,
     0,     0,    11,    69,    70,    71,    72,    16,     0,    17,
    18,    19,     0,    20,     0,     0,    23,     0,     0,     0,
     0,    27,     0,    28,     0,     0,     0,    38,     0,    29,
     3,     4,     5,     6,     0,   149,     7,     0,     9,    10,
    11,    12,    13,    14,    15,    16,     0,    17,    18,    19,
     0,    20,    21,    22,    23,    24,    25,    26,     0,    27,
     0,    28,     0,    64,    65,     0,     0,    29,     0,    30,
    66,     0,    64,    65,     0,     0,     0,     0,     0,    66,
     0,    64,    65,     0,     0,     0,     0,     0,    66,    67,
    68,    69,    70,    71,    72,    67,    68,    69,    70,    71,
    72,     0,     0,   115,    67,    68,    69,    70,    71,    72,
     0,     0,   118,    67,    68,    69,    70,    71,    72,    64,
    65,   120,     0,     0,     0,     0,    66,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    67,    68,    69,    70,    71,    72,     0,     0,   121,
     3,     4,     5,     6,     0,     0,     7,     0,     9,    10,
    11,    12,    13,    14,    15,    16,     0,    17,    18,    19,
     0,    20,    21,    22,    23,    24,    25,    26,     0,    27,
     0,    28,     3,    88,     5,     6,     0,    29,     7,    30,
     0,     0,    11,     0,     0,     0,     0,    16,     0,    17,
    18,    19,     0,    20,     0,     0,    23,     0,     0,     0,
     0,    27,     3,    28,     5,     6,     0,     0,     7,    29,
     0,     0,    11,     0,     0,     0,     0,    16,     0,    17,
    18,    19,     0,    20,     0,     0,    23,     0,     0,     0,
     0,    27,     3,    28,    75,     6,     0,     0,     7,    29,
     0,     0,    11,     0,     0,     0,     0,    16,     0,    17,
    18,    19,     0,    20,     0,     0,    23,     0,     0,     0,
     0,    27,     3,    28,   129,     6,     0,     0,     7,    29,
     0,     0,    11,     0,     0,    64,    65,    16,     0,    17,
    18,    19,    66,    20,     0,    64,    23,     0,     0,     0,
     0,    27,    66,    28,     0,     0,     0,     0,     0,    29,
     0,     0,     0,     0,     0,     0,     0,    67,    68,    69,
    70,    71,    72,     0,     0,     0,     0,    67,    68,    69,
    70,    71,    72
};

static const short yycheck[] = {     2,
   136,     1,     3,     8,     8,     3,     8,     3,     3,   146,
    43,    12,   148,    43,     8,     3,    43,    43,     3,    23,
    24,    25,    48,    10,    43,    12,    30,    28,    29,    48,
    26,    35,   169,    38,   170,    43,    38,    43,    48,    48,
    41,    42,     4,     5,    42,   181,    47,    42,    46,    11,
    51,    46,    53,    54,    42,    56,    59,    42,    44,    47,
    43,    43,    43,    63,    44,    66,    67,    68,    69,    70,
    71,    72,    47,    11,    36,    37,    38,    39,    40,    41,
    43,    82,    47,    84,    41,    48,     8,    49,    44,    47,
    42,    94,     3,    93,    49,    48,    97,    98,    36,    37,
    38,    39,    40,    41,    44,   106,   107,    49,   109,    45,
     8,    48,     6,     3,     8,     9,    49,    48,    12,    42,
    22,   122,    16,    49,    44,     4,     5,    21,     0,    23,
    24,    25,    11,    27,   172,   166,    30,     7,   139,    -1,
    -1,    35,    -1,    37,   147,   146,   122,    -1,    -1,    43,
    -1,    -1,    -1,   156,    -1,    49,    -1,    36,    37,    38,
    39,    40,    41,    -1,    -1,    44,    -1,    -1,   169,     0,
     1,   174,     3,    -1,    -1,     6,     7,     8,     9,   182,
    -1,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    -1,    23,    24,    25,    -1,    27,    28,    29,    30,
    31,    32,    33,    -1,    35,    -1,    37,    -1,    -1,    -1,
    -1,    42,    43,     1,    45,     3,    -1,    -1,     6,     7,
     8,     9,    -1,    -1,    12,    -1,    14,    15,    16,    17,
    18,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
    28,    29,    30,    31,    32,    33,    -1,    35,    -1,    37,
    -1,    -1,    -1,    -1,    42,    43,    -1,    45,    46,     1,
    -1,     3,    -1,    -1,     6,     7,     8,     9,    -1,    -1,
    12,    -1,    14,    15,    16,    17,    18,    19,    20,    21,
    -1,    23,    24,    25,    -1,    27,    28,    29,    30,    31,
    32,    33,    -1,    35,    -1,    37,    -1,    -1,    -1,    -1,
    42,    43,    -1,    45,    46,     1,    -1,     3,    -1,    -1,
     6,     7,     8,     9,    -1,    -1,    12,    -1,    14,    15,
    16,    17,    18,    19,    20,    21,    -1,    23,    24,    25,
    -1,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
    -1,    37,     6,    -1,     8,     9,    42,    43,    12,    45,
    -1,    -1,    16,    38,    39,    40,    41,    21,    -1,    23,
    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,    -1,
    -1,    35,    -1,    37,    -1,    -1,    -1,     3,    -1,    43,
     6,     7,     8,     9,    -1,    49,    12,    -1,    14,    15,
    16,    17,    18,    19,    20,    21,    -1,    23,    24,    25,
    -1,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
    -1,    37,    -1,     4,     5,    -1,    -1,    43,    -1,    45,
    11,    -1,     4,     5,    -1,    -1,    -1,    -1,    -1,    11,
    -1,     4,     5,    -1,    -1,    -1,    -1,    -1,    11,    36,
    37,    38,    39,    40,    41,    36,    37,    38,    39,    40,
    41,    -1,    -1,    44,    36,    37,    38,    39,    40,    41,
    -1,    -1,    44,    36,    37,    38,    39,    40,    41,     4,
     5,    44,    -1,    -1,    -1,    -1,    11,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    36,    37,    38,    39,    40,    41,    -1,    -1,    44,
     6,     7,     8,     9,    -1,    -1,    12,    -1,    14,    15,
    16,    17,    18,    19,    20,    21,    -1,    23,    24,    25,
    -1,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
    -1,    37,     6,     7,     8,     9,    -1,    43,    12,    45,
    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,    -1,    23,
    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,    -1,
    -1,    35,     6,    37,     8,     9,    -1,    -1,    12,    43,
    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,    -1,    23,
    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,    -1,
    -1,    35,     6,    37,     8,     9,    -1,    -1,    12,    43,
    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,    -1,    23,
    24,    25,    -1,    27,    -1,    -1,    30,    -1,    -1,    -1,
    -1,    35,     6,    37,     8,     9,    -1,    -1,    12,    43,
    -1,    -1,    16,    -1,    -1,     4,     5,    21,    -1,    23,
    24,    25,    11,    27,    -1,     4,    30,    -1,    -1,    -1,
    -1,    35,    11,    37,    -1,    -1,    -1,    -1,    -1,    43,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    38,
    39,    40,    41,    -1,    -1,    -1,    -1,    36,    37,    38,
    39,    40,    41
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/gnu/share/bison.simple"
/* This file comes from bison-1.27.  */

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
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
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

#line 216 "/usr/gnu/share/bison.simple"

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
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
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

case 1:
#line 108 "bc.y"
{
			      yyval.i_value = 0;
			      if (interactive && !quiet)
				{
				  show_bc_version ();
				  welcome ();
				}
			    ;
    break;}
case 3:
#line 119 "bc.y"
{ run_code (); ;
    break;}
case 4:
#line 121 "bc.y"
{ run_code (); ;
    break;}
case 5:
#line 123 "bc.y"
{
			      yyerrok;
			      init_gen ();
			    ;
    break;}
case 7:
#line 130 "bc.y"
{ warn ("newline not allowed"); ;
    break;}
case 8:
#line 133 "bc.y"
{ yyval.i_value = 0; ;
    break;}
case 12:
#line 139 "bc.y"
{ yyval.i_value = 0; ;
    break;}
case 19:
#line 148 "bc.y"
{ yyval.i_value = yyvsp[0].i_value; ;
    break;}
case 20:
#line 151 "bc.y"
{ warranty (""); ;
    break;}
case 21:
#line 153 "bc.y"
{ limits (); ;
    break;}
case 22:
#line 155 "bc.y"
{
			      if (yyvsp[0].i_value & 2)
				warn ("comparison in expression");
			      if (yyvsp[0].i_value & 1)
				generate ("W");
			      else 
				generate ("p");
			    ;
    break;}
case 23:
#line 164 "bc.y"
{
			      yyval.i_value = 0;
			      generate ("w");
			      generate (yyvsp[0].s_value);
			      free (yyvsp[0].s_value);
			    ;
    break;}
case 24:
#line 171 "bc.y"
{
			      if (break_label == 0)
				yyerror ("Break outside a for/while");
			      else
				{
				  sprintf (genstr, "J%1d:", break_label);
				  generate (genstr);
				}
			    ;
    break;}
case 25:
#line 181 "bc.y"
{
			      warn ("Continue statement");
			      if (continue_label == 0)
				yyerror ("Continue outside a for");
			      else
				{
				  sprintf (genstr, "J%1d:", continue_label);
				  generate (genstr);
				}
			    ;
    break;}
case 26:
#line 192 "bc.y"
{ exit (0); ;
    break;}
case 27:
#line 194 "bc.y"
{ generate ("h"); ;
    break;}
case 28:
#line 196 "bc.y"
{ generate ("R"); ;
    break;}
case 29:
#line 198 "bc.y"
{
			      yyvsp[0].i_value = break_label; 
			      break_label = next_label++;
			    ;
    break;}
case 30:
#line 203 "bc.y"
{
			      if (yyvsp[-1].i_value & 2)
				warn ("Comparison in first for expression");
			      if (yyvsp[-1].i_value >= 0)
				generate ("p");
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "N%1d:", yyvsp[-1].i_value);
			      generate (genstr);
			    ;
    break;}
case 31:
#line 213 "bc.y"
{
			      if (yyvsp[-1].i_value < 0) generate ("1");
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "B%1d:J%1d:", yyvsp[-1].i_value, break_label);
			      generate (genstr);
			      yyval.i_value = continue_label;
			      continue_label = next_label++;
			      sprintf (genstr, "N%1d:", continue_label);
			      generate (genstr);
			    ;
    break;}
case 32:
#line 224 "bc.y"
{
			      if (yyvsp[-1].i_value & 2 )
				warn ("Comparison in third for expression");
			      if (yyvsp[-1].i_value & 16)
				sprintf (genstr, "J%1d:N%1d:", yyvsp[-7].i_value, yyvsp[-4].i_value);
			      else
				sprintf (genstr, "pJ%1d:N%1d:", yyvsp[-7].i_value, yyvsp[-4].i_value);
			      generate (genstr);
			    ;
    break;}
case 33:
#line 234 "bc.y"
{
			      sprintf (genstr, "J%1d:N%1d:",
				       continue_label, break_label);
			      generate (genstr);
			      break_label = yyvsp[-13].i_value;
			      continue_label = yyvsp[-5].i_value;
			    ;
    break;}
case 34:
#line 242 "bc.y"
{
			      yyvsp[-1].i_value = if_label;
			      if_label = next_label++;
			      sprintf (genstr, "Z%1d:", if_label);
			      generate (genstr);
			    ;
    break;}
case 35:
#line 249 "bc.y"
{
			      sprintf (genstr, "N%1d:", if_label); 
			      generate (genstr);
			      if_label = yyvsp[-5].i_value;
			    ;
    break;}
case 36:
#line 255 "bc.y"
{
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "N%1d:", yyvsp[0].i_value);
			      generate (genstr);
			    ;
    break;}
case 37:
#line 261 "bc.y"
{
			      yyvsp[0].i_value = break_label; 
			      break_label = next_label++;
			      sprintf (genstr, "Z%1d:", break_label);
			      generate (genstr);
			    ;
    break;}
case 38:
#line 268 "bc.y"
{
			      sprintf (genstr, "J%1d:N%1d:", yyvsp[-7].i_value, break_label);
			      generate (genstr);
			      break_label = yyvsp[-4].i_value;
			    ;
    break;}
case 39:
#line 274 "bc.y"
{ yyval.i_value = 0; ;
    break;}
case 40:
#line 276 "bc.y"
{  warn ("print statement"); ;
    break;}
case 44:
#line 283 "bc.y"
{
			      generate ("O");
			      generate (yyvsp[0].s_value);
			      free (yyvsp[0].s_value);
			    ;
    break;}
case 45:
#line 289 "bc.y"
{ generate ("P"); ;
    break;}
case 47:
#line 293 "bc.y"
{
			      warn ("else clause in if statement");
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "J%d:N%1d:", yyvsp[0].i_value, if_label); 
			      generate (genstr);
			      if_label = yyvsp[0].i_value;
			    ;
    break;}
case 49:
#line 303 "bc.y"
{
			      /* Check auto list against parameter list? */
			      check_params (yyvsp[-5].a_value,yyvsp[0].a_value);
			      sprintf (genstr, "F%d,%s.%s[",
				       lookup(yyvsp[-7].s_value,FUNCTDEF), 
				       arg_str (yyvsp[-5].a_value), arg_str (yyvsp[0].a_value));
			      generate (genstr);
			      free_args (yyvsp[-5].a_value);
			      free_args (yyvsp[0].a_value);
			      yyvsp[-8].i_value = next_label;
			      next_label = 1;
			    ;
    break;}
case 50:
#line 316 "bc.y"
{
			      generate ("0R]");
			      next_label = yyvsp[-11].i_value;
			    ;
    break;}
case 51:
#line 322 "bc.y"
{ yyval.a_value = NULL; ;
    break;}
case 53:
#line 326 "bc.y"
{ yyval.a_value = NULL; ;
    break;}
case 54:
#line 328 "bc.y"
{ yyval.a_value = yyvsp[-1].a_value; ;
    break;}
case 55:
#line 330 "bc.y"
{ yyval.a_value = yyvsp[-1].a_value; ;
    break;}
case 56:
#line 333 "bc.y"
{ yyval.a_value = nextarg (NULL, lookup (yyvsp[0].s_value,SIMPLE), FALSE);;
    break;}
case 57:
#line 335 "bc.y"
{ yyval.a_value = nextarg (NULL, lookup (yyvsp[-2].s_value,ARRAY), FALSE); ;
    break;}
case 58:
#line 337 "bc.y"
{ yyval.a_value = nextarg (NULL, lookup (yyvsp[-2].s_value,ARRAY), TRUE); ;
    break;}
case 59:
#line 339 "bc.y"
{ yyval.a_value = nextarg (yyvsp[-2].a_value, lookup (yyvsp[0].s_value,SIMPLE), FALSE); ;
    break;}
case 60:
#line 341 "bc.y"
{ yyval.a_value = nextarg (yyvsp[-4].a_value, lookup (yyvsp[-2].s_value,ARRAY), FALSE); ;
    break;}
case 61:
#line 343 "bc.y"
{ yyval.a_value = nextarg (yyvsp[-5].a_value, lookup (yyvsp[-2].s_value,ARRAY), TRUE); ;
    break;}
case 62:
#line 346 "bc.y"
{ yyval.a_value = NULL; ;
    break;}
case 64:
#line 350 "bc.y"
{
			      if (yyvsp[0].i_value & 2) warn ("comparison in argument");
			      yyval.a_value = nextarg (NULL,0,FALSE);
			    ;
    break;}
case 65:
#line 355 "bc.y"
{
			      sprintf (genstr, "K%d:", -lookup (yyvsp[-2].s_value,ARRAY));
			      generate (genstr);
			      yyval.a_value = nextarg (NULL,1,FALSE);
			    ;
    break;}
case 66:
#line 361 "bc.y"
{
			      if (yyvsp[0].i_value & 2) warn ("comparison in argument");
			      yyval.a_value = nextarg (yyvsp[-2].a_value,0,FALSE);
			    ;
    break;}
case 67:
#line 366 "bc.y"
{
			      sprintf (genstr, "K%d:", -lookup (yyvsp[-2].s_value,ARRAY));
			      generate (genstr);
			      yyval.a_value = nextarg (yyvsp[-4].a_value,1,FALSE);
			    ;
    break;}
case 68:
#line 382 "bc.y"
{
			      yyval.i_value = 16;
			      warn ("Missing expression in for statement");
			    ;
    break;}
case 70:
#line 389 "bc.y"
{
			      yyval.i_value = 0;
			      generate ("0");
			    ;
    break;}
case 71:
#line 394 "bc.y"
{
			      if (yyvsp[0].i_value & 2)
				warn ("comparison in return expresion");
			      if (!(yyvsp[0].i_value & 4))
				warn ("return expression requires parenthesis");
			    ;
    break;}
case 72:
#line 402 "bc.y"
{
			      if (yyvsp[0].c_value != '=')
				{
				  if (yyvsp[-1].i_value < 0)
				    sprintf (genstr, "DL%d:", -yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "l%d:", yyvsp[-1].i_value);
				  generate (genstr);
				}
			    ;
    break;}
case 73:
#line 413 "bc.y"
{
			      if (yyvsp[0].i_value & 2) warn("comparison in assignment");
			      if (yyvsp[-2].c_value != '=')
				{
				  sprintf (genstr, "%c", yyvsp[-2].c_value);
				  generate (genstr);
				}
			      if (yyvsp[-3].i_value < 0)
				sprintf (genstr, "S%d:", -yyvsp[-3].i_value);
			      else
				sprintf (genstr, "s%d:", yyvsp[-3].i_value);
			      generate (genstr);
			      yyval.i_value = 0;
			    ;
    break;}
case 74:
#line 429 "bc.y"
{
			      warn("&& operator");
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "DZ%d:p", yyvsp[0].i_value);
			      generate (genstr);
			    ;
    break;}
case 75:
#line 436 "bc.y"
{
			      sprintf (genstr, "DZ%d:p1N%d:", yyvsp[-2].i_value, yyvsp[-2].i_value);
			      generate (genstr);
			      yyval.i_value = (yyvsp[-3].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 76:
#line 442 "bc.y"
{
			      warn("|| operator");
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "B%d:", yyvsp[0].i_value);
			      generate (genstr);
			    ;
    break;}
case 77:
#line 449 "bc.y"
{
			      int tmplab;
			      tmplab = next_label++;
			      sprintf (genstr, "B%d:0J%d:N%d:1N%d:",
				       yyvsp[-2].i_value, tmplab, yyvsp[-2].i_value, tmplab);
			      generate (genstr);
			      yyval.i_value = (yyvsp[-3].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 78:
#line 458 "bc.y"
{
			      yyval.i_value = yyvsp[0].i_value & ~4;
			      warn("! operator");
			      generate ("!");
			    ;
    break;}
case 79:
#line 464 "bc.y"
{
			      yyval.i_value = 3;
			      switch (*(yyvsp[-1].s_value))
				{
				case '=':
				  generate ("=");
				  break;

				case '!':
				  generate ("#");
				  break;

				case '<':
				  if (yyvsp[-1].s_value[1] == '=')
				    generate ("{");
				  else
				    generate ("<");
				  break;

				case '>':
				  if (yyvsp[-1].s_value[1] == '=')
				    generate ("}");
				  else
				    generate (">");
				  break;
				}
			    ;
    break;}
case 80:
#line 492 "bc.y"
{
			      generate ("+");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 81:
#line 497 "bc.y"
{
			      generate ("-");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 82:
#line 502 "bc.y"
{
			      generate ("*");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 83:
#line 507 "bc.y"
{
			      generate ("/");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 84:
#line 512 "bc.y"
{
			      generate ("%");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 85:
#line 517 "bc.y"
{
			      generate ("^");
			      yyval.i_value = (yyvsp[-2].i_value | yyvsp[0].i_value) & ~4;
			    ;
    break;}
case 86:
#line 522 "bc.y"
{
			      generate ("n");
			      yyval.i_value = yyvsp[0].i_value & ~4;
			    ;
    break;}
case 87:
#line 527 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[0].i_value < 0)
				sprintf (genstr, "L%d:", -yyvsp[0].i_value);
			      else
				sprintf (genstr, "l%d:", yyvsp[0].i_value);
			      generate (genstr);
			    ;
    break;}
case 88:
#line 536 "bc.y"
{
			      int len = strlen(yyvsp[0].s_value);
			      yyval.i_value = 1;
			      if (len == 1 && *yyvsp[0].s_value == '0')
				generate ("0");
			      else if (len == 1 && *yyvsp[0].s_value == '1')
				generate ("1");
			      else
				{
				  generate ("K");
				  generate (yyvsp[0].s_value);
				  generate (":");
				}
			      free (yyvsp[0].s_value);
			    ;
    break;}
case 89:
#line 552 "bc.y"
{ yyval.i_value = yyvsp[-1].i_value | 5; ;
    break;}
case 90:
#line 554 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[-1].a_value != NULL)
				{ 
				  sprintf (genstr, "C%d,%s:",
					   lookup (yyvsp[-3].s_value,FUNCT),
					   call_str (yyvsp[-1].a_value));
				  free_args (yyvsp[-1].a_value);
				}
			      else
				{
				  sprintf (genstr, "C%d:", lookup (yyvsp[-3].s_value,FUNCT));
				}
			      generate (genstr);
			    ;
    break;}
case 91:
#line 570 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[0].i_value < 0)
				{
				  if (yyvsp[-1].c_value == '+')
				    sprintf (genstr, "DA%d:L%d:", -yyvsp[0].i_value, -yyvsp[0].i_value);
				  else
				    sprintf (genstr, "DM%d:L%d:", -yyvsp[0].i_value, -yyvsp[0].i_value);
				}
			      else
				{
				  if (yyvsp[-1].c_value == '+')
				    sprintf (genstr, "i%d:l%d:", yyvsp[0].i_value, yyvsp[0].i_value);
				  else
				    sprintf (genstr, "d%d:l%d:", yyvsp[0].i_value, yyvsp[0].i_value);
				}
			      generate (genstr);
			    ;
    break;}
case 92:
#line 589 "bc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[-1].i_value < 0)
				{
				  sprintf (genstr, "DL%d:x", -yyvsp[-1].i_value);
				  generate (genstr); 
				  if (yyvsp[0].c_value == '+')
				    sprintf (genstr, "A%d:", -yyvsp[-1].i_value);
				  else
				      sprintf (genstr, "M%d:", -yyvsp[-1].i_value);
				}
			      else
				{
				  sprintf (genstr, "l%d:", yyvsp[-1].i_value);
				  generate (genstr);
				  if (yyvsp[0].c_value == '+')
				    sprintf (genstr, "i%d:", yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "d%d:", yyvsp[-1].i_value);
				}
			      generate (genstr);
			    ;
    break;}
case 93:
#line 612 "bc.y"
{ generate ("cL"); yyval.i_value = 1;;
    break;}
case 94:
#line 614 "bc.y"
{ generate ("cR"); yyval.i_value = 1;;
    break;}
case 95:
#line 616 "bc.y"
{ generate ("cS"); yyval.i_value = 1;;
    break;}
case 96:
#line 618 "bc.y"
{
			      warn ("read function");
			      generate ("cI"); yyval.i_value = 1;
			    ;
    break;}
case 97:
#line 624 "bc.y"
{ yyval.i_value = lookup(yyvsp[0].s_value,SIMPLE); ;
    break;}
case 98:
#line 626 "bc.y"
{
			      if (yyvsp[-1].i_value > 1) warn("comparison in subscript");
			      yyval.i_value = lookup(yyvsp[-3].s_value,ARRAY);
			    ;
    break;}
case 99:
#line 631 "bc.y"
{ yyval.i_value = 0; ;
    break;}
case 100:
#line 633 "bc.y"
{ yyval.i_value = 1; ;
    break;}
case 101:
#line 635 "bc.y"
{ yyval.i_value = 2; ;
    break;}
case 102:
#line 637 "bc.y"
{ yyval.i_value = 3;
			      warn ("History variable");
			    ;
    break;}
case 103:
#line 641 "bc.y"
{ yyval.i_value = 4;
			      warn ("Last variable");
			    ;
    break;}
case 104:
#line 647 "bc.y"
{ warn ("End of line required"); ;
    break;}
case 106:
#line 650 "bc.y"
{ warn ("Too many end of lines"); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 542 "/usr/gnu/share/bison.simple"

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
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 653 "bc.y"


