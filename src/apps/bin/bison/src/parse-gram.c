/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse gram_parse
#define yylex   gram_lex
#define yyerror gram_error
#define yylval  gram_lval
#define yychar  gram_char
#define yydebug gram_debug
#define yynerrs gram_nerrs
#define yylloc gram_lloc

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     GRAM_EOF = 0,
     STRING = 258,
     INT = 259,
     PERCENT_TOKEN = 260,
     PERCENT_NTERM = 261,
     PERCENT_TYPE = 262,
     PERCENT_DESTRUCTOR = 263,
     PERCENT_PRINTER = 264,
     PERCENT_UNION = 265,
     PERCENT_LEFT = 266,
     PERCENT_RIGHT = 267,
     PERCENT_NONASSOC = 268,
     PERCENT_PREC = 269,
     PERCENT_DPREC = 270,
     PERCENT_MERGE = 271,
     PERCENT_DEBUG = 272,
     PERCENT_DEFAULT_PREC = 273,
     PERCENT_DEFINE = 274,
     PERCENT_DEFINES = 275,
     PERCENT_ERROR_VERBOSE = 276,
     PERCENT_EXPECT = 277,
     PERCENT_EXPECT_RR = 278,
     PERCENT_FILE_PREFIX = 279,
     PERCENT_GLR_PARSER = 280,
     PERCENT_INITIAL_ACTION = 281,
     PERCENT_LEX_PARAM = 282,
     PERCENT_LOCATIONS = 283,
     PERCENT_NAME_PREFIX = 284,
     PERCENT_NO_DEFAULT_PREC = 285,
     PERCENT_NO_LINES = 286,
     PERCENT_NONDETERMINISTIC_PARSER = 287,
     PERCENT_OUTPUT = 288,
     PERCENT_PARSE_PARAM = 289,
     PERCENT_PURE_PARSER = 290,
     PERCENT_SKELETON = 291,
     PERCENT_START = 292,
     PERCENT_TOKEN_TABLE = 293,
     PERCENT_VERBOSE = 294,
     PERCENT_YACC = 295,
     TYPE = 296,
     EQUAL = 297,
     SEMICOLON = 298,
     PIPE = 299,
     ID = 300,
     ID_COLON = 301,
     PERCENT_PERCENT = 302,
     PROLOGUE = 303,
     EPILOGUE = 304,
     BRACED_CODE = 305
   };
#endif
#define GRAM_EOF 0
#define STRING 258
#define INT 259
#define PERCENT_TOKEN 260
#define PERCENT_NTERM 261
#define PERCENT_TYPE 262
#define PERCENT_DESTRUCTOR 263
#define PERCENT_PRINTER 264
#define PERCENT_UNION 265
#define PERCENT_LEFT 266
#define PERCENT_RIGHT 267
#define PERCENT_NONASSOC 268
#define PERCENT_PREC 269
#define PERCENT_DPREC 270
#define PERCENT_MERGE 271
#define PERCENT_DEBUG 272
#define PERCENT_DEFAULT_PREC 273
#define PERCENT_DEFINE 274
#define PERCENT_DEFINES 275
#define PERCENT_ERROR_VERBOSE 276
#define PERCENT_EXPECT 277
#define PERCENT_EXPECT_RR 278
#define PERCENT_FILE_PREFIX 279
#define PERCENT_GLR_PARSER 280
#define PERCENT_INITIAL_ACTION 281
#define PERCENT_LEX_PARAM 282
#define PERCENT_LOCATIONS 283
#define PERCENT_NAME_PREFIX 284
#define PERCENT_NO_DEFAULT_PREC 285
#define PERCENT_NO_LINES 286
#define PERCENT_NONDETERMINISTIC_PARSER 287
#define PERCENT_OUTPUT 288
#define PERCENT_PARSE_PARAM 289
#define PERCENT_PURE_PARSER 290
#define PERCENT_SKELETON 291
#define PERCENT_START 292
#define PERCENT_TOKEN_TABLE 293
#define PERCENT_VERBOSE 294
#define PERCENT_YACC 295
#define TYPE 296
#define EQUAL 297
#define SEMICOLON 298
#define PIPE 299
#define ID 300
#define ID_COLON 301
#define PERCENT_PERCENT 302
#define PROLOGUE 303
#define EPILOGUE 304
#define BRACED_CODE 305




/* Copy the first part of user declarations.  */
#line 1 "parse-gram.y"
/* Bison Grammar Parser                             -*- C -*-

   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307  USA
*/

#include "system.h"

#include "complain.h"
#include "conflicts.h"
#include "files.h"
#include "getargs.h"
#include "gram.h"
#include "muscle_tab.h"
#include "output.h"
#include "reader.h"
#include "symlist.h"

#define YYLLOC_DEFAULT(Current, Rhs, N)  (Current) = lloc_default (Rhs, N)
static YYLTYPE lloc_default (YYLTYPE const *, int);

/* Request detailed syntax error messages, and pass them to GRAM_ERROR.
   FIXME: depends on the undocumented availability of YYLLOC.  */
#undef  yyerror
#define yyerror(Msg) \
        gram_error (&yylloc, Msg)
static void gram_error (location const *, char const *);

#define YYPRINT(File, Type, Value) \
	print_token_value (File, Type, &Value)
static void print_token_value (FILE *, int, YYSTYPE const *);

static void add_param (char const *, char *, location);

symbol_class current_class = unknown_sym;
uniqstr current_type = 0;
symbol *current_lhs;
location current_lhs_location;
assoc current_assoc;
int current_prec = 0;


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 78 "parse-gram.y"
typedef union YYSTYPE {
  symbol *symbol;
  symbol_list *list;
  int integer;
  char *chars;
  assoc assoc;
  uniqstr uniqstr;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 254 "parse-gram.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined (YYLTYPE) && ! defined (YYLTYPE_IS_DECLARED)
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 278 "parse-gram.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYLTYPE_IS_TRIVIAL) && YYLTYPE_IS_TRIVIAL \
             && defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   158

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  25
/* YYNRULES -- Number of rules. */
#define YYNRULES  79
/* YYNRULES -- Number of states. */
#define YYNSTATES  108

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   305

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     8,     9,    12,    14,    16,    18,    22,
      24,    26,    29,    32,    36,    38,    40,    42,    44,    48,
      50,    52,    56,    58,    60,    63,    65,    67,    69,    71,
      73,    75,    78,    80,    83,    86,    88,    90,    91,    95,
      96,   100,   104,   108,   110,   112,   114,   115,   117,   119,
     122,   124,   126,   129,   132,   136,   138,   141,   143,   146,
     148,   151,   154,   155,   159,   161,   165,   168,   169,   172,
     175,   179,   183,   187,   189,   191,   193,   195,   197,   198
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      52,     0,    -1,    53,    47,    65,    75,    -1,    -1,    53,
      54,    -1,    55,    -1,    48,    -1,    17,    -1,    19,    74,
      74,    -1,    20,    -1,    21,    -1,    22,     4,    -1,    23,
       4,    -1,    24,    42,    74,    -1,    25,    -1,    26,    -1,
      27,    -1,    28,    -1,    29,    42,    74,    -1,    31,    -1,
      32,    -1,    33,    42,    74,    -1,    34,    -1,    35,    -1,
      36,    74,    -1,    38,    -1,    39,    -1,    40,    -1,    43,
      -1,    59,    -1,    56,    -1,    37,    71,    -1,    10,    -1,
       8,    62,    -1,     9,    62,    -1,    18,    -1,    30,    -1,
      -1,     6,    57,    64,    -1,    -1,     5,    58,    64,    -1,
       7,    41,    62,    -1,    60,    61,    62,    -1,    11,    -1,
      12,    -1,    13,    -1,    -1,    41,    -1,    71,    -1,    62,
      71,    -1,    41,    -1,    45,    -1,    45,     4,    -1,    45,
      73,    -1,    45,     4,    73,    -1,    63,    -1,    64,    63,
      -1,    66,    -1,    65,    66,    -1,    67,    -1,    55,    43,
      -1,     1,    43,    -1,    -1,    46,    68,    69,    -1,    70,
      -1,    69,    44,    70,    -1,    69,    43,    -1,    -1,    70,
      71,    -1,    70,    72,    -1,    70,    14,    71,    -1,    70,
      15,     4,    -1,    70,    16,    41,    -1,    45,    -1,    73,
      -1,    50,    -1,     3,    -1,     3,    -1,    -1,    47,    49,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   171,   171,   179,   181,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   199,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   219,
     220,   221,   225,   231,   238,   245,   249,   256,   256,   261,
     261,   266,   276,   291,   292,   293,   297,   298,   304,   305,
     310,   314,   319,   325,   331,   342,   343,   352,   353,   359,
     360,   365,   372,   372,   376,   377,   378,   383,   384,   386,
     388,   390,   392,   397,   398,   402,   408,   417,   424,   426
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "$undefined", "\"string\"", "\"integer\"",
  "\"%token\"", "\"%nterm\"", "\"%type\"", "\"%destructor {...}\"",
  "\"%printer {...}\"", "\"%union {...}\"", "\"%left\"", "\"%right\"",
  "\"%nonassoc\"", "\"%prec\"", "\"%dprec\"", "\"%merge\"", "\"%debug\"",
  "\"%default-prec\"", "\"%define\"", "\"%defines\"", "\"%error-verbose\"",
  "\"%expect\"", "\"%expect-rr\"", "\"%file-prefix\"", "\"%glr-parser\"",
  "\"%initial-action {...}\"", "\"%lex-param {...}\"", "\"%locations\"",
  "\"%name-prefix\"", "\"%no-default-prec\"", "\"%no-lines\"",
  "\"%nondeterministic-parser\"", "\"%output\"", "\"%parse-param {...}\"",
  "\"%pure-parser\"", "\"%skeleton\"", "\"%start\"", "\"%token-table\"",
  "\"%verbose\"", "\"%yacc\"", "\"type\"", "\"=\"", "\";\"", "\"|\"",
  "\"identifier\"", "\"identifier:\"", "\"%%\"", "\"%{...%}\"",
  "\"epilogue\"", "\"{...}\"", "$accept", "input", "declarations",
  "declaration", "grammar_declaration", "symbol_declaration", "@1", "@2",
  "precedence_declaration", "precedence_declarator", "type.opt",
  "symbols.1", "symbol_def", "symbol_defs.1", "grammar",
  "rules_or_grammar_declaration", "rules", "@3", "rhses.1", "rhs",
  "symbol", "action", "string_as_id", "string_content", "epilogue.opt", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    51,    52,    53,    53,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    55,
      55,    55,    55,    55,    55,    55,    55,    57,    56,    58,
      56,    56,    59,    60,    60,    60,    61,    61,    62,    62,
      63,    63,    63,    63,    63,    64,    64,    65,    65,    66,
      66,    66,    68,    67,    69,    69,    69,    70,    70,    70,
      70,    70,    70,    71,    71,    72,    73,    74,    75,    75
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     4,     0,     2,     1,     1,     1,     3,     1,
       1,     2,     2,     3,     1,     1,     1,     1,     3,     1,
       1,     3,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     2,     1,     2,     2,     1,     1,     0,     3,     0,
       3,     3,     3,     1,     1,     1,     0,     1,     1,     2,
       1,     1,     2,     2,     3,     1,     2,     1,     2,     1,
       2,     2,     0,     3,     1,     3,     2,     0,     2,     2,
       3,     3,     3,     1,     1,     1,     1,     1,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     0,     1,    39,    37,     0,     0,     0,    32,
      43,    44,    45,     7,    35,     0,     9,    10,     0,     0,
       0,    14,    15,    16,    17,     0,    36,    19,    20,     0,
      22,    23,     0,     0,    25,    26,    27,    28,     0,     6,
       4,     5,    30,    29,    46,     0,     0,     0,    76,    73,
      33,    48,    74,    34,    77,     0,    11,    12,     0,     0,
       0,    24,    31,     0,    62,     0,     0,    57,    59,    47,
       0,    50,    51,    55,    40,    38,    41,    49,     8,    13,
      18,    21,    61,    67,    60,     0,    58,     2,    42,    52,
      53,    56,    63,    64,    79,    54,    66,    67,     0,     0,
       0,    75,    68,    69,    65,    70,    71,    72
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     2,    40,    65,    42,    46,    45,    43,    44,
      70,    50,    73,    74,    66,    67,    68,    83,    92,    93,
      51,   103,    52,    55,    87
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -67
static const yysigned_char yypact[] =
{
     -67,     5,   110,   -67,   -67,   -67,   -34,     0,     0,   -67,
     -67,   -67,   -67,   -67,   -67,    13,   -67,   -67,    20,    31,
     -17,   -67,   -67,   -67,   -67,    -6,   -67,   -67,   -67,    -5,
     -67,   -67,    13,     0,   -67,   -67,   -67,   -67,    68,   -67,
     -67,   -67,   -67,   -67,    -3,   -37,   -37,     0,   -67,   -67,
       0,   -67,   -67,     0,   -67,    13,   -67,   -67,    13,    13,
      13,   -67,   -67,    -2,   -67,     3,    21,   -67,   -67,   -67,
       0,   -67,     6,   -67,   -37,   -37,     0,   -67,   -67,   -67,
     -67,   -67,   -67,   -67,   -67,     1,   -67,   -67,     0,    39,
     -67,   -67,   -32,    -1,   -67,   -67,   -67,   -67,     0,    43,
       7,   -67,   -67,   -67,    -1,   -67,   -67,   -67
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -67,   -67,   -67,   -67,    50,   -67,   -67,   -67,   -67,   -67,
     -67,    -7,   -56,     8,   -67,   -13,   -67,   -67,   -67,   -41,
     -33,   -67,   -66,    29,   -67
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -79
static const yysigned_char yytable[] =
{
      62,    53,    48,    48,    71,     3,    90,    47,    72,    48,
      89,    96,    97,    98,    99,   100,    54,    77,    91,    91,
      77,   -78,    63,    95,    56,    58,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    57,    59,    60,    69,    14,
      76,    82,    48,    77,    49,    49,    84,   106,   107,   101,
      94,    26,    41,    86,    75,    77,   104,     0,    33,     0,
     102,    61,     0,    88,     0,   105,     0,    64,    85,    63,
       0,   102,     0,     4,     5,     6,     7,     8,     9,    10,
      11,    12,     0,     0,    78,     0,    14,    79,    80,    81,
       0,     0,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,    33,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     4,     5,     6,     7,     8,
       9,    10,    11,    12,     0,     0,     0,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,     0,     0,    37,     0,     0,     0,    38,    39
};

static const yysigned_char yycheck[] =
{
      33,     8,     3,     3,    41,     0,    72,    41,    45,     3,
       4,    43,    44,    14,    15,    16,     3,    50,    74,    75,
      53,     0,     1,    89,     4,    42,     5,     6,     7,     8,
       9,    10,    11,    12,    13,     4,    42,    42,    41,    18,
      47,    43,     3,    76,    45,    45,    43,     4,    41,    50,
      49,    30,     2,    66,    46,    88,    97,    -1,    37,    -1,
      93,    32,    -1,    70,    -1,    98,    -1,    46,    47,     1,
      -1,   104,    -1,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    -1,    -1,    55,    -1,    18,    58,    59,    60,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    43,    -1,    -1,    -1,    47,    48
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    52,    53,     0,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    43,    47,    48,
      54,    55,    56,    59,    60,    58,    57,    41,     3,    45,
      62,    71,    73,    62,     3,    74,     4,     4,    42,    42,
      42,    74,    71,     1,    46,    55,    65,    66,    67,    41,
      61,    41,    45,    63,    64,    64,    62,    71,    74,    74,
      74,    74,    43,    68,    43,    47,    66,    75,    62,     4,
      73,    63,    69,    70,    49,    73,    43,    44,    14,    15,
      16,    50,    71,    72,    70,    71,     4,    41
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value, Location);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
  (void) yylocationp;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yytype, yyvaluep, yylocationp)
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
  (void) yylocationp;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  YYLTYPE *yylerrsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

  /* User initialization code. */
  #line 68 "parse-gram.y"
{
  /* Bison's grammar can initial empty locations, hence a default
     location is needed. */
  (*yylsp).start.file   = (*yylsp).end.file   = current_file;
  (*yylsp).start.line   = (*yylsp).end.line   = 1;
  (*yylsp).start.column = (*yylsp).end.column = 0;
}
/* Line 818 of yacc.c.  */
#line 1118 "parse-gram.c"
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
  *++yylsp = yylloc;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, yylsp - yylen, yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 6:
#line 186 "parse-gram.y"
    { prologue_augment (yyvsp[0].chars, yylsp[0]); }
    break;

  case 7:
#line 187 "parse-gram.y"
    { debug_flag = true; }
    break;

  case 8:
#line 188 "parse-gram.y"
    { muscle_insert (yyvsp[-1].chars, yyvsp[0].chars); }
    break;

  case 9:
#line 189 "parse-gram.y"
    { defines_flag = true; }
    break;

  case 10:
#line 190 "parse-gram.y"
    { error_verbose = true; }
    break;

  case 11:
#line 191 "parse-gram.y"
    { expected_sr_conflicts = yyvsp[0].integer; }
    break;

  case 12:
#line 192 "parse-gram.y"
    { expected_rr_conflicts = yyvsp[0].integer; }
    break;

  case 13:
#line 193 "parse-gram.y"
    { spec_file_prefix = yyvsp[0].chars; }
    break;

  case 14:
#line 195 "parse-gram.y"
    {
    nondeterministic_parser = true;
    glr_parser = true;
  }
    break;

  case 15:
#line 200 "parse-gram.y"
    {
    muscle_code_grow ("initial_action", yyvsp[0].chars, yylsp[0]);
  }
    break;

  case 16:
#line 203 "parse-gram.y"
    { add_param ("lex_param", yyvsp[0].chars, yylsp[0]); }
    break;

  case 17:
#line 204 "parse-gram.y"
    { locations_flag = true; }
    break;

  case 18:
#line 205 "parse-gram.y"
    { spec_name_prefix = yyvsp[0].chars; }
    break;

  case 19:
#line 206 "parse-gram.y"
    { no_lines_flag = true; }
    break;

  case 20:
#line 207 "parse-gram.y"
    { nondeterministic_parser = true; }
    break;

  case 21:
#line 208 "parse-gram.y"
    { spec_outfile = yyvsp[0].chars; }
    break;

  case 22:
#line 209 "parse-gram.y"
    { add_param ("parse_param", yyvsp[0].chars, yylsp[0]); }
    break;

  case 23:
#line 210 "parse-gram.y"
    { pure_parser = true; }
    break;

  case 24:
#line 211 "parse-gram.y"
    { skeleton = yyvsp[0].chars; }
    break;

  case 25:
#line 212 "parse-gram.y"
    { token_table_flag = true; }
    break;

  case 26:
#line 213 "parse-gram.y"
    { report_flag = report_states; }
    break;

  case 27:
#line 214 "parse-gram.y"
    { yacc_flag = true; }
    break;

  case 31:
#line 222 "parse-gram.y"
    {
      grammar_start_symbol_set (yyvsp[0].symbol, yylsp[0]);
    }
    break;

  case 32:
#line 226 "parse-gram.y"
    {
      typed = true;
      MUSCLE_INSERT_INT ("stype_line", yylsp[0].start.line);
      muscle_insert ("stype", yyvsp[0].chars);
    }
    break;

  case 33:
#line 232 "parse-gram.y"
    {
      symbol_list *list;
      for (list = yyvsp[0].list; list; list = list->next)
	symbol_destructor_set (list->sym, yyvsp[-1].chars, yylsp[-1]);
      symbol_list_free (yyvsp[0].list);
    }
    break;

  case 34:
#line 239 "parse-gram.y"
    {
      symbol_list *list;
      for (list = yyvsp[0].list; list; list = list->next)
	symbol_printer_set (list->sym, yyvsp[-1].chars, list->location);
      symbol_list_free (yyvsp[0].list);
    }
    break;

  case 35:
#line 246 "parse-gram.y"
    {
      default_prec = true;
    }
    break;

  case 36:
#line 250 "parse-gram.y"
    {
      default_prec = false;
    }
    break;

  case 37:
#line 256 "parse-gram.y"
    { current_class = nterm_sym; }
    break;

  case 38:
#line 257 "parse-gram.y"
    {
      current_class = unknown_sym;
      current_type = NULL;
    }
    break;

  case 39:
#line 261 "parse-gram.y"
    { current_class = token_sym; }
    break;

  case 40:
#line 262 "parse-gram.y"
    {
      current_class = unknown_sym;
      current_type = NULL;
    }
    break;

  case 41:
#line 267 "parse-gram.y"
    {
      symbol_list *list;
      for (list = yyvsp[0].list; list; list = list->next)
	symbol_type_set (list->sym, yyvsp[-1].uniqstr, yylsp[-1]);
      symbol_list_free (yyvsp[0].list);
    }
    break;

  case 42:
#line 277 "parse-gram.y"
    {
      symbol_list *list;
      ++current_prec;
      for (list = yyvsp[0].list; list; list = list->next)
	{
	  symbol_type_set (list->sym, current_type, yylsp[-1]);
	  symbol_precedence_set (list->sym, current_prec, yyvsp[-2].assoc, yylsp[-2]);
	}
      symbol_list_free (yyvsp[0].list);
      current_type = NULL;
    }
    break;

  case 43:
#line 291 "parse-gram.y"
    { yyval.assoc = left_assoc; }
    break;

  case 44:
#line 292 "parse-gram.y"
    { yyval.assoc = right_assoc; }
    break;

  case 45:
#line 293 "parse-gram.y"
    { yyval.assoc = non_assoc; }
    break;

  case 46:
#line 297 "parse-gram.y"
    { current_type = NULL; }
    break;

  case 47:
#line 298 "parse-gram.y"
    { current_type = yyvsp[0].uniqstr; }
    break;

  case 48:
#line 304 "parse-gram.y"
    { yyval.list = symbol_list_new (yyvsp[0].symbol, yylsp[0]); }
    break;

  case 49:
#line 305 "parse-gram.y"
    { yyval.list = symbol_list_prepend (yyvsp[-1].list, yyvsp[0].symbol, yylsp[0]); }
    break;

  case 50:
#line 311 "parse-gram.y"
    {
       current_type = yyvsp[0].uniqstr;
     }
    break;

  case 51:
#line 315 "parse-gram.y"
    {
       symbol_class_set (yyvsp[0].symbol, current_class, yylsp[0]);
       symbol_type_set (yyvsp[0].symbol, current_type, yylsp[0]);
     }
    break;

  case 52:
#line 320 "parse-gram.y"
    {
      symbol_class_set (yyvsp[-1].symbol, current_class, yylsp[-1]);
      symbol_type_set (yyvsp[-1].symbol, current_type, yylsp[-1]);
      symbol_user_token_number_set (yyvsp[-1].symbol, yyvsp[0].integer, yylsp[0]);
    }
    break;

  case 53:
#line 326 "parse-gram.y"
    {
      symbol_class_set (yyvsp[-1].symbol, current_class, yylsp[-1]);
      symbol_type_set (yyvsp[-1].symbol, current_type, yylsp[-1]);
      symbol_make_alias (yyvsp[-1].symbol, yyvsp[0].symbol, yyloc);
    }
    break;

  case 54:
#line 332 "parse-gram.y"
    {
      symbol_class_set (yyvsp[-2].symbol, current_class, yylsp[-2]);
      symbol_type_set (yyvsp[-2].symbol, current_type, yylsp[-2]);
      symbol_user_token_number_set (yyvsp[-2].symbol, yyvsp[-1].integer, yylsp[-1]);
      symbol_make_alias (yyvsp[-2].symbol, yyvsp[0].symbol, yyloc);
    }
    break;

  case 60:
#line 361 "parse-gram.y"
    {
      if (yacc_flag)
	complain_at (yyloc, _("POSIX forbids declarations in the grammar"));
    }
    break;

  case 61:
#line 366 "parse-gram.y"
    {
      yyerrok;
    }
    break;

  case 62:
#line 372 "parse-gram.y"
    { current_lhs = yyvsp[0].symbol; current_lhs_location = yylsp[0]; }
    break;

  case 64:
#line 376 "parse-gram.y"
    { grammar_rule_end (yylsp[0]); }
    break;

  case 65:
#line 377 "parse-gram.y"
    { grammar_rule_end (yylsp[0]); }
    break;

  case 67:
#line 383 "parse-gram.y"
    { grammar_rule_begin (current_lhs, current_lhs_location); }
    break;

  case 68:
#line 385 "parse-gram.y"
    { grammar_current_rule_symbol_append (yyvsp[0].symbol, yylsp[0]); }
    break;

  case 69:
#line 387 "parse-gram.y"
    { grammar_current_rule_action_append (yyvsp[0].chars, yylsp[0]); }
    break;

  case 70:
#line 389 "parse-gram.y"
    { grammar_current_rule_prec_set (yyvsp[0].symbol, yylsp[0]); }
    break;

  case 71:
#line 391 "parse-gram.y"
    { grammar_current_rule_dprec_set (yyvsp[0].integer, yylsp[0]); }
    break;

  case 72:
#line 393 "parse-gram.y"
    { grammar_current_rule_merge_set (yyvsp[0].uniqstr, yylsp[0]); }
    break;

  case 73:
#line 397 "parse-gram.y"
    { yyval.symbol = yyvsp[0].symbol; }
    break;

  case 74:
#line 398 "parse-gram.y"
    { yyval.symbol = yyvsp[0].symbol; }
    break;

  case 75:
#line 403 "parse-gram.y"
    { yyval.chars = yyvsp[0].chars; }
    break;

  case 76:
#line 409 "parse-gram.y"
    {
      yyval.symbol = symbol_get (yyvsp[0].chars, yylsp[0]);
      symbol_class_set (yyval.symbol, token_sym, yylsp[0]);
    }
    break;

  case 77:
#line 418 "parse-gram.y"
    {
      yyval.chars = yyvsp[0].chars + 1;
      yyval.chars[strlen (yyval.chars) - 1] = '\0';
    }
    break;

  case 79:
#line 427 "parse-gram.y"
    {
      muscle_code_grow ("epilogue", yyvsp[0].chars, yylsp[0]);
      scanner_last_string_free ();
    }
    break;


    }

/* Line 1010 of yacc.c.  */
#line 1702 "parse-gram.c"

  yyvsp -= yylen;
  yyssp -= yylen;
  yylsp -= yylen;

  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }

  yylerrsp = yylsp;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp, yylsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	  *++yylerrsp = yylloc;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  yylerrsp = yylsp;
  *++yylerrsp = yyloc;
  yylsp -= yylen;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
  YYLLOC_DEFAULT (yyloc, yylsp, yylerrsp - yylsp);
  *++yylsp = yyloc;

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 433 "parse-gram.y"



/* Return the location of the left-hand side of a rule whose
   right-hand side is RHS[1] ... RHS[N].  Ignore empty nonterminals in
   the right-hand side, and return an empty location equal to the end
   boundary of RHS[0] if the right-hand side is empty.  */

static YYLTYPE
lloc_default (YYLTYPE const *rhs, int n)
{
  int i;
  YYLTYPE loc;
  loc.start = loc.end = rhs[n].end;

  /* Ignore empty nonterminals the start of the the right-hand side.
     Do not bother to ignore them at the end of the right-hand side,
     since empty nonterminals have the same end as their predecessors.  */
  for (i = 1; i <= n; i++)
    if (! equal_boundaries (rhs[i].start, rhs[i].end))
      {
	loc.start = rhs[i].start;
	break;
      }

  return loc;
}


/* Add a lex-param or a parse-param (depending on TYPE) with
   declaration DECL and location LOC.  */

static void
add_param (char const *type, char *decl, location loc)
{
  static char const alphanum[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_";
  char const *alpha = alphanum + 10;
  char const *name_start = NULL;
  char *p;

  for (p = decl; *p; p++)
    if ((p == decl || ! strchr (alphanum, p[-1])) && strchr (alpha, p[0]))
      name_start = p;

  /* Strip the surrounding '{' and '}'.  */
  decl++;
  p[-1] = '\0';

  if (! name_start)
    complain_at (loc, _("missing identifier in parameter declaration"));
  else
    {
      char *name;
      size_t name_len;

      for (name_len = 1;
	   name_start[name_len] && strchr (alphanum, name_start[name_len]);
	   name_len++)
	continue;

      name = xmalloc (name_len + 1);
      memcpy (name, name_start, name_len);
      name[name_len] = '\0';
      muscle_pair_list_grow (type, decl, name);
      free (name);
    }

  scanner_last_string_free ();
}

/*----------------------------------------------------.
| When debugging the parser, display tokens' values.  |
`----------------------------------------------------*/

static void
print_token_value (FILE *file, int type, YYSTYPE const *value)
{
  fputc (' ', file);
  switch (type)
    {
    case ID:
      fprintf (file, " = %s", value->symbol->tag);
      break;

    case INT:
      fprintf (file, " = %d", value->integer);
      break;

    case STRING:
      fprintf (file, " = \"%s\"", value->chars);
      break;

    case TYPE:
      fprintf (file, " = <%s>", value->uniqstr);
      break;

    case BRACED_CODE:
    case PERCENT_DESTRUCTOR:
    case PERCENT_LEX_PARAM:
    case PERCENT_PARSE_PARAM:
    case PERCENT_PRINTER:
    case PERCENT_UNION:
    case PROLOGUE:
    case EPILOGUE:
      fprintf (file, " = {{ %s }}", value->chars);
      break;

    default:
      fprintf (file, "unknown token type");
      break;
    }
}

static void
gram_error (location const *loc, char const *msg)
{
  complain_at (*loc, "%s", msg);
}

char const *
token_name (int type)
{
  return yytname[type];
}

