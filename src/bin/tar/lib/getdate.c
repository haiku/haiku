
/*  A Bison parser, made from getdate.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	tAGO	257
#define	tDST	258
#define	tDAY	259
#define	tDAY_UNIT	260
#define	tDAYZONE	261
#define	tHOUR_UNIT	262
#define	tLOCAL_ZONE	263
#define	tMERIDIAN	264
#define	tMINUTE_UNIT	265
#define	tMONTH	266
#define	tMONTH_UNIT	267
#define	tSEC_UNIT	268
#define	tYEAR_UNIT	269
#define	tZONE	270
#define	tSNUMBER	271
#define	tUNUMBER	272

#line 1 "getdate.y"

/* Parse a string into an internal time stamp.
   Copyright (C) 1999, 2000, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Originally written by Steven M. Bellovin <smb@research.att.com> while
   at the University of North Carolina at Chapel Hill.  Later tweaked by
   a couple of people on Usenet.  Completely overhauled by Rich $alz
   <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990.

   Modified by Paul Eggert <eggert@twinsun.com> in August 1999 to do
   the right thing about local DST.  Unlike previous versions, this
   version is reentrant.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <alloca.h>

/* Since the code of getdate.y is not included in the Emacs executable
   itself, there is no need to #define static in this file.  Even if
   the code were included in the Emacs executable, it probably
   wouldn't do any harm to #undef it here; this will only cause
   problems if we try to write to a static variable, which I don't
   think this code needs to do.  */
#ifdef emacs
# undef static
#endif

#include <ctype.h>

#include <stdlib.h> /* for `free'; used by Bison 1.27 */

#if STDC_HEADERS || (! defined isascii && ! HAVE_ISASCII)
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii (c)
#endif

#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))
#define ISDIGIT_LOCALE(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))

/* ISDIGIT differs from ISDIGIT_LOCALE, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   ISDIGIT_LOCALE unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

#include <string.h>

#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
# define __attribute__(x)
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

#define EPOCH_YEAR 1970
#define TM_YEAR_BASE 1900

#define HOUR(x) ((x) * 60)

/* An integer value, and the number of digits in its textual
   representation.  */
typedef struct
{
  int value;
  int digits;
} textint;

/* An entry in the lexical lookup table.  */
typedef struct
{
  char const *name;
  int type;
  int value;
} table;

/* Meridian: am, pm, or 24-hour style.  */
enum { MERam, MERpm, MER24 };

/* Information passed to and from the parser.  */
typedef struct
{
  /* The input string remaining to be parsed. */
  const char *input;

  /* N, if this is the Nth Tuesday.  */
  int day_ordinal;

  /* Day of week; Sunday is 0.  */
  int day_number;

  /* tm_isdst flag for the local zone.  */
  int local_isdst;

  /* Time zone, in minutes east of UTC.  */
  int time_zone;

  /* Style used for time.  */
  int meridian;

  /* Gregorian year, month, day, hour, minutes, and seconds.  */
  textint year;
  int month;
  int day;
  int hour;
  int minutes;
  int seconds;

  /* Relative year, month, day, hour, minutes, and seconds.  */
  int rel_year;
  int rel_month;
  int rel_day;
  int rel_hour;
  int rel_minutes;
  int rel_seconds;

  /* Counts of nonterminals of various flavors parsed so far.  */
  int dates_seen;
  int days_seen;
  int local_zones_seen;
  int rels_seen;
  int times_seen;
  int zones_seen;

  /* Table of local time zone abbrevations, terminated by a null entry.  */
  table local_time_zone_table[3];
} parser_control;

#define PC (* (parser_control *) parm)
#define YYLEX_PARAM parm
#define YYPARSE_PARAM parm

static int yyerror ();
static int yylex ();


#line 166 "getdate.y"
typedef union
{
  int intval;
  textint textintval;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		66
#define	YYFLAG		-32768
#define	YYNTBASE	22

#define YYTRANSLATE(x) ((unsigned)(x) <= 272 ? yytranslate[x] : 33)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    20,     2,     2,    21,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    19,     2,     2,
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
    17,    18
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,     8,    10,    12,    14,    16,    18,
    21,    26,    31,    38,    45,    47,    50,    52,    54,    57,
    59,    62,    65,    69,    75,    79,    83,    87,    90,    95,
    98,   102,   105,   107,   110,   113,   115,   118,   121,   123,
   126,   129,   131,   134,   137,   139,   142,   145,   147,   150,
   153,   155,   157,   158
};

static const short yyrhs[] = {    -1,
    22,    23,     0,    24,     0,    25,     0,    26,     0,    28,
     0,    27,     0,    29,     0,    31,     0,    18,    10,     0,
    18,    19,    18,    32,     0,    18,    19,    18,    17,     0,
    18,    19,    18,    19,    18,    32,     0,    18,    19,    18,
    19,    18,    17,     0,     9,     0,     9,     4,     0,    16,
     0,     7,     0,    16,     4,     0,     5,     0,     5,    20,
     0,    18,     5,     0,    18,    21,    18,     0,    18,    21,
    18,    21,    18,     0,    18,    17,    17,     0,    18,    12,
    17,     0,    12,    17,    17,     0,    12,    18,     0,    12,
    18,    20,    18,     0,    18,    12,     0,    18,    12,    18,
     0,    30,     3,     0,    30,     0,    18,    15,     0,    17,
    15,     0,    15,     0,    18,    13,     0,    17,    13,     0,
    13,     0,    18,     6,     0,    17,     6,     0,     6,     0,
    18,     8,     0,    17,     8,     0,     8,     0,    18,    11,
     0,    17,    11,     0,    11,     0,    18,    14,     0,    17,
    14,     0,    14,     0,    18,     0,     0,    10,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   183,   185,   188,   191,   193,   195,   197,   199,   201,   204,
   212,   219,   227,   234,   245,   248,   252,   255,   257,   261,
   267,   272,   279,   285,   305,   312,   320,   328,   333,   339,
   344,   352,   362,   365,   368,   370,   372,   374,   376,   378,
   380,   382,   384,   386,   388,   390,   392,   394,   396,   398,
   400,   404,   440,   443
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","tAGO","tDST",
"tDAY","tDAY_UNIT","tDAYZONE","tHOUR_UNIT","tLOCAL_ZONE","tMERIDIAN","tMINUTE_UNIT",
"tMONTH","tMONTH_UNIT","tSEC_UNIT","tYEAR_UNIT","tZONE","tSNUMBER","tUNUMBER",
"':'","','","'/'","spec","item","time","local_zone","zone","day","date","rel",
"relunit","number","o_merid", NULL
};
#endif

static const short yyr1[] = {     0,
    22,    22,    23,    23,    23,    23,    23,    23,    23,    24,
    24,    24,    24,    24,    25,    25,    26,    26,    26,    27,
    27,    27,    28,    28,    28,    28,    28,    28,    28,    28,
    28,    29,    29,    30,    30,    30,    30,    30,    30,    30,
    30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
    30,    31,    32,    32
};

static const short yyr2[] = {     0,
     0,     2,     1,     1,     1,     1,     1,     1,     1,     2,
     4,     4,     6,     6,     1,     2,     1,     1,     2,     1,
     2,     2,     3,     5,     3,     3,     3,     2,     4,     2,
     3,     2,     1,     2,     2,     1,     2,     2,     1,     2,
     2,     1,     2,     2,     1,     2,     2,     1,     2,     2,
     1,     1,     0,     1
};

static const short yydefact[] = {     1,
     0,    20,    42,    18,    45,    15,    48,     0,    39,    51,
    36,    17,     0,    52,     2,     3,     4,     5,     7,     6,
     8,    33,     9,    21,    16,     0,    28,    19,    41,    44,
    47,    38,    50,    35,    22,    40,    43,    10,    46,    30,
    37,    49,    34,     0,     0,     0,    32,    27,     0,    26,
    31,    25,    53,    23,    29,    54,    12,     0,    11,     0,
    53,    24,    14,    13,     0,     0
};

static const short yydefgoto[] = {     1,
    15,    16,    17,    18,    19,    20,    21,    22,    23,    59
};

static const short yypact[] = {-32768,
     0,     1,-32768,-32768,-32768,    19,-32768,   -16,-32768,-32768,
-32768,    32,    26,    14,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    27,-32768,-32768,-32768,   -13,    24,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    25,
-32768,-32768,-32768,    29,    30,    31,-32768,-32768,    33,-32768,
-32768,-32768,    28,    34,-32768,-32768,-32768,    35,-32768,    36,
    -7,-32768,-32768,-32768,    50,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    -9
};


#define	YYLAST		55


static const short yytable[] = {    65,
    26,    27,    56,    48,     2,     3,     4,     5,     6,    63,
     7,     8,     9,    10,    11,    12,    13,    14,    35,    36,
    24,    37,    25,    38,    39,    40,    41,    42,    43,    47,
    44,    29,    45,    30,    46,    28,    31,    56,    32,    33,
    34,    50,    51,    49,    57,    52,    58,    53,    54,    66,
    55,    64,    61,    62,    60
};

static const short yycheck[] = {     0,
    17,    18,    10,    17,     5,     6,     7,     8,     9,    17,
    11,    12,    13,    14,    15,    16,    17,    18,     5,     6,
    20,     8,     4,    10,    11,    12,    13,    14,    15,     3,
    17,     6,    19,     8,    21,     4,    11,    10,    13,    14,
    15,    17,    18,    20,    17,    17,    19,    18,    18,     0,
    18,    61,    18,    18,    21
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison.simple"
/* This file comes from bison-1.28.  */

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

#line 217 "/usr/share/bison.simple"

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

case 3:
#line 190 "getdate.y"
{ PC.times_seen++; ;
    break;}
case 4:
#line 192 "getdate.y"
{ PC.local_zones_seen++; ;
    break;}
case 5:
#line 194 "getdate.y"
{ PC.zones_seen++; ;
    break;}
case 6:
#line 196 "getdate.y"
{ PC.dates_seen++; ;
    break;}
case 7:
#line 198 "getdate.y"
{ PC.days_seen++; ;
    break;}
case 8:
#line 200 "getdate.y"
{ PC.rels_seen++; ;
    break;}
case 10:
#line 206 "getdate.y"
{
	PC.hour = yyvsp[-1].textintval.value;
	PC.minutes = 0;
	PC.seconds = 0;
	PC.meridian = yyvsp[0].intval;
      ;
    break;}
case 11:
#line 213 "getdate.y"
{
	PC.hour = yyvsp[-3].textintval.value;
	PC.minutes = yyvsp[-1].textintval.value;
	PC.seconds = 0;
	PC.meridian = yyvsp[0].intval;
      ;
    break;}
case 12:
#line 220 "getdate.y"
{
	PC.hour = yyvsp[-3].textintval.value;
	PC.minutes = yyvsp[-1].textintval.value;
	PC.meridian = MER24;
	PC.zones_seen++;
	PC.time_zone = yyvsp[0].textintval.value % 100 + (yyvsp[0].textintval.value / 100) * 60;
      ;
    break;}
case 13:
#line 228 "getdate.y"
{
	PC.hour = yyvsp[-5].textintval.value;
	PC.minutes = yyvsp[-3].textintval.value;
	PC.seconds = yyvsp[-1].textintval.value;
	PC.meridian = yyvsp[0].intval;
      ;
    break;}
case 14:
#line 235 "getdate.y"
{
	PC.hour = yyvsp[-5].textintval.value;
	PC.minutes = yyvsp[-3].textintval.value;
	PC.seconds = yyvsp[-1].textintval.value;
	PC.meridian = MER24;
	PC.zones_seen++;
	PC.time_zone = yyvsp[0].textintval.value % 100 + (yyvsp[0].textintval.value / 100) * 60;
      ;
    break;}
case 15:
#line 247 "getdate.y"
{ PC.local_isdst = yyvsp[0].intval; ;
    break;}
case 16:
#line 249 "getdate.y"
{ PC.local_isdst = yyvsp[-1].intval < 0 ? 1 : yyvsp[-1].intval + 1; ;
    break;}
case 17:
#line 254 "getdate.y"
{ PC.time_zone = yyvsp[0].intval; ;
    break;}
case 18:
#line 256 "getdate.y"
{ PC.time_zone = yyvsp[0].intval + 60; ;
    break;}
case 19:
#line 258 "getdate.y"
{ PC.time_zone = yyvsp[-1].intval + 60; ;
    break;}
case 20:
#line 263 "getdate.y"
{
	PC.day_ordinal = 1;
	PC.day_number = yyvsp[0].intval;
      ;
    break;}
case 21:
#line 268 "getdate.y"
{
	PC.day_ordinal = 1;
	PC.day_number = yyvsp[-1].intval;
      ;
    break;}
case 22:
#line 273 "getdate.y"
{
	PC.day_ordinal = yyvsp[-1].textintval.value;
	PC.day_number = yyvsp[0].intval;
      ;
    break;}
case 23:
#line 281 "getdate.y"
{
	PC.month = yyvsp[-2].textintval.value;
	PC.day = yyvsp[0].textintval.value;
      ;
    break;}
case 24:
#line 286 "getdate.y"
{
	/* Interpret as YYYY/MM/DD if the first value has 4 or more digits,
	   otherwise as MM/DD/YY.
	   The goal in recognizing YYYY/MM/DD is solely to support legacy
	   machine-generated dates like those in an RCS log listing.  If
	   you want portability, use the ISO 8601 format.  */
	if (4 <= yyvsp[-4].textintval.digits)
	  {
	    PC.year = yyvsp[-4].textintval;
	    PC.month = yyvsp[-2].textintval.value;
	    PC.day = yyvsp[0].textintval.value;
	  }
	else
	  {
	    PC.month = yyvsp[-4].textintval.value;
	    PC.day = yyvsp[-2].textintval.value;
	    PC.year = yyvsp[0].textintval;
	  }
      ;
    break;}
case 25:
#line 306 "getdate.y"
{
	/* ISO 8601 format.  YYYY-MM-DD.  */
	PC.year = yyvsp[-2].textintval;
	PC.month = -yyvsp[-1].textintval.value;
	PC.day = -yyvsp[0].textintval.value;
      ;
    break;}
case 26:
#line 313 "getdate.y"
{
	/* e.g. 17-JUN-1992.  */
	PC.day = yyvsp[-2].textintval.value;
	PC.month = yyvsp[-1].intval;
	PC.year.value = -yyvsp[0].textintval.value;
	PC.year.digits = yyvsp[0].textintval.digits;
      ;
    break;}
case 27:
#line 321 "getdate.y"
{
	/* e.g. JUN-17-1992.  */
	PC.month = yyvsp[-2].intval;
	PC.day = -yyvsp[-1].textintval.value;
	PC.year.value = -yyvsp[0].textintval.value;
	PC.year.digits = yyvsp[0].textintval.digits;
      ;
    break;}
case 28:
#line 329 "getdate.y"
{
	PC.month = yyvsp[-1].intval;
	PC.day = yyvsp[0].textintval.value;
      ;
    break;}
case 29:
#line 334 "getdate.y"
{
	PC.month = yyvsp[-3].intval;
	PC.day = yyvsp[-2].textintval.value;
	PC.year = yyvsp[0].textintval;
      ;
    break;}
case 30:
#line 340 "getdate.y"
{
	PC.day = yyvsp[-1].textintval.value;
	PC.month = yyvsp[0].intval;
      ;
    break;}
case 31:
#line 345 "getdate.y"
{
	PC.day = yyvsp[-2].textintval.value;
	PC.month = yyvsp[-1].intval;
	PC.year = yyvsp[0].textintval;
      ;
    break;}
case 32:
#line 354 "getdate.y"
{
	PC.rel_seconds = -PC.rel_seconds;
	PC.rel_minutes = -PC.rel_minutes;
	PC.rel_hour = -PC.rel_hour;
	PC.rel_day = -PC.rel_day;
	PC.rel_month = -PC.rel_month;
	PC.rel_year = -PC.rel_year;
      ;
    break;}
case 34:
#line 367 "getdate.y"
{ PC.rel_year += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 35:
#line 369 "getdate.y"
{ PC.rel_year += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 36:
#line 371 "getdate.y"
{ PC.rel_year += yyvsp[0].intval; ;
    break;}
case 37:
#line 373 "getdate.y"
{ PC.rel_month += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 38:
#line 375 "getdate.y"
{ PC.rel_month += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 39:
#line 377 "getdate.y"
{ PC.rel_month += yyvsp[0].intval; ;
    break;}
case 40:
#line 379 "getdate.y"
{ PC.rel_day += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 41:
#line 381 "getdate.y"
{ PC.rel_day += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 42:
#line 383 "getdate.y"
{ PC.rel_day += yyvsp[0].intval; ;
    break;}
case 43:
#line 385 "getdate.y"
{ PC.rel_hour += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 44:
#line 387 "getdate.y"
{ PC.rel_hour += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 45:
#line 389 "getdate.y"
{ PC.rel_hour += yyvsp[0].intval; ;
    break;}
case 46:
#line 391 "getdate.y"
{ PC.rel_minutes += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 47:
#line 393 "getdate.y"
{ PC.rel_minutes += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 48:
#line 395 "getdate.y"
{ PC.rel_minutes += yyvsp[0].intval; ;
    break;}
case 49:
#line 397 "getdate.y"
{ PC.rel_seconds += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 50:
#line 399 "getdate.y"
{ PC.rel_seconds += yyvsp[-1].textintval.value * yyvsp[0].intval; ;
    break;}
case 51:
#line 401 "getdate.y"
{ PC.rel_seconds += yyvsp[0].intval; ;
    break;}
case 52:
#line 406 "getdate.y"
{
	if (PC.dates_seen
	    && ! PC.rels_seen && (PC.times_seen || 2 < yyvsp[0].textintval.digits))
	  PC.year = yyvsp[0].textintval;
	else
	  {
	    if (4 < yyvsp[0].textintval.digits)
	      {
		PC.dates_seen++;
		PC.day = yyvsp[0].textintval.value % 100;
		PC.month = (yyvsp[0].textintval.value / 100) % 100;
		PC.year.value = yyvsp[0].textintval.value / 10000;
		PC.year.digits = yyvsp[0].textintval.digits - 4;
	      }
	    else
	      {
		PC.times_seen++;
		if (yyvsp[0].textintval.digits <= 2)
		  {
		    PC.hour = yyvsp[0].textintval.value;
		    PC.minutes = 0;
		  }
		else
		  {
		    PC.hour = yyvsp[0].textintval.value / 100;
		    PC.minutes = yyvsp[0].textintval.value % 100;
		  }
		PC.seconds = 0;
		PC.meridian = MER24;
	      }
	  }
      ;
    break;}
case 53:
#line 442 "getdate.y"
{ yyval.intval = MER24; ;
    break;}
case 54:
#line 444 "getdate.y"
{ yyval.intval = yyvsp[0].intval; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/share/bison.simple"

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
#line 447 "getdate.y"


/* Include this file down here because bison inserts code above which
   may define-away `const'.  We want the prototype for get_date to have
   the same signature as the function definition.  */
#include "getdate.h"
#include "unlocked-io.h"

#ifndef gmtime
struct tm *gmtime ();
#endif
#ifndef localtime
struct tm *localtime ();
#endif
#ifndef mktime
time_t mktime ();
#endif

static table const meridian_table[] =
{
  { "AM",   tMERIDIAN, MERam },
  { "A.M.", tMERIDIAN, MERam },
  { "PM",   tMERIDIAN, MERpm },
  { "P.M.", tMERIDIAN, MERpm },
  { 0, 0, 0 }
};

static table const dst_table[] =
{
  { "DST", tDST, 0 }
};

static table const month_and_day_table[] =
{
  { "JANUARY",	tMONTH,	 1 },
  { "FEBRUARY",	tMONTH,	 2 },
  { "MARCH",	tMONTH,	 3 },
  { "APRIL",	tMONTH,	 4 },
  { "MAY",	tMONTH,	 5 },
  { "JUNE",	tMONTH,	 6 },
  { "JULY",	tMONTH,	 7 },
  { "AUGUST",	tMONTH,	 8 },
  { "SEPTEMBER",tMONTH,	 9 },
  { "SEPT",	tMONTH,	 9 },
  { "OCTOBER",	tMONTH,	10 },
  { "NOVEMBER",	tMONTH,	11 },
  { "DECEMBER",	tMONTH,	12 },
  { "SUNDAY",	tDAY,	 0 },
  { "MONDAY",	tDAY,	 1 },
  { "TUESDAY",	tDAY,	 2 },
  { "TUES",	tDAY,	 2 },
  { "WEDNESDAY",tDAY,	 3 },
  { "WEDNES",	tDAY,	 3 },
  { "THURSDAY",	tDAY,	 4 },
  { "THUR",	tDAY,	 4 },
  { "THURS",	tDAY,	 4 },
  { "FRIDAY",	tDAY,	 5 },
  { "SATURDAY",	tDAY,	 6 },
  { 0, 0, 0 }
};

static table const time_units_table[] =
{
  { "YEAR",	tYEAR_UNIT,	 1 },
  { "MONTH",	tMONTH_UNIT,	 1 },
  { "FORTNIGHT",tDAY_UNIT,	14 },
  { "WEEK",	tDAY_UNIT,	 7 },
  { "DAY",	tDAY_UNIT,	 1 },
  { "HOUR",	tHOUR_UNIT,	 1 },
  { "MINUTE",	tMINUTE_UNIT,	 1 },
  { "MIN",	tMINUTE_UNIT,	 1 },
  { "SECOND",	tSEC_UNIT,	 1 },
  { "SEC",	tSEC_UNIT,	 1 },
  { 0, 0, 0 }
};

/* Assorted relative-time words. */
static table const relative_time_table[] =
{
  { "TOMORROW",	tDAY_UNIT,	 1 },
  { "YESTERDAY",tDAY_UNIT,	-1 },
  { "TODAY",	tDAY_UNIT,	 0 },
  { "NOW",	tDAY_UNIT,	 0 },
  { "LAST",	tUNUMBER,	-1 },
  { "THIS",	tUNUMBER,	 0 },
  { "NEXT",	tUNUMBER,	 1 },
  { "FIRST",	tUNUMBER,	 1 },
/*{ "SECOND",	tUNUMBER,	 2 }, */
  { "THIRD",	tUNUMBER,	 3 },
  { "FOURTH",	tUNUMBER,	 4 },
  { "FIFTH",	tUNUMBER,	 5 },
  { "SIXTH",	tUNUMBER,	 6 },
  { "SEVENTH",	tUNUMBER,	 7 },
  { "EIGHTH",	tUNUMBER,	 8 },
  { "NINTH",	tUNUMBER,	 9 },
  { "TENTH",	tUNUMBER,	10 },
  { "ELEVENTH",	tUNUMBER,	11 },
  { "TWELFTH",	tUNUMBER,	12 },
  { "AGO",	tAGO,		 1 },
  { 0, 0, 0 }
};

/* The time zone table.  This table is necessarily incomplete, as time
   zone abbreviations are ambiguous; e.g. Australians interpret "EST"
   as Eastern time in Australia, not as US Eastern Standard Time.
   You cannot rely on getdate to handle arbitrary time zone
   abbreviations; use numeric abbreviations like `-0500' instead.  */
static table const time_zone_table[] =
{
  { "GMT",	tZONE,     HOUR ( 0) },	/* Greenwich Mean */
  { "UT",	tZONE,     HOUR ( 0) },	/* Universal (Coordinated) */
  { "UTC",	tZONE,     HOUR ( 0) },
  { "WET",	tZONE,     HOUR ( 0) },	/* Western European */
  { "WEST",	tDAYZONE,  HOUR ( 0) },	/* Western European Summer */
  { "BST",	tDAYZONE,  HOUR ( 0) },	/* British Summer */
  { "ART",	tZONE,	  -HOUR ( 3) },	/* Argentina */
  { "BRT",	tZONE,	  -HOUR ( 3) },	/* Brazil */
  { "BRST",	tDAYZONE, -HOUR ( 3) },	/* Brazil Summer */
  { "NST",	tZONE,	 -(HOUR ( 3) + 30) },	/* Newfoundland Standard */
  { "NDT",	tDAYZONE,-(HOUR ( 3) + 30) },	/* Newfoundland Daylight */
  { "AST",	tZONE,    -HOUR ( 4) },	/* Atlantic Standard */
  { "ADT",	tDAYZONE, -HOUR ( 4) },	/* Atlantic Daylight */
  { "CLT",	tZONE,    -HOUR ( 4) },	/* Chile */
  { "CLST",	tDAYZONE, -HOUR ( 4) },	/* Chile Summer */
  { "EST",	tZONE,    -HOUR ( 5) },	/* Eastern Standard */
  { "EDT",	tDAYZONE, -HOUR ( 5) },	/* Eastern Daylight */
  { "CST",	tZONE,    -HOUR ( 6) },	/* Central Standard */
  { "CDT",	tDAYZONE, -HOUR ( 6) },	/* Central Daylight */
  { "MST",	tZONE,    -HOUR ( 7) },	/* Mountain Standard */
  { "MDT",	tDAYZONE, -HOUR ( 7) },	/* Mountain Daylight */
  { "PST",	tZONE,    -HOUR ( 8) },	/* Pacific Standard */
  { "PDT",	tDAYZONE, -HOUR ( 8) },	/* Pacific Daylight */
  { "AKST",	tZONE,    -HOUR ( 9) },	/* Alaska Standard */
  { "AKDT",	tDAYZONE, -HOUR ( 9) },	/* Alaska Daylight */
  { "HST",	tZONE,    -HOUR (10) },	/* Hawaii Standard */
  { "HAST",	tZONE,	  -HOUR (10) },	/* Hawaii-Aleutian Standard */
  { "HADT",	tDAYZONE, -HOUR (10) },	/* Hawaii-Aleutian Daylight */
  { "SST",	tZONE,    -HOUR (12) },	/* Samoa Standard */
  { "WAT",	tZONE,     HOUR ( 1) },	/* West Africa */
  { "CET",	tZONE,     HOUR ( 1) },	/* Central European */
  { "CEST",	tDAYZONE,  HOUR ( 1) },	/* Central European Summer */
  { "MET",	tZONE,     HOUR ( 1) },	/* Middle European */
  { "MEZ",	tZONE,     HOUR ( 1) },	/* Middle European */
  { "MEST",	tDAYZONE,  HOUR ( 1) },	/* Middle European Summer */
  { "MESZ",	tDAYZONE,  HOUR ( 1) },	/* Middle European Summer */
  { "EET",	tZONE,     HOUR ( 2) },	/* Eastern European */
  { "EEST",	tDAYZONE,  HOUR ( 2) },	/* Eastern European Summer */
  { "CAT",	tZONE,	   HOUR ( 2) },	/* Central Africa */
  { "SAST",	tZONE,	   HOUR ( 2) },	/* South Africa Standard */
  { "EAT",	tZONE,	   HOUR ( 3) },	/* East Africa */
  { "MSK",	tZONE,	   HOUR ( 3) },	/* Moscow */
  { "MSD",	tDAYZONE,  HOUR ( 3) },	/* Moscow Daylight */
  { "IST",	tZONE,	  (HOUR ( 5) + 30) },	/* India Standard */
  { "SGT",	tZONE,     HOUR ( 8) },	/* Singapore */
  { "KST",	tZONE,     HOUR ( 9) },	/* Korea Standard */
  { "JST",	tZONE,     HOUR ( 9) },	/* Japan Standard */
  { "GST",	tZONE,     HOUR (10) },	/* Guam Standard */
  { "NZST",	tZONE,     HOUR (12) },	/* New Zealand Standard */
  { "NZDT",	tDAYZONE,  HOUR (12) },	/* New Zealand Daylight */
  { 0, 0, 0  }
};

/* Military time zone table. */
static table const military_table[] =
{
  { "A", tZONE,	-HOUR ( 1) },
  { "B", tZONE,	-HOUR ( 2) },
  { "C", tZONE,	-HOUR ( 3) },
  { "D", tZONE,	-HOUR ( 4) },
  { "E", tZONE,	-HOUR ( 5) },
  { "F", tZONE,	-HOUR ( 6) },
  { "G", tZONE,	-HOUR ( 7) },
  { "H", tZONE,	-HOUR ( 8) },
  { "I", tZONE,	-HOUR ( 9) },
  { "K", tZONE,	-HOUR (10) },
  { "L", tZONE,	-HOUR (11) },
  { "M", tZONE,	-HOUR (12) },
  { "N", tZONE,	 HOUR ( 1) },
  { "O", tZONE,	 HOUR ( 2) },
  { "P", tZONE,	 HOUR ( 3) },
  { "Q", tZONE,	 HOUR ( 4) },
  { "R", tZONE,	 HOUR ( 5) },
  { "S", tZONE,	 HOUR ( 6) },
  { "T", tZONE,	 HOUR ( 7) },
  { "U", tZONE,	 HOUR ( 8) },
  { "V", tZONE,	 HOUR ( 9) },
  { "W", tZONE,	 HOUR (10) },
  { "X", tZONE,	 HOUR (11) },
  { "Y", tZONE,	 HOUR (12) },
  { "Z", tZONE,	 HOUR ( 0) },
  { 0, 0, 0 }
};



static int
to_hour (int hours, int meridian)
{
  switch (meridian)
    {
    case MER24:
      return 0 <= hours && hours < 24 ? hours : -1;
    case MERam:
      return 0 < hours && hours < 12 ? hours : hours == 12 ? 0 : -1;
    case MERpm:
      return 0 < hours && hours < 12 ? hours + 12 : hours == 12 ? 12 : -1;
    default:
      abort ();
    }
  /* NOTREACHED */
}

static int
to_year (textint textyear)
{
  int year = textyear.value;

  if (year < 0)
    year = -year;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  if (textyear.digits == 2)
    year += year < 69 ? 2000 : 1900;

  return year;
}

static table const *
lookup_zone (parser_control const *pc, char const *name)
{
  table const *tp;

  /* Try local zone abbreviations first; they're more likely to be right.  */
  for (tp = pc->local_time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  for (tp = time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  return 0;
}

#if ! HAVE_TM_GMTOFF
/* Yield the difference between *A and *B,
   measured in seconds, ignoring leap seconds.
   The body of this function is taken directly from the GNU C Library;
   see src/strftime.c.  */
static int
tm_diff (struct tm const *a, struct tm const *b)
{
  /* Compute intervening leap days correctly even if year is negative.
     Take care to avoid int overflow in leap day calculations,
     but it's OK to assume that A and B are close to each other.  */
  int a4 = (a->tm_year >> 2) + (TM_YEAR_BASE >> 2) - ! (a->tm_year & 3);
  int b4 = (b->tm_year >> 2) + (TM_YEAR_BASE >> 2) - ! (b->tm_year & 3);
  int a100 = a4 / 25 - (a4 % 25 < 0);
  int b100 = b4 / 25 - (b4 % 25 < 0);
  int a400 = a100 >> 2;
  int b400 = b100 >> 2;
  int intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
  int years = a->tm_year - b->tm_year;
  int days = (365 * years + intervening_leap_days
	      + (a->tm_yday - b->tm_yday));
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
		+ (a->tm_min - b->tm_min))
	  + (a->tm_sec - b->tm_sec));
}
#endif /* ! HAVE_TM_GMTOFF */

static table const *
lookup_word (parser_control const *pc, char *word)
{
  char *p;
  char *q;
  size_t wordlen;
  table const *tp;
  int i;
  int abbrev;

  /* Make it uppercase.  */
  for (p = word; *p; p++)
    if (ISLOWER ((unsigned char) *p))
      *p = toupper ((unsigned char) *p);

  for (tp = meridian_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* See if we have an abbreviation for a month. */
  wordlen = strlen (word);
  abbrev = wordlen == 3 || (wordlen == 4 && word[3] == '.');

  for (tp = month_and_day_table; tp->name; tp++)
    if ((abbrev ? strncmp (word, tp->name, 3) : strcmp (word, tp->name)) == 0)
      return tp;

  if ((tp = lookup_zone (pc, word)))
    return tp;

  if (strcmp (word, dst_table[0].name) == 0)
    return dst_table;

  for (tp = time_units_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Strip off any plural and try the units table again. */
  if (word[wordlen - 1] == 'S')
    {
      word[wordlen - 1] = '\0';
      for (tp = time_units_table; tp->name; tp++)
	if (strcmp (word, tp->name) == 0)
	  return tp;
      word[wordlen - 1] = 'S';	/* For "this" in relative_time_table.  */
    }

  for (tp = relative_time_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Military time zones. */
  if (wordlen == 1)
    for (tp = military_table; tp->name; tp++)
      if (word[0] == tp->name[0])
	return tp;

  /* Drop out any periods and try the time zone table again. */
  for (i = 0, p = q = word; (*p = *q); q++)
    if (*q == '.')
      i = 1;
    else
      p++;
  if (i && (tp = lookup_zone (pc, word)))
    return tp;

  return 0;
}

static int
yylex (YYSTYPE *lvalp, parser_control *pc)
{
  unsigned char c;
  int count;

  for (;;)
    {
      while (c = *pc->input, ISSPACE (c))
	pc->input++;

      if (ISDIGIT (c) || c == '-' || c == '+')
	{
	  char const *p;
	  int sign;
	  int value;
	  if (c == '-' || c == '+')
	    {
	      sign = c == '-' ? -1 : 1;
	      c = *++pc->input;
	      if (! ISDIGIT (c))
		/* skip the '-' sign */
		continue;
	    }
	  else
	    sign = 0;
	  p = pc->input;
	  value = 0;
	  do
	    {
	      value = 10 * value + c - '0';
	      c = *++p;
	    }
	  while (ISDIGIT (c));
	  lvalp->textintval.value = sign < 0 ? -value : value;
	  lvalp->textintval.digits = p - pc->input;
	  pc->input = p;
	  return sign ? tSNUMBER : tUNUMBER;
	}

      if (ISALPHA (c))
	{
	  char buff[20];
	  char *p = buff;
	  table const *tp;

	  do
	    {
	      if (p < buff + sizeof buff - 1)
		*p++ = c;
	      c = *++pc->input;
	    }
	  while (ISALPHA (c) || c == '.');

	  *p = '\0';
	  tp = lookup_word (pc, buff);
	  if (! tp)
	    return '?';
	  lvalp->intval = tp->value;
	  return tp->type;
	}

      if (c != '(')
	return *pc->input++;
      count = 0;
      do
	{
	  c = *pc->input++;
	  if (c == '\0')
	    return c;
	  if (c == '(')
	    count++;
	  else if (c == ')')
	    count--;
	}
      while (count > 0);
    }
}

/* Do nothing if the parser reports an error.  */
static int
yyerror (char *s ATTRIBUTE_UNUSED)
{
  return 0;
}

/* Parse a date/time string P.  Return the corresponding time_t value,
   or (time_t) -1 if there is an error.  P can be an incomplete or
   relative time specification; if so, use *NOW as the basis for the
   returned time.  */
time_t
get_date (const char *p, const time_t *now)
{
  time_t Start = now ? *now : time (0);
  struct tm *tmp = localtime (&Start);
  struct tm tm;
  struct tm tm0;
  parser_control pc;

  if (! tmp)
    return -1;

  pc.input = p;
  pc.year.value = tmp->tm_year + TM_YEAR_BASE;
  pc.year.digits = 4;
  pc.month = tmp->tm_mon + 1;
  pc.day = tmp->tm_mday;
  pc.hour = tmp->tm_hour;
  pc.minutes = tmp->tm_min;
  pc.seconds = tmp->tm_sec;
  tm.tm_isdst = tmp->tm_isdst;

  pc.meridian = MER24;
  pc.rel_seconds = 0;
  pc.rel_minutes = 0;
  pc.rel_hour = 0;
  pc.rel_day = 0;
  pc.rel_month = 0;
  pc.rel_year = 0;
  pc.dates_seen = 0;
  pc.days_seen = 0;
  pc.rels_seen = 0;
  pc.times_seen = 0;
  pc.local_zones_seen = 0;
  pc.zones_seen = 0;

#if HAVE_STRUCT_TM_TM_ZONE
  pc.local_time_zone_table[0].name = tmp->tm_zone;
  pc.local_time_zone_table[0].type = tLOCAL_ZONE;
  pc.local_time_zone_table[0].value = tmp->tm_isdst;
  pc.local_time_zone_table[1].name = 0;

  /* Probe the names used in the next three calendar quarters, looking
     for a tm_isdst different from the one we already have.  */
  {
    int quarter;
    for (quarter = 1; quarter <= 3; quarter++)
      {
	time_t probe = Start + quarter * (90 * 24 * 60 * 60);
	struct tm *probe_tm = localtime (&probe);
	if (probe_tm && probe_tm->tm_zone
	    && probe_tm->tm_isdst != pc.local_time_zone_table[0].value)
	  {
	      {
		pc.local_time_zone_table[1].name = probe_tm->tm_zone;
		pc.local_time_zone_table[1].type = tLOCAL_ZONE;
		pc.local_time_zone_table[1].value = probe_tm->tm_isdst;
		pc.local_time_zone_table[2].name = 0;
	      }
	    break;
	  }
      }
  }
#else
#if HAVE_TZNAME
  {
# ifndef tzname
    extern char *tzname[];
# endif
    int i;
    for (i = 0; i < 2; i++)
      {
	pc.local_time_zone_table[i].name = tzname[i];
	pc.local_time_zone_table[i].type = tLOCAL_ZONE;
	pc.local_time_zone_table[i].value = i;
      }
    pc.local_time_zone_table[i].name = 0;
  }
#else
  pc.local_time_zone_table[0].name = 0;
#endif
#endif

  if (pc.local_time_zone_table[0].name && pc.local_time_zone_table[1].name
      && ! strcmp (pc.local_time_zone_table[0].name,
		   pc.local_time_zone_table[1].name))
    {
      /* This locale uses the same abbrevation for standard and
	 daylight times.  So if we see that abbreviation, we don't
	 know whether it's daylight time.  */
      pc.local_time_zone_table[0].value = -1;
      pc.local_time_zone_table[1].name = 0;
    }

  if (yyparse (&pc) != 0
      || 1 < pc.times_seen || 1 < pc.dates_seen || 1 < pc.days_seen
      || 1 < (pc.local_zones_seen + pc.zones_seen)
      || (pc.local_zones_seen && 1 < pc.local_isdst))
    return -1;

  tm.tm_year = to_year (pc.year) - TM_YEAR_BASE + pc.rel_year;
  tm.tm_mon = pc.month - 1 + pc.rel_month;
  tm.tm_mday = pc.day + pc.rel_day;
  if (pc.times_seen || (pc.rels_seen && ! pc.dates_seen && ! pc.days_seen))
    {
      tm.tm_hour = to_hour (pc.hour, pc.meridian);
      if (tm.tm_hour < 0)
	return -1;
      tm.tm_min = pc.minutes;
      tm.tm_sec = pc.seconds;
    }
  else
    {
      tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    }

  /* Let mktime deduce tm_isdst if we have an absolute time stamp,
     or if the relative time stamp mentions days, months, or years.  */
  if (pc.dates_seen | pc.days_seen | pc.times_seen | pc.rel_day
      | pc.rel_month | pc.rel_year)
    tm.tm_isdst = -1;

  /* But if the input explicitly specifies local time with or without
     DST, give mktime that information.  */
  if (pc.local_zones_seen)
    tm.tm_isdst = pc.local_isdst;

  tm0 = tm;

  Start = mktime (&tm);

  if (Start == (time_t) -1)
    {

      /* Guard against falsely reporting errors near the time_t boundaries
         when parsing times in other time zones.  For example, if the min
         time_t value is 1970-01-01 00:00:00 UTC and we are 8 hours ahead
         of UTC, then the min localtime value is 1970-01-01 08:00:00; if
         we apply mktime to 1970-01-01 00:00:00 we will get an error, so
         we apply mktime to 1970-01-02 08:00:00 instead and adjust the time
         zone by 24 hours to compensate.  This algorithm assumes that
         there is no DST transition within a day of the time_t boundaries.  */
      if (pc.zones_seen)
	{
	  tm = tm0;
	  if (tm.tm_year <= EPOCH_YEAR - TM_YEAR_BASE)
	    {
	      tm.tm_mday++;
	      pc.time_zone += 24 * 60;
	    }
	  else
	    {
	      tm.tm_mday--;
	      pc.time_zone -= 24 * 60;
	    }
	  Start = mktime (&tm);
	}

      if (Start == (time_t) -1)
	return Start;
    }

  if (pc.days_seen && ! pc.dates_seen)
    {
      tm.tm_mday += ((pc.day_number - tm.tm_wday + 7) % 7
		     + 7 * (pc.day_ordinal - (0 < pc.day_ordinal)));
      tm.tm_isdst = -1;
      Start = mktime (&tm);
      if (Start == (time_t) -1)
	return Start;
    }

  if (pc.zones_seen)
    {
      int delta = pc.time_zone * 60;
#ifdef HAVE_TM_GMTOFF
      delta -= tm.tm_gmtoff;
#else
      struct tm *gmt = gmtime (&Start);
      if (! gmt)
	return -1;
      delta -= tm_diff (&tm, gmt);
#endif
      if ((Start < Start - delta) != (delta < 0))
	return -1;	/* time_t overflow */
      Start -= delta;
    }

  /* Add relative hours, minutes, and seconds.  Ignore leap seconds;
     i.e. "+ 10 minutes" means 600 seconds, even if one of them is a
     leap second.  Typically this is not what the user wants, but it's
     too hard to do it the other way, because the time zone indicator
     must be applied before relative times, and if mktime is applied
     again the time zone will be lost.  */
  {
    time_t t0 = Start;
    long d1 = 60 * 60 * (long) pc.rel_hour;
    time_t t1 = t0 + d1;
    long d2 = 60 * (long) pc.rel_minutes;
    time_t t2 = t1 + d2;
    int d3 = pc.rel_seconds;
    time_t t3 = t2 + d3;
    if ((d1 / (60 * 60) ^ pc.rel_hour)
	| (d2 / 60 ^ pc.rel_minutes)
	| ((t0 + d1 < t0) ^ (d1 < 0))
	| ((t1 + d2 < t1) ^ (d2 < 0))
	| ((t2 + d3 < t2) ^ (d3 < 0)))
      return -1;
    Start = t3;
  }

  return Start;
}

#if TEST

#include <stdio.h>

int
main (int ac, char **av)
{
  char buff[BUFSIZ];
  time_t d;

  printf ("Enter date, or blank line to exit.\n\t> ");
  fflush (stdout);

  buff[BUFSIZ - 1] = 0;
  while (fgets (buff, BUFSIZ - 1, stdin) && buff[0])
    {
      d = get_date (buff, 0);
      if (d == (time_t) -1)
	printf ("Bad format - couldn't convert.\n");
      else
	printf ("%s", ctime (&d));
      printf ("\t> ");
      fflush (stdout);
    }
  return 0;
}
#endif /* defined TEST */
