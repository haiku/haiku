/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 23 "testpatterny.y"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "testpattern.h"

extern int mylineno;

extern int yylex(void);
char *quotestrip(const char *i);
char *endstrip(const char *i);

extern char* yytext;

static int yyerror( const char *s )
{
	fprintf(stderr,"stdin:%d: %s before '%s'\n",mylineno,s,yytext);
	return 0;
}

typedef struct
{
  const char *name;
  int channel;
} color_t;

static color_t color_map[] =
  {
    { "black", 0 },
    { "cyan", 1 },
    { "red", 1 },
    { "magenta", 2 },
    { "green", 2 },
    { "yellow", 3 },
    { "blue", 3 },
    { "l_black", 4 },
    { "l_cyan", 5 },
    { "l_magenta", 6 },
    { "d_yellow", 4 },
    { "l_l_black", 7 },
    { NULL, -1 }
  };

static int current_index = 0;
static testpattern_t *current_testpattern;
extern FILE *yyin;

static int
find_color(const char *name)
{
  int i = 0;
  while (color_map[i].name)
    {
      if (strcasecmp(color_map[i].name, name) == 0)
	return color_map[i].channel;
      i++;
    }
  return -1;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 259 "testpatterny.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  60
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   166

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  71
/* YYNRULES -- Number of rules.  */
#define YYNRULES  120
/* YYNRULES -- Number of states.  */
#define YYNSTATES  172

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   298

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
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
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    22,    24,    26,    28,    30,    32,    34,    36,    39,
      41,    43,    46,    50,    54,    58,    62,    65,    68,    71,
      74,    77,    81,    83,    85,    89,    93,    97,   101,   105,
     107,   109,   111,   113,   115,   118,   121,   124,   127,   130,
     133,   136,   140,   142,   145,   146,   148,   151,   156,   162,
     164,   166,   168,   171,   172,   174,   176,   178,   184,   188,
     191,   194,   198,   200,   201,   204,   207,   209,   212,   214,
     216,   218,   220,   222,   224,   226,   228,   230,   232,   234,
     236,   238,   240,   242,   244,   246,   248,   250,   252,   254,
     256,   258,   260,   262,   264,   267,   269,   271,   273,   275,
     278,   279,   282,   284,   285,   288,   290,   292,   293,   296,
     297
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
     113,     0,    -1,     4,    -1,     3,    -1,    31,    -1,    32,
      -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,    26,
       3,    -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,
      50,    -1,    51,    -1,    52,    -1,    53,     3,    -1,    53,
      -1,    54,    -1,    37,    55,    -1,     8,     6,    45,    -1,
       8,     3,    45,    -1,     7,     6,    45,    -1,     7,     3,
      45,    -1,     7,    45,    -1,     9,     3,    -1,    10,    45,
      -1,    11,     5,    -1,    38,     5,    -1,    38,     3,     3,
      -1,    65,    -1,    66,    -1,    12,     5,     5,    -1,    13,
       5,     3,    -1,    14,     5,     3,    -1,    15,     5,    45,
      -1,    16,     5,     5,    -1,    68,    -1,    69,    -1,    71,
      -1,    72,    -1,    70,    -1,    17,    45,    -1,    18,    45,
      -1,    19,    45,    -1,    20,    45,    -1,    21,    45,    -1,
      22,     3,    -1,    23,     3,    -1,    45,    45,    45,    -1,
      81,    -1,    82,    81,    -1,    -1,    82,    -1,    81,    83,
      -1,     6,    45,    45,    45,    -1,    30,     3,    45,    45,
      45,    -1,    85,    -1,    86,    -1,    87,    -1,    88,    87,
      -1,    -1,    88,    -1,    84,    -1,    89,    -1,    45,    45,
      45,    45,    45,    -1,    24,    91,    90,    -1,    25,    90,
      -1,    28,     3,    -1,    27,     3,     3,    -1,     5,    -1,
      -1,    97,    96,    -1,    39,    97,    -1,    40,    -1,    40,
       5,    -1,    99,    -1,   100,    -1,    41,    -1,    42,    -1,
      59,    -1,    60,    -1,    57,    -1,    58,    -1,    61,    -1,
      62,    -1,    63,    -1,    64,    -1,    73,    -1,    74,    -1,
      75,    -1,    76,    -1,    77,    -1,    78,    -1,    79,    -1,
      80,    -1,    56,    -1,    67,    -1,    98,    -1,   101,    -1,
     102,    -1,   103,    -1,   104,    29,    -1,    92,    -1,    93,
      -1,    94,    -1,    98,    -1,   106,    29,    -1,    -1,   108,
     107,    -1,    95,    -1,    -1,   110,   105,    -1,   108,    -1,
     109,    -1,    -1,    43,    29,    -1,    -1,   110,   114,   111,
     112,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   139,   139,   139,   144,   154,   164,   174,   184,   194,
     204,   214,   214,   214,   214,   214,   214,   214,   217,   225,
     225,   228,   231,   241,   250,   260,   269,   276,   283,   290,
     299,   308,   317,   317,   320,   330,   339,   348,   357,   371,
     371,   371,   371,   371,   373,   380,   387,   394,   401,   408,
     416,   424,   439,   439,   442,   442,   445,   448,   462,   475,
     475,   478,   478,   481,   481,   484,   484,   487,   502,   505,
     520,   531,   548,   555,   555,   558,   561,   571,   577,   577,
     580,   584,   588,   588,   588,   588,   588,   588,   589,   589,
     589,   589,   589,   589,   589,   590,   590,   590,   590,   590,
     590,   591,   591,   591,   594,   598,   598,   598,   598,   601,
     605,   605,   608,   612,   612,   615,   615,   618,   618,   623,
     622
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tINT", "tDOUBLE", "tSTRING", "COLOR",
  "GAMMA", "LEVEL", "STEPS", "INK_LIMIT", "PRINTER", "PARAMETER",
  "PARAMETER_INT", "PARAMETER_BOOL", "PARAMETER_FLOAT", "PARAMETER_CURVE",
  "DENSITY", "TOP", "LEFT", "HSIZE", "VSIZE", "BLACKLINE", "NOSCALE",
  "PATTERN", "XPATTERN", "EXTENDED", "IMAGE", "GRID", "SEMI", "CHANNEL",
  "CMYK", "KCMY", "RGB", "CMY", "GRAY", "WHITE", "MODE", "PAGESIZE",
  "MESSAGE", "OUTPUT", "START_JOB", "END_JOB", "END", "$accept", "NUMBER",
  "cmykspec", "kcmyspec", "rgbspec", "cmyspec", "grayspec", "whitespec",
  "extendedspec", "modespec1", "modespec2", "modespec", "inputspec",
  "level", "channel_level", "gamma", "channel_gamma", "global_gamma",
  "steps", "ink_limit", "printer", "page_size_name", "page_size_custom",
  "page_size", "parameter_string", "parameter_int", "parameter_bool",
  "parameter_float", "parameter_curve", "parameter", "density", "top",
  "left", "hsize", "vsize", "blackline", "noscale", "color_block1",
  "color_blocks1a", "color_blocks1b", "color_blocks1", "color_block2a",
  "color_block2b", "color_block2", "color_blocks2a", "color_blocks2",
  "color_blocks", "patvars", "pattern", "xpattern", "grid", "image",
  "Message", "Messages", "message", "Output0", "Output1", "output",
  "start_job", "end_job", "A_Rule", "Rule", "A_Pattern", "Pattern",
  "Patterns", "Image", "Rules", "Print", "EOF", "Thing", "@1", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    44,    45,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    53,    53,    53,    53,    53,    53,    54,    55,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    67,    68,    69,    70,    71,    72,    73,
      73,    73,    73,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    82,    83,    83,    84,    85,    86,    87,
      87,    88,    88,    89,    89,    90,    90,    91,    92,    93,
      94,    95,    96,    97,    97,    98,    99,   100,   101,   101,
     102,   103,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   105,   106,   106,   106,   106,   107,
     108,   108,   109,   110,   110,   111,   111,   112,   112,   114,
     113
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       1,     2,     3,     3,     3,     3,     2,     2,     2,     2,
       2,     3,     1,     1,     3,     3,     3,     3,     3,     1,
       1,     1,     1,     1,     2,     2,     2,     2,     2,     2,
       2,     3,     1,     2,     0,     1,     2,     4,     5,     1,
       1,     1,     2,     0,     1,     1,     1,     5,     3,     2,
       2,     3,     1,     0,     2,     2,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     2,
       0,     2,     1,     0,     2,     1,     1,     0,     2,     0,
       4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
     113,   119,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,    76,    80,    81,    98,    84,    85,    82,
      83,    86,    87,    88,    89,    32,    33,    99,    39,    40,
      43,    41,    42,    90,    91,    92,    93,    94,    95,    96,
      97,   100,    78,    79,   101,   102,   103,     0,   114,   110,
       1,     3,     2,     0,    26,     0,     0,    27,     3,    28,
      29,     0,     0,     0,     0,     0,    44,    45,    46,    47,
      48,    49,    50,     0,     4,     5,     6,     7,     8,     9,
      11,    12,    13,    14,    15,    16,    17,    19,    20,    21,
       0,    30,    75,    77,   104,     0,   112,   115,   116,   117,
      25,    24,    23,    22,    34,    35,    36,    37,    38,    10,
      18,    31,    72,    74,     0,     0,    63,     0,   105,   106,
     107,   108,     0,   111,     0,   120,    71,     0,    63,     0,
       0,     0,    54,    65,    59,    60,    61,    64,    66,    69,
      70,   109,   118,     0,    68,     0,     0,     0,    52,    55,
      56,    62,     0,     0,     0,    51,    53,     0,    57,     0,
      67,    58
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   141,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,   142,   159,   160,
     143,   144,   145,   146,   147,   148,   149,   138,   128,   129,
     130,   106,   123,   102,    51,    52,    53,    54,    55,    56,
      57,    58,   132,   133,   107,   108,     1,   109,   135,     2,
      59
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -109
static const yytype_int8 yypact[] =
{
    -109,     8,     7,    -2,    36,    37,    32,    38,    39,    49,
      51,    52,    56,    32,    32,    32,    32,    32,    67,    69,
      33,    50,  -109,    68,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,    45,  -109,    48,
    -109,    32,  -109,    32,  -109,    32,    32,  -109,  -109,  -109,
    -109,    71,    74,    75,    32,    76,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,    77,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,    79,  -109,  -109,
      80,  -109,    81,  -109,  -109,    82,  -109,    13,  -109,    41,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,    84,    32,     2,    85,  -109,  -109,
    -109,  -109,    60,  -109,    61,  -109,  -109,    32,     2,    32,
      88,    32,    32,  -109,  -109,  -109,  -109,     3,  -109,  -109,
    -109,  -109,  -109,    32,  -109,    32,    32,    32,  -109,    32,
    -109,  -109,    32,    32,    32,  -109,  -109,    32,  -109,    32,
    -109,  -109
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -109,    -3,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -108,  -109,  -109,
    -109,  -109,  -109,   -68,  -109,  -109,   -46,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,   -14,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      64,    61,    62,    69,    63,    68,    62,    60,   139,   139,
      76,    77,    78,    79,    80,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,   140,   140,   158,    68,    62,   125,   126,    65,
      67,   127,    66,    70,    71,    20,    21,    22,    23,    24,
      25,   166,    22,   100,    72,   101,    73,    74,   110,    83,
     111,    75,   112,   113,    84,    85,    86,    87,    88,    89,
      81,   117,    82,   103,   104,   105,   114,   115,   116,   161,
     119,   118,   120,   121,   134,   124,   122,   136,   150,   151,
     152,   156,   154,   131,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   137,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   153,     0,   155,     0,   157,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     162,     0,   163,   164,   165,     0,     0,     0,     0,   167,
     168,   169,     0,     0,   170,     0,   171
};

static const yytype_int16 yycheck[] =
{
       3,     3,     4,     6,     6,     3,     4,     0,     6,     6,
      13,    14,    15,    16,    17,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    30,    30,   142,     3,     4,    24,    25,     3,
       3,    28,     6,     5,     5,    37,    38,    39,    40,    41,
      42,   159,    39,     3,     5,     5,     5,     5,    61,    26,
      63,     5,    65,    66,    31,    32,    33,    34,    35,    36,
       3,    74,     3,     5,    29,    27,     5,     3,     3,   147,
       3,     5,     3,     3,    43,     3,     5,     3,     3,    29,
      29,     3,   138,   107,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   137,    -1,   139,    -1,   141,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     153,    -1,   155,   156,   157,    -1,    -1,    -1,    -1,   162,
     163,   164,    -1,    -1,   167,    -1,   169
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   110,   113,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      37,    38,    39,    40,    41,    42,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    98,    99,   100,   101,   102,   103,   104,   105,   114,
       0,     3,     4,     6,    45,     3,     6,     3,     3,    45,
       5,     5,     5,     5,     5,     5,    45,    45,    45,    45,
      45,     3,     3,    26,    31,    32,    33,    34,    35,    36,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       3,     5,    97,     5,    29,    27,    95,   108,   109,   111,
      45,    45,    45,    45,     5,     3,     3,    45,     5,     3,
       3,     3,     5,    96,     3,    24,    25,    28,    92,    93,
      94,    98,   106,   107,    43,   112,     3,    45,    91,     6,
      30,    45,    81,    84,    85,    86,    87,    88,    89,    90,
       3,    29,    29,    45,    90,    45,     3,    45,    81,    82,
      83,    87,    45,    45,    45,    45,    81,    45,    45,    45,
      45,    45
};

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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 140 "testpatterny.y"
    {
	}
    break;

  case 4:
#line 145 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>cmykspec\n");
	  global_image_type = "CMYK";
	  global_channel_depth = 4;
	  global_invert_data = 0;
	}
    break;

  case 5:
#line 155 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>kcmyspec\n");
	  global_image_type = "KCMY";
	  global_channel_depth = 4;
	  global_invert_data = 0;
	}
    break;

  case 6:
#line 165 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>rgbspec\n");
	  global_image_type = "RGB";
	  global_channel_depth = 3;
	  global_invert_data = 1;
	}
    break;

  case 7:
#line 175 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>cmyspec\n");
	  global_image_type = "CMY";
	  global_channel_depth = 3;
	  global_invert_data = 0;
	}
    break;

  case 8:
#line 185 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>grayspec\n");
	  global_image_type = "Grayscale";
	  global_channel_depth = 1;
	  global_invert_data = 0;
	}
    break;

  case 9:
#line 195 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>whitespec\n");
	  global_image_type = "Whitescale";
	  global_channel_depth = 1;
	  global_invert_data = 1;
	}
    break;

  case 10:
#line 205 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>extendedspec %d\n", (yyvsp[(2) - (2)].ival));
	  global_image_type = "Raw";
	  global_invert_data = 0;
	  global_channel_depth = (yyvsp[(2) - (2)].ival);
	}
    break;

  case 18:
#line 218 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>modespec2 %d\n", (yyvsp[(2) - (2)].ival));
	  if ((yyvsp[(2) - (2)].ival) == 8 || (yyvsp[(2) - (2)].ival) == 16)
	    global_bit_depth = (yyvsp[(2) - (2)].ival);
	}
    break;

  case 22:
#line 232 "testpatterny.y"
    {
	  int channel = find_color((yyvsp[(2) - (3)].sval));
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>level %s %f\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].dval));
	  if (channel >= 0)
	    global_levels[channel] = (yyvsp[(3) - (3)].dval);
	}
    break;

  case 23:
#line 242 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>channel_level %d %f\n", (yyvsp[(2) - (3)].ival), (yyvsp[(3) - (3)].dval));
	  if ((yyvsp[(2) - (3)].ival) >= 0 && (yyvsp[(2) - (3)].ival) <= STP_CHANNEL_LIMIT)
	    global_levels[(yyvsp[(2) - (3)].ival)] = (yyvsp[(3) - (3)].dval);
	}
    break;

  case 24:
#line 251 "testpatterny.y"
    {
	  int channel = find_color((yyvsp[(2) - (3)].sval));
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>gamma %s %f\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].dval));
	  if (channel >= 0)
	    global_gammas[channel] = (yyvsp[(3) - (3)].dval);
	}
    break;

  case 25:
#line 261 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>channel_gamma %d %f\n", (yyvsp[(2) - (3)].ival), (yyvsp[(3) - (3)].dval));
	  if ((yyvsp[(2) - (3)].ival) >= 0 && (yyvsp[(2) - (3)].ival) <= STP_CHANNEL_LIMIT)
	    global_gammas[(yyvsp[(2) - (3)].ival)] = (yyvsp[(3) - (3)].dval);
	}
    break;

  case 26:
#line 270 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>global_gamma %f\n", (yyvsp[(2) - (2)].dval));
	  global_gamma = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 27:
#line 277 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>steps %d\n", (yyvsp[(2) - (2)].ival));
	  global_steps = (yyvsp[(2) - (2)].ival);
	}
    break;

  case 28:
#line 284 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>ink_limit %f\n", (yyvsp[(2) - (2)].dval));
	  global_ink_limit = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 29:
#line 291 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>printer %s\n", (yyvsp[(2) - (2)].sval));
	  global_printer = strdup((yyvsp[(2) - (2)].sval));
	  free((yyvsp[(2) - (2)].sval));
	}
    break;

  case 30:
#line 300 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>page_size_name %s\n", (yyvsp[(2) - (2)].sval));
	  stp_set_string_parameter(global_vars, "PageSize", (yyvsp[(2) - (2)].sval));
	  free((yyvsp[(2) - (2)].sval));
	}
    break;

  case 31:
#line 309 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>page_size_custom %d %d\n", (yyvsp[(2) - (3)].ival), (yyvsp[(3) - (3)].ival));
	  stp_set_page_width(global_vars, (yyvsp[(2) - (3)].ival));
	  stp_set_page_height(global_vars, (yyvsp[(3) - (3)].ival));
	}
    break;

  case 34:
#line 321 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>parameter_string %s %s\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].sval));
	  stp_set_string_parameter(global_vars, (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].sval));
	  free((yyvsp[(2) - (3)].sval));
	  free((yyvsp[(3) - (3)].sval));
	}
    break;

  case 35:
#line 331 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>parameter_int %s %d\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].ival));
	  stp_set_int_parameter(global_vars, (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].ival));
	  free((yyvsp[(2) - (3)].sval));
	}
    break;

  case 36:
#line 340 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>parameter_bool %s %d\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].ival));
	  stp_set_boolean_parameter(global_vars, (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].ival));
	  free((yyvsp[(2) - (3)].sval));
	}
    break;

  case 37:
#line 349 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>parameter_float %s %f\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].dval));
	  stp_set_float_parameter(global_vars, (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].dval));
	  free((yyvsp[(2) - (3)].sval));
	}
    break;

  case 38:
#line 358 "testpatterny.y"
    {
	  stp_curve_t *curve = stp_curve_create_from_string((yyvsp[(3) - (3)].sval));
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>parameter_curve %s %s\n", (yyvsp[(2) - (3)].sval), (yyvsp[(3) - (3)].sval));
	  if (curve)
	    {
	      stp_set_curve_parameter(global_vars, (yyvsp[(2) - (3)].sval), curve);
	      stp_curve_destroy(curve);
	    }
	  free((yyvsp[(2) - (3)].sval));
	}
    break;

  case 44:
#line 374 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>density %f\n", (yyvsp[(2) - (2)].dval));
	  global_density = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 45:
#line 381 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>top %f\n", (yyvsp[(2) - (2)].dval));
	  global_xtop = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 46:
#line 388 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
 	    fprintf(stderr, ">>>left %f\n", (yyvsp[(2) - (2)].dval));
	  global_xleft = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 47:
#line 395 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>hsize %f\n", (yyvsp[(2) - (2)].dval));
	  global_hsize = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 48:
#line 402 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>vsize %f\n", (yyvsp[(2) - (2)].dval));
	  global_vsize = (yyvsp[(2) - (2)].dval);
	}
    break;

  case 49:
#line 409 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>blackline %d\n", (yyvsp[(2) - (2)].ival));
	  global_noblackline = !((yyvsp[(2) - (2)].ival));
	}
    break;

  case 50:
#line 417 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>noscale %d\n", (yyvsp[(2) - (2)].ival));
	  global_noscale = (yyvsp[(2) - (2)].ival);
	}
    break;

  case 51:
#line 425 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>color_block1 %f %f %f (%d)\n", (yyvsp[(1) - (3)].dval), (yyvsp[(2) - (3)].dval), (yyvsp[(3) - (3)].dval),
		    current_index);
	  if (current_index < STP_CHANNEL_LIMIT)
	    {
	      current_testpattern->d.pattern.mins[current_index] = (yyvsp[(1) - (3)].dval);
	      current_testpattern->d.pattern.vals[current_index] = (yyvsp[(2) - (3)].dval);
	      current_testpattern->d.pattern.gammas[current_index] = (yyvsp[(3) - (3)].dval);
	      current_index++;
	    }
	}
    break;

  case 57:
#line 449 "testpatterny.y"
    {
	  int channel = find_color((yyvsp[(1) - (4)].sval));
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>color_block2a %s %f %f %f\n", (yyvsp[(1) - (4)].sval), (yyvsp[(2) - (4)].dval), (yyvsp[(3) - (4)].dval), (yyvsp[(4) - (4)].dval));
	  if (channel >= 0 && channel < STP_CHANNEL_LIMIT)
	    {
	      current_testpattern->d.pattern.mins[channel] = (yyvsp[(2) - (4)].dval);
	      current_testpattern->d.pattern.vals[channel] = (yyvsp[(3) - (4)].dval);
	      current_testpattern->d.pattern.gammas[channel] = (yyvsp[(4) - (4)].dval);
	    }
	}
    break;

  case 58:
#line 463 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>color_block2b %d %f %f %f\n", (yyvsp[(2) - (5)].ival), (yyvsp[(3) - (5)].dval), (yyvsp[(4) - (5)].dval), (yyvsp[(5) - (5)].dval));
	  if ((yyvsp[(2) - (5)].ival) >= 0 && (yyvsp[(2) - (5)].ival) < STP_CHANNEL_LIMIT)
	    {
	      current_testpattern->d.pattern.mins[(yyvsp[(2) - (5)].ival)] = (yyvsp[(3) - (5)].dval);
	      current_testpattern->d.pattern.vals[(yyvsp[(2) - (5)].ival)] = (yyvsp[(4) - (5)].dval);
	      current_testpattern->d.pattern.gammas[(yyvsp[(2) - (5)].ival)] = (yyvsp[(5) - (5)].dval);
	    }
	}
    break;

  case 67:
#line 488 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>patvars %f %f %f %f %f\n", (yyvsp[(1) - (5)].dval), (yyvsp[(2) - (5)].dval), (yyvsp[(3) - (5)].dval), (yyvsp[(4) - (5)].dval), (yyvsp[(5) - (5)].dval));
	  current_testpattern->type = E_PATTERN;
	  current_testpattern->d.pattern.lower = (yyvsp[(1) - (5)].dval);
	  current_testpattern->d.pattern.upper = (yyvsp[(2) - (5)].dval);
	  current_testpattern->d.pattern.levels[1] = (yyvsp[(3) - (5)].dval);
	  current_testpattern->d.pattern.levels[2] = (yyvsp[(4) - (5)].dval);
	  current_testpattern->d.pattern.levels[3] = (yyvsp[(5) - (5)].dval);
	  current_testpattern = get_next_testpattern();
	  current_index = 0;
	}
    break;

  case 69:
#line 506 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>xpattern\n");
	  if (global_channel_depth == 0)
	    {
	      fprintf(stderr, "xpattern may only be used with extended color depth\n");
	      exit(1);
	    }
	  current_testpattern->type = E_XPATTERN;
	  current_testpattern = get_next_testpattern();
	  current_index = 0;
	}
    break;

  case 70:
#line 521 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>grid %d\n", (yyvsp[(2) - (2)].ival));
	  current_testpattern->type = E_GRID;
	  current_testpattern->d.grid.ticks = (yyvsp[(2) - (2)].ival);
	  current_testpattern = get_next_testpattern();
	  current_index = 0;
	}
    break;

  case 71:
#line 532 "testpatterny.y"
    {
	  if (getenv("STP_TESTPATTERN_DEBUG"))
	    fprintf(stderr, ">>>image %d %d\n", (yyvsp[(2) - (3)].ival), (yyvsp[(3) - (3)].ival));
	  current_testpattern->type = E_IMAGE;
	  current_testpattern->d.image.x = (yyvsp[(2) - (3)].ival);
	  current_testpattern->d.image.y = (yyvsp[(3) - (3)].ival);
	  if (current_testpattern->d.image.x <= 0 ||
	      current_testpattern->d.image.y <= 0)
	    {
	      fprintf(stderr, "image width and height must be greater than zero\n");
	      exit(1);
	    }
	  return 0;
	}
    break;

  case 72:
#line 549 "testpatterny.y"
    {
	  fprintf(stderr,"%s",(yyvsp[(1) - (1)].sval));
	  free((yyvsp[(1) - (1)].sval));
	}
    break;

  case 76:
#line 562 "testpatterny.y"
    {
	  close_output();
	  if (global_output)
	    free(global_output);
	  global_output = NULL;
	  output = stdout;
	}
    break;

  case 77:
#line 572 "testpatterny.y"
    {
	  global_output = (yyvsp[(2) - (2)].sval);
	}
    break;

  case 80:
#line 581 "testpatterny.y"
    { start_job = 1; }
    break;

  case 81:
#line 585 "testpatterny.y"
    { end_job = 1; }
    break;

  case 104:
#line 595 "testpatterny.y"
    { global_did_something = 1; }
    break;

  case 109:
#line 602 "testpatterny.y"
    { global_did_something = 1; }
    break;

  case 112:
#line 609 "testpatterny.y"
    { global_did_something = 1; }
    break;

  case 118:
#line 619 "testpatterny.y"
    { return 0; }
    break;

  case 119:
#line 623 "testpatterny.y"
    {
	  current_testpattern = get_next_testpattern();
	}
    break;


/* Line 1267 of yacc.c.  */
#line 2120 "testpatterny.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 629 "testpatterny.y"


