/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     FUNC_CALL = 258,
     NAME = 259,
     REGEXP = 260,
     ERROR = 261,
     YNUMBER = 262,
     YSTRING = 263,
     RELOP = 264,
     IO_OUT = 265,
     IO_IN = 266,
     ASSIGNOP = 267,
     ASSIGN = 268,
     MATCHOP = 269,
     CONCAT_OP = 270,
     LEX_BEGIN = 271,
     LEX_END = 272,
     LEX_IF = 273,
     LEX_ELSE = 274,
     LEX_RETURN = 275,
     LEX_DELETE = 276,
     LEX_SWITCH = 277,
     LEX_CASE = 278,
     LEX_DEFAULT = 279,
     LEX_WHILE = 280,
     LEX_DO = 281,
     LEX_FOR = 282,
     LEX_BREAK = 283,
     LEX_CONTINUE = 284,
     LEX_PRINT = 285,
     LEX_PRINTF = 286,
     LEX_NEXT = 287,
     LEX_EXIT = 288,
     LEX_FUNCTION = 289,
     LEX_GETLINE = 290,
     LEX_NEXTFILE = 291,
     LEX_IN = 292,
     LEX_AND = 293,
     LEX_OR = 294,
     INCREMENT = 295,
     DECREMENT = 296,
     LEX_BUILTIN = 297,
     LEX_LENGTH = 298,
     NEWLINE = 299,
     SLASH_BEFORE_EQUAL = 300,
     UNARY = 301
   };
#endif
#define FUNC_CALL 258
#define NAME 259
#define REGEXP 260
#define ERROR 261
#define YNUMBER 262
#define YSTRING 263
#define RELOP 264
#define IO_OUT 265
#define IO_IN 266
#define ASSIGNOP 267
#define ASSIGN 268
#define MATCHOP 269
#define CONCAT_OP 270
#define LEX_BEGIN 271
#define LEX_END 272
#define LEX_IF 273
#define LEX_ELSE 274
#define LEX_RETURN 275
#define LEX_DELETE 276
#define LEX_SWITCH 277
#define LEX_CASE 278
#define LEX_DEFAULT 279
#define LEX_WHILE 280
#define LEX_DO 281
#define LEX_FOR 282
#define LEX_BREAK 283
#define LEX_CONTINUE 284
#define LEX_PRINT 285
#define LEX_PRINTF 286
#define LEX_NEXT 287
#define LEX_EXIT 288
#define LEX_FUNCTION 289
#define LEX_GETLINE 290
#define LEX_NEXTFILE 291
#define LEX_IN 292
#define LEX_AND 293
#define LEX_OR 294
#define INCREMENT 295
#define DECREMENT 296
#define LEX_BUILTIN 297
#define LEX_LENGTH 298
#define NEWLINE 299
#define SLASH_BEFORE_EQUAL 300
#define UNARY 301




/* Copy the first part of user declarations.  */
#line 26 "awkgram.y"

#ifdef GAWKDEBUG
#define YYDEBUG 12
#endif

#include "awk.h"

#define CAN_FREE	TRUE
#define DONT_FREE	FALSE

#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
static void yyerror(const char *m, ...) ATTRIBUTE_PRINTF_1;
#else
static void yyerror(); /* va_alist */
#endif
static char *get_src_buf P((void));
static int yylex P((void));
static NODE *node_common P((NODETYPE op));
static NODE *snode P((NODE *subn, NODETYPE op, int sindex));
static NODE *make_for_loop P((NODE *init, NODE *cond, NODE *incr));
static NODE *append_right P((NODE *list, NODE *new));
static inline NODE *append_pattern P((NODE **list, NODE *patt));
static void func_install P((NODE *params, NODE *def));
static void pop_var P((NODE *np, int freeit));
static void pop_params P((NODE *params));
static NODE *make_param P((char *name));
static NODE *mk_rexp P((NODE *exp));
static int dup_parms P((NODE *func));
static void param_sanity P((NODE *arglist));
static int parms_shadow P((const char *fname, NODE *func));
static int isnoeffect P((NODETYPE t));
static int isassignable P((NODE *n));
static void dumpintlstr P((const char *str, size_t len));
static void dumpintlstr2 P((const char *str1, size_t len1, const char *str2, size_t len2));
static void count_args P((NODE *n));
static int isarray P((NODE *n));

enum defref { FUNC_DEFINE, FUNC_USE };
static void func_use P((const char *name, enum defref how));
static void check_funcs P((void));

static int want_regexp;		/* lexical scanning kludge */
static int can_return;		/* parsing kludge */
static int begin_or_end_rule = FALSE;	/* parsing kludge */
static int parsing_end_rule = FALSE; /* for warnings */
static int in_print = FALSE;	/* lexical scanning kludge for print */
static int in_parens = 0;	/* lexical scanning kludge for print */
static char *lexptr;		/* pointer to next char during parsing */
static char *lexend;
static char *lexptr_begin;	/* keep track of where we were for error msgs */
static char *lexeme;		/* beginning of lexeme for debugging */
static char *thisline = NULL;
#define YYDEBUG_LEXER_TEXT (lexeme)
static int param_counter;
static char *tokstart = NULL;
static char *tok = NULL;
static char *tokend;

static long func_count;		/* total number of functions */

#define HASHSIZE	1021	/* this constant only used here */
NODE *variables[HASHSIZE];
static int var_count;		/* total number of global variables */

extern char *source;
extern int sourceline;
extern struct src *srcfiles;
extern int numfiles;
extern int errcount;
extern NODE *begin_block;
extern NODE *end_block;

/*
 * This string cannot occur as a real awk identifier.
 * Use it as a special token to make function parsing
 * uniform, but if it's seen, don't install the function.
 * e.g.
 * 	function split(x) { return x }
 * 	function x(a) { return a }
 * should only produce one error message, and not core dump.
 */
static char builtin_func[] = "@builtin";


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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 110 "awkgram.y"
typedef union YYSTYPE {
	long lval;
	AWKNUM fval;
	NODE *nodeval;
	NODETYPE nodetypeval;
	char *sval;
	NODE *(*ptrval) P((void));
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 260 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 272 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#undef YYSTACK_USE_ALLOCA	/* Gawk: nuke alloca once and for all */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
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
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
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
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1008

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  53
/* YYNRULES -- Number of rules. */
#define YYNRULES  155
/* YYNRULES -- Number of states. */
#define YYNSTATES  293

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,     2,     2,    59,    55,     2,     2,
      60,    61,    53,    51,    48,    52,     2,    54,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    47,    66,
      49,     2,    50,    46,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    62,     2,    63,    58,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    64,     2,    65,     2,     2,     2,     2,
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
      45,    57
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     7,     8,    11,    14,    17,    20,    23,
      24,    26,    30,    32,    34,    40,    42,    44,    46,    48,
      50,    51,    59,    60,    64,    66,    68,    69,    72,    75,
      77,    80,    83,    87,    89,    99,   106,   115,   124,   137,
     149,   152,   155,   158,   161,   165,   166,   171,   174,   175,
     180,   186,   189,   194,   196,   197,   199,   201,   202,   205,
     208,   214,   219,   221,   224,   227,   229,   231,   233,   235,
     237,   243,   244,   245,   249,   256,   266,   268,   271,   272,
     274,   275,   278,   279,   281,   283,   287,   289,   292,   296,
     297,   299,   300,   302,   304,   308,   310,   313,   317,   321,
     325,   329,   333,   337,   341,   345,   351,   353,   355,   357,
     360,   362,   364,   366,   368,   370,   373,   379,   381,   384,
     386,   390,   394,   398,   402,   406,   410,   414,   419,   422,
     425,   428,   432,   437,   442,   444,   449,   451,   454,   457,
     459,   461,   464,   467,   468,   470,   472,   477,   480,   483,
     486,   488,   489,   491,   493,   495
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      68,     0,    -1,    97,    69,    97,    -1,    -1,    69,    70,
      -1,    69,     1,    -1,    71,    72,    -1,    71,    81,    -1,
      75,    72,    -1,    -1,   104,    -1,   104,    48,   104,    -1,
      16,    -1,    17,    -1,   113,    80,   114,   116,    97,    -1,
       4,    -1,     3,    -1,    74,    -1,    42,    -1,    43,    -1,
      -1,    34,    76,    73,    60,    99,   115,    97,    -1,    -1,
      79,    78,     5,    -1,    54,    -1,    45,    -1,    -1,    80,
      82,    -1,    80,     1,    -1,    96,    -1,   117,    97,    -1,
     117,    97,    -1,   113,    80,   114,    -1,    95,    -1,    22,
      60,   104,   115,    97,   113,    87,    97,   114,    -1,    25,
      60,   104,   115,    97,    82,    -1,    26,    97,    82,    25,
      60,   104,   115,    97,    -1,    27,    60,     4,    37,     4,
     115,    97,    82,    -1,    27,    60,    86,   117,    97,   104,
     117,    97,    86,   115,    97,    82,    -1,    27,    60,    86,
     117,    97,   117,    97,    86,   115,    97,    82,    -1,    28,
      81,    -1,    29,    81,    -1,    32,    81,    -1,    36,    81,
      -1,    33,   101,    81,    -1,    -1,    20,    83,   101,    81,
      -1,    84,    81,    -1,    -1,    91,    85,    92,    93,    -1,
      21,     4,    62,   103,    63,    -1,    21,     4,    -1,    21,
      60,     4,    61,    -1,   104,    -1,    -1,    84,    -1,    88,
      -1,    -1,    88,    89,    -1,    88,     1,    -1,    23,    90,
     118,    97,    80,    -1,    24,   118,    97,    80,    -1,     7,
      -1,    52,     7,    -1,    51,     7,    -1,     8,    -1,    77,
      -1,    30,    -1,    31,    -1,   102,    -1,    60,   104,   119,
     103,   115,    -1,    -1,    -1,    10,    94,   108,    -1,    18,
      60,   104,   115,    97,    82,    -1,    18,    60,   104,   115,
      97,    82,    19,    97,    82,    -1,    44,    -1,    96,    44,
      -1,    -1,    96,    -1,    -1,    49,   109,    -1,    -1,   100,
      -1,     4,    -1,   100,   119,     4,    -1,     1,    -1,   100,
       1,    -1,   100,   119,     1,    -1,    -1,   104,    -1,    -1,
     103,    -1,   104,    -1,   103,   119,   104,    -1,     1,    -1,
     103,     1,    -1,   103,     1,   104,    -1,   103,   119,     1,
      -1,   112,   105,   104,    -1,   104,    38,   104,    -1,   104,
      39,   104,    -1,   104,    14,   104,    -1,   104,    37,     4,
      -1,   104,   107,   104,    -1,   104,    46,   104,    47,   104,
      -1,   108,    -1,    13,    -1,    12,    -1,    45,    13,    -1,
       9,    -1,    49,    -1,   106,    -1,    50,    -1,    77,    -1,
      56,    77,    -1,    60,   103,   115,    37,     4,    -1,   109,
      -1,   108,   109,    -1,   110,    -1,   109,    58,   109,    -1,
     109,    53,   109,    -1,   109,    54,   109,    -1,   109,    55,
     109,    -1,   109,    51,   109,    -1,   109,    52,   109,    -1,
      35,   111,    98,    -1,   109,    11,    35,   111,    -1,   112,
      40,    -1,   112,    41,    -1,    56,   109,    -1,    60,   104,
     115,    -1,    42,    60,   102,   115,    -1,    43,    60,   102,
     115,    -1,    43,    -1,     3,    60,   102,   115,    -1,   112,
      -1,    40,   112,    -1,    41,   112,    -1,     7,    -1,     8,
      -1,    52,   109,    -1,    51,   109,    -1,    -1,   112,    -1,
       4,    -1,     4,    62,   103,    63,    -1,    59,   110,    -1,
      64,    97,    -1,    65,    97,    -1,    61,    -1,    -1,   117,
      -1,    66,    -1,    47,    -1,    48,    97,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   171,   171,   177,   179,   184,   196,   200,   215,   226,
     229,   233,   243,   248,   256,   261,   263,   265,   276,   277,
     282,   281,   305,   304,   327,   328,   333,   334,   352,   357,
     358,   362,   364,   366,   368,   370,   372,   374,   416,   420,
     425,   428,   431,   440,   460,   463,   462,   469,   481,   481,
     512,   514,   528,   543,   549,   550,   555,   608,   609,   626,
     631,   633,   638,   640,   645,   647,   649,   654,   655,   663,
     664,   670,   675,   675,   686,   691,   698,   699,   702,   704,
     709,   710,   716,   717,   722,   724,   726,   728,   730,   737,
     738,   744,   745,   750,   752,   758,   760,   762,   764,   769,
     775,   777,   779,   785,   787,   793,   795,   800,   802,   804,
     809,   811,   815,   816,   821,   823,   831,   833,   835,   840,
     842,   844,   846,   848,   850,   852,   854,   860,   865,   867,
     872,   874,   876,   879,   881,   889,   897,   898,   900,   902,
     904,   907,   915,   927,   928,   933,   935,   949,   954,   958,
     962,   965,   967,   971,   975,   978
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "FUNC_CALL", "NAME", "REGEXP", "ERROR", 
  "YNUMBER", "YSTRING", "RELOP", "IO_OUT", "IO_IN", "ASSIGNOP", "ASSIGN", 
  "MATCHOP", "CONCAT_OP", "LEX_BEGIN", "LEX_END", "LEX_IF", "LEX_ELSE", 
  "LEX_RETURN", "LEX_DELETE", "LEX_SWITCH", "LEX_CASE", "LEX_DEFAULT", 
  "LEX_WHILE", "LEX_DO", "LEX_FOR", "LEX_BREAK", "LEX_CONTINUE", 
  "LEX_PRINT", "LEX_PRINTF", "LEX_NEXT", "LEX_EXIT", "LEX_FUNCTION", 
  "LEX_GETLINE", "LEX_NEXTFILE", "LEX_IN", "LEX_AND", "LEX_OR", 
  "INCREMENT", "DECREMENT", "LEX_BUILTIN", "LEX_LENGTH", "NEWLINE", 
  "SLASH_BEFORE_EQUAL", "'?'", "':'", "','", "'<'", "'>'", "'+'", "'-'", 
  "'*'", "'/'", "'%'", "'!'", "UNARY", "'^'", "'$'", "'('", "')'", "'['", 
  "']'", "'{'", "'}'", "';'", "$accept", "start", "program", "rule", 
  "pattern", "action", "func_name", "lex_builtin", "function_prologue", 
  "@1", "regexp", "@2", "a_slash", "statements", "statement_term", 
  "statement", "@3", "simple_stmt", "@4", "opt_simple_stmt", 
  "switch_body", "case_statements", "case_statement", "case_value", 
  "print", "print_expression_list", "output_redir", "@5", "if_statement", 
  "nls", "opt_nls", "input_redir", "opt_param_list", "param_list", 
  "opt_exp", "opt_expression_list", "expression_list", "exp", 
  "assign_operator", "relop_or_less", "a_relop", "common_exp", "simp_exp", 
  "non_post_simp_exp", "opt_variable", "variable", "l_brace", "r_brace", 
  "r_paren", "opt_semi", "semi", "colon", "comma", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,    63,    58,    44,    60,
      62,    43,    45,    42,    47,    37,    33,   301,    94,    36,
      40,    41,    91,    93,   123,   125,    59
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    67,    68,    69,    69,    69,    70,    70,    70,    71,
      71,    71,    71,    71,    72,    73,    73,    73,    74,    74,
      76,    75,    78,    77,    79,    79,    80,    80,    80,    81,
      81,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    83,    82,    82,    85,    84,
      84,    84,    84,    84,    86,    86,    87,    88,    88,    88,
      89,    89,    90,    90,    90,    90,    90,    91,    91,    92,
      92,    93,    94,    93,    95,    95,    96,    96,    97,    97,
      98,    98,    99,    99,   100,   100,   100,   100,   100,   101,
     101,   102,   102,   103,   103,   103,   103,   103,   103,   104,
     104,   104,   104,   104,   104,   104,   104,   105,   105,   105,
     106,   106,   107,   107,   108,   108,   108,   108,   108,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     110,   110,   110,   110,   110,   110,   110,   110,   110,   110,
     110,   110,   110,   111,   111,   112,   112,   112,   113,   114,
     115,   116,   116,   117,   118,   119
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     0,     2,     2,     2,     2,     2,     0,
       1,     3,     1,     1,     5,     1,     1,     1,     1,     1,
       0,     7,     0,     3,     1,     1,     0,     2,     2,     1,
       2,     2,     3,     1,     9,     6,     8,     8,    12,    11,
       2,     2,     2,     2,     3,     0,     4,     2,     0,     4,
       5,     2,     4,     1,     0,     1,     1,     0,     2,     2,
       5,     4,     1,     2,     2,     1,     1,     1,     1,     1,
       5,     0,     0,     3,     6,     9,     1,     2,     0,     1,
       0,     2,     0,     1,     1,     3,     1,     2,     3,     0,
       1,     0,     1,     1,     3,     1,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     2,     5,     1,     2,     1,
       3,     3,     3,     3,     3,     3,     3,     4,     2,     2,
       2,     3,     4,     4,     1,     4,     1,     2,     2,     1,
       1,     2,     2,     0,     1,     1,     4,     2,     2,     2,
       1,     0,     1,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
      78,    76,     0,    79,     3,     1,    77,     0,     5,     0,
     145,   139,   140,    12,    13,    20,   143,     0,     0,     0,
     134,    25,     0,     0,    24,     0,     0,     0,     4,     0,
       0,   114,    22,     2,    10,   106,   117,   119,   136,     0,
       0,     0,    80,   144,   137,   138,     0,     0,     0,     0,
     142,   136,   141,   115,   130,   147,   136,    95,     0,    93,
      78,   153,     6,     7,    29,    26,    78,     8,     0,   110,
       0,     0,     0,     0,     0,     0,   111,   113,   112,     0,
     118,     0,     0,     0,     0,     0,     0,     0,   108,   107,
     128,   129,     0,     0,     0,     0,    93,     0,    16,    15,
      18,    19,     0,    17,     0,   126,     0,     0,     0,    96,
      78,   150,     0,     0,   131,   148,     0,    30,    23,   102,
     103,   100,   101,     0,    11,   104,   143,   124,   125,   121,
     122,   123,   120,   109,    99,   135,   146,     0,    81,   132,
     133,    97,   155,     0,    98,    94,    28,     0,    45,     0,
       0,     0,    78,     0,     0,     0,    67,    68,     0,    89,
       0,    78,    27,     0,    48,    33,    53,    26,   151,    78,
       0,   127,    86,    84,     0,     0,   116,     0,    89,    51,
       0,     0,     0,     0,    54,    40,    41,    42,     0,    90,
      43,   149,    47,     0,     0,    78,   152,    31,   105,    78,
      87,     0,     0,     0,     0,     0,     0,     0,     0,   145,
      55,     0,    44,     0,    71,    69,    32,    14,    21,    88,
      85,    78,    46,     0,    52,    78,    78,     0,     0,    78,
      93,    72,    49,     0,    50,     0,     0,     0,     0,     0,
       0,     0,    74,    57,    35,     0,    78,     0,    78,     0,
      73,    78,    78,     0,    78,     0,    78,    54,    70,     0,
       0,    59,     0,     0,    58,    36,    37,    54,     0,    75,
      34,    62,    65,     0,     0,    66,     0,   154,    78,     0,
      78,    64,    63,    78,    26,    78,     0,    26,     0,     0,
      39,     0,    38
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     2,     7,    28,    29,    62,   102,   103,    30,    41,
      31,    68,    32,   116,    63,   162,   178,   163,   193,   211,
     252,   253,   264,   276,   164,   214,   232,   241,   165,     3,
       4,   105,   174,   175,   188,    94,    95,   166,    93,    78,
      79,    35,    36,    37,    42,    38,   167,   168,   114,   195,
     169,   278,   113
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -236
static const short yypact[] =
{
     -15,  -236,    31,    -3,  -236,  -236,  -236,   292,  -236,    -8,
      -2,  -236,  -236,  -236,  -236,  -236,    26,    26,    26,     4,
       6,  -236,   934,   934,  -236,   876,   262,   717,  -236,   -16,
       3,  -236,  -236,  -236,   769,   934,   480,  -236,   554,   656,
     717,    33,    49,  -236,  -236,  -236,   656,   656,   934,   905,
      41,    -1,    41,  -236,    41,  -236,  -236,  -236,   117,   695,
     -15,  -236,  -236,  -236,    -3,  -236,   -15,  -236,   101,  -236,
     905,   103,   905,   905,   905,   905,  -236,  -236,  -236,   905,
     480,    74,   934,   934,   934,   934,   934,   934,  -236,  -236,
    -236,  -236,   100,   905,    54,    17,   958,    32,  -236,  -236,
    -236,  -236,    56,  -236,   934,  -236,    54,    54,   695,   905,
     -15,  -236,    86,   739,  -236,  -236,   454,  -236,  -236,   133,
    -236,   184,   166,   823,   958,     7,    26,   136,   136,    41,
      41,    41,    41,  -236,   958,  -236,  -236,    43,   538,  -236,
    -236,   958,  -236,   121,  -236,   958,  -236,    68,  -236,     2,
      69,    70,   -15,    71,    61,    61,  -236,  -236,    61,   905,
      61,   -15,  -236,    61,  -236,  -236,   958,  -236,    73,   -15,
     905,  -236,  -236,  -236,    54,   123,  -236,   905,   905,    81,
     131,   905,   905,   580,   793,  -236,  -236,  -236,    61,   958,
    -236,  -236,  -236,   520,   454,   -15,  -236,  -236,   958,   -15,
    -236,    45,   695,    61,   717,    83,   695,   695,   130,   -11,
    -236,    73,  -236,   717,   147,  -236,  -236,  -236,  -236,  -236,
    -236,   -15,  -236,    34,  -236,   -15,   -15,   108,   169,   -15,
     508,  -236,  -236,   580,  -236,     3,   580,   905,    54,   634,
     717,   905,   155,  -236,  -236,   695,   -15,   502,   -15,   117,
     934,   -15,   -15,    19,   -15,   580,   -15,   847,  -236,   580,
     111,  -236,   201,   132,  -236,  -236,  -236,   847,    54,  -236,
    -236,  -236,  -236,   178,   179,  -236,   132,  -236,   -15,    54,
     -15,  -236,  -236,   -15,  -236,   -15,   580,  -236,   346,   580,
    -236,   400,  -236
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -236,  -236,  -236,  -236,  -236,   165,  -236,  -236,  -236,  -236,
     -24,  -236,  -236,  -150,   152,  -178,  -236,  -165,  -236,  -235,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,   -22,
      -7,  -236,  -236,  -236,    21,   -32,   -17,    47,  -236,  -236,
    -236,   -41,    66,   175,    76,   -14,    -5,  -181,    52,  -236,
       9,   -71,  -130
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -93
static const short yytable[] =
{
      33,    53,    43,    44,    45,   208,   179,    64,    51,    51,
      58,    51,    56,   216,   106,   107,   -93,   194,   109,   210,
     261,    51,   268,    97,    65,    65,   228,   -92,     1,     1,
      10,     5,   279,   109,    51,   109,    98,    99,    66,    90,
      91,     6,   262,   263,   172,   201,   219,   173,    60,   220,
      61,    40,    39,   115,    34,   242,   -93,   -93,   244,   117,
      40,   -92,   180,   -56,    46,   110,    47,    60,    51,    51,
      51,    51,    51,    51,    59,   100,   101,   266,   -92,   270,
     110,   269,   110,   -92,   -56,    26,    96,    96,    50,    52,
      51,    54,   210,    96,    96,   136,   108,   234,   104,    87,
     240,    80,   210,   142,   -82,     1,   118,   120,   290,   126,
     112,   292,    43,   133,    54,   111,   137,   119,   109,   121,
     122,   123,   124,   143,   200,   176,   125,    61,   177,   181,
     182,   184,    64,    64,   288,   205,    64,   291,    64,    61,
     134,    64,    69,   204,   224,   183,   135,   -93,   127,   128,
     129,   130,   131,   132,   191,   227,   141,   231,   139,   140,
     145,   215,   197,    66,    66,   110,    64,    66,   237,    66,
     138,   110,    66,   238,   251,    69,   161,   196,   111,   277,
      70,    64,    76,    77,   -83,   281,   282,   223,   217,    84,
      85,    86,   218,    69,    87,    67,    58,    66,    70,   203,
     250,    55,   171,    71,    72,   283,   189,     0,   271,   272,
       0,     0,    66,     0,   233,    76,    77,   198,   235,   236,
     229,    71,   239,   249,   202,   189,   199,    51,   206,   207,
     243,     0,     0,    76,    77,     0,    51,     0,   275,   255,
      96,   257,     0,     0,   259,   260,    21,   265,   248,   267,
       0,    96,   273,   274,   221,    24,   256,     0,   225,   226,
     230,     0,     0,     0,     0,     9,    10,     0,     0,    11,
      12,   284,     0,   286,     0,     0,   287,     0,   289,     0,
       0,     0,     0,     0,   245,     0,   247,    96,     0,     0,
     246,     0,   -78,     8,     0,     9,    10,   254,     0,    11,
      12,   258,    17,    18,    19,    20,   185,   186,    13,    14,
     187,     0,   190,    22,    23,   192,    80,     0,    48,     0,
     280,    26,    49,     0,     0,     0,    15,    16,     0,     0,
       0,   285,    17,    18,    19,    20,     1,    21,     0,     0,
     212,     0,     0,    22,    23,     0,    24,   146,    25,     9,
      10,    26,    27,    11,    12,   222,    -9,     0,    -9,     0,
       0,     0,     0,     0,   147,     0,   148,   149,   150,   -61,
     -61,   151,   152,   153,   154,   155,   156,   157,   158,   159,
       0,    16,   160,     0,     0,     0,    17,    18,    19,    20,
     -61,    21,     0,     0,     0,     0,     0,    22,    23,     0,
      24,   146,    25,     9,    10,    26,    27,    11,    12,     0,
      60,   -61,    61,     0,     0,     0,     0,     0,   147,     0,
     148,   149,   150,   -60,   -60,   151,   152,   153,   154,   155,
     156,   157,   158,   159,     0,    16,   160,     0,     0,     0,
      17,    18,    19,    20,   -60,    21,     0,     0,     0,     0,
       0,    22,    23,     0,    24,   146,    25,     9,    10,    26,
      27,    11,    12,     0,    60,   -60,    61,     0,     0,     0,
       0,     0,   147,     0,   148,   149,   150,     0,     0,   151,
     152,   153,   154,   155,   156,   157,   158,   159,     0,    16,
     160,    81,     0,     0,    17,    18,    19,    20,     0,    21,
       0,     0,     0,     0,     0,    22,    23,     0,    24,     0,
      25,    69,     0,    26,    27,     0,    70,    69,    60,   161,
      61,    57,    70,     9,    10,     0,     0,    11,    12,     0,
     -91,    82,    83,    84,    85,    86,     0,     0,    87,    71,
      72,    73,     0,     0,     0,    71,    72,    73,    74,   -93,
       0,    76,    77,     0,    74,    16,   110,    76,    77,     0,
      17,    18,    19,    20,   -91,    21,    88,    89,    61,   111,
       0,    22,    23,     0,    24,     0,    25,     0,     0,    26,
     213,   -91,     0,     9,    10,     0,   -91,    11,    12,    82,
      83,    84,    85,    86,    90,    91,    87,     0,   147,    92,
     148,   149,   150,     0,     0,   151,   152,   153,   154,   155,
     156,   157,   158,   159,     0,    16,   160,     0,     0,     0,
      17,    18,    19,    20,     0,    21,     0,     0,     0,     0,
       0,    22,    23,     0,    24,     0,    25,     9,    10,    26,
      27,    11,    12,     0,    60,     0,    61,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     9,
      10,     0,     0,    11,    12,     0,     0,     0,     0,    16,
       0,     0,     0,     0,    17,    18,    19,    20,     0,    21,
       0,     0,     0,     0,     0,    22,    23,     0,    24,     0,
      25,    16,     0,    26,    27,     0,    17,    18,    19,    20,
      61,    21,     0,     0,    69,     0,     0,    22,    23,    70,
      24,     0,    25,     0,     0,    26,    27,   -91,    57,     0,
       9,    10,     0,     0,    11,    12,     0,     0,     0,     0,
       0,     0,    71,    72,    73,     0,     0,     0,     0,     0,
     144,    74,     9,    10,    76,    77,    11,    12,     0,     0,
       0,     0,    16,     0,     0,     0,   111,    17,    18,    19,
      20,     0,    21,     0,     0,     0,     0,     0,    22,    23,
       0,    24,     0,    25,    16,     0,    26,    27,    69,    17,
      18,    19,    20,    70,    21,     0,     0,     0,     0,     0,
      22,    23,     0,    24,     0,    25,     9,   209,    26,    27,
      11,    12,     0,     0,     0,     0,    71,    72,    73,     0,
       0,     0,     0,     0,   149,    74,     0,    75,    76,    77,
       0,     0,     0,   156,   157,     0,     0,     0,    16,     0,
       0,     0,    69,    17,    18,    19,    20,    70,    21,     0,
       0,     0,     0,     0,    22,    23,     0,    24,     0,    25,
       9,    10,    26,    27,    11,    12,     0,     0,     0,     0,
      71,    72,    73,     0,     0,     0,     0,     0,   149,    74,
     170,     0,    76,    77,     0,     0,     0,   156,   157,     9,
      10,     0,    16,    11,    12,     0,     0,    17,    18,    19,
      20,     0,    21,     0,     0,     0,     0,     0,    22,    23,
       0,    24,     0,    25,     0,     0,    26,    27,     9,    10,
       0,    16,    11,    12,     0,     0,    17,    18,    19,    20,
       0,    21,     0,     0,     0,     0,     0,    22,    23,     0,
      24,     0,    48,     0,     0,    26,    49,     9,    10,     0,
      16,    11,    12,     0,     0,    17,    18,    19,    20,     0,
      21,     0,     0,     0,     0,     0,    22,    23,     0,    24,
       0,    25,     0,     0,    26,    27,     0,    69,     0,    16,
       0,     0,    70,     0,    17,    18,    19,    20,     0,     0,
       0,     0,     0,     0,     0,    22,    23,     0,     0,     0,
      48,     0,     0,    26,    49,    71,    72,    73,     0,     0,
       0,     0,     0,     0,    74,     0,     0,    76,    77
};

static const short yycheck[] =
{
       7,    25,    16,    17,    18,   183,     4,    29,    22,    23,
      27,    25,    26,   194,    46,    47,     9,   167,     1,   184,
       1,    35,   257,    40,    29,    30,    37,    10,    44,    44,
       4,     0,   267,     1,    48,     1,     3,     4,    29,    40,
      41,    44,    23,    24,     1,   175,     1,     4,    64,     4,
      66,    62,    60,    60,     7,   233,    49,    50,   236,    66,
      62,    44,    60,    44,    60,    48,    60,    64,    82,    83,
      84,    85,    86,    87,    27,    42,    43,   255,    61,   260,
      48,   259,    48,    66,    65,    59,    39,    40,    22,    23,
     104,    25,   257,    46,    47,    63,    49,    63,    49,    58,
     230,    35,   267,   110,    61,    44,     5,     4,   286,    35,
      58,   289,   126,    13,    48,    61,    60,    70,     1,    72,
      73,    74,    75,    37,     1,     4,    79,    66,    60,    60,
      60,    60,   154,   155,   284,     4,   158,   287,   160,    66,
      93,   163,     9,    62,    61,   152,    94,    14,    82,    83,
      84,    85,    86,    87,   161,    25,   109,    10,   106,   107,
     113,   193,   169,   154,   155,    48,   188,   158,    60,   160,
     104,    48,   163,     4,    19,     9,    65,   168,    61,    47,
      14,   203,    49,    50,    61,     7,     7,   204,   195,    53,
      54,    55,   199,     9,    58,    30,   213,   188,    14,   178,
     241,    26,   126,    37,    38,   276,   159,    -1,     7,     8,
      -1,    -1,   203,    -1,   221,    49,    50,   170,   225,   226,
     211,    37,   229,   240,   177,   178,   174,   241,   181,   182,
     235,    -1,    -1,    49,    50,    -1,   250,    -1,   262,   246,
     193,   248,    -1,    -1,   251,   252,    45,   254,   239,   256,
      -1,   204,    51,    52,   202,    54,   247,    -1,   206,   207,
     213,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,     7,
       8,   278,    -1,   280,    -1,    -1,   283,    -1,   285,    -1,
      -1,    -1,    -1,    -1,   237,    -1,   239,   240,    -1,    -1,
     238,    -1,     0,     1,    -1,     3,     4,   245,    -1,     7,
       8,   249,    40,    41,    42,    43,   154,   155,    16,    17,
     158,    -1,   160,    51,    52,   163,   250,    -1,    56,    -1,
     268,    59,    60,    -1,    -1,    -1,    34,    35,    -1,    -1,
      -1,   279,    40,    41,    42,    43,    44,    45,    -1,    -1,
     188,    -1,    -1,    51,    52,    -1,    54,     1,    56,     3,
       4,    59,    60,     7,     8,   203,    64,    -1,    66,    -1,
      -1,    -1,    -1,    -1,    18,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    -1,    -1,    -1,    40,    41,    42,    43,
      44,    45,    -1,    -1,    -1,    -1,    -1,    51,    52,    -1,
      54,     1,    56,     3,     4,    59,    60,     7,     8,    -1,
      64,    65,    66,    -1,    -1,    -1,    -1,    -1,    18,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    -1,    -1,    -1,
      40,    41,    42,    43,    44,    45,    -1,    -1,    -1,    -1,
      -1,    51,    52,    -1,    54,     1,    56,     3,     4,    59,
      60,     7,     8,    -1,    64,    65,    66,    -1,    -1,    -1,
      -1,    -1,    18,    -1,    20,    21,    22,    -1,    -1,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    11,    -1,    -1,    40,    41,    42,    43,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    51,    52,    -1,    54,    -1,
      56,     9,    -1,    59,    60,    -1,    14,     9,    64,    65,
      66,     1,    14,     3,     4,    -1,    -1,     7,     8,    -1,
      10,    51,    52,    53,    54,    55,    -1,    -1,    58,    37,
      38,    39,    -1,    -1,    -1,    37,    38,    39,    46,    11,
      -1,    49,    50,    -1,    46,    35,    48,    49,    50,    -1,
      40,    41,    42,    43,    44,    45,    12,    13,    66,    61,
      -1,    51,    52,    -1,    54,    -1,    56,    -1,    -1,    59,
      60,    61,    -1,     3,     4,    -1,    66,     7,     8,    51,
      52,    53,    54,    55,    40,    41,    58,    -1,    18,    45,
      20,    21,    22,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    -1,    -1,    -1,
      40,    41,    42,    43,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    51,    52,    -1,    54,    -1,    56,     3,     4,    59,
      60,     7,     8,    -1,    64,    -1,    66,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,
       4,    -1,    -1,     7,     8,    -1,    -1,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    40,    41,    42,    43,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    51,    52,    -1,    54,    -1,
      56,    35,    -1,    59,    60,    -1,    40,    41,    42,    43,
      66,    45,    -1,    -1,     9,    -1,    -1,    51,    52,    14,
      54,    -1,    56,    -1,    -1,    59,    60,    61,     1,    -1,
       3,     4,    -1,    -1,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,
       1,    46,     3,     4,    49,    50,     7,     8,    -1,    -1,
      -1,    -1,    35,    -1,    -1,    -1,    61,    40,    41,    42,
      43,    -1,    45,    -1,    -1,    -1,    -1,    -1,    51,    52,
      -1,    54,    -1,    56,    35,    -1,    59,    60,     9,    40,
      41,    42,    43,    14,    45,    -1,    -1,    -1,    -1,    -1,
      51,    52,    -1,    54,    -1,    56,     3,     4,    59,    60,
       7,     8,    -1,    -1,    -1,    -1,    37,    38,    39,    -1,
      -1,    -1,    -1,    -1,    21,    46,    -1,    48,    49,    50,
      -1,    -1,    -1,    30,    31,    -1,    -1,    -1,    35,    -1,
      -1,    -1,     9,    40,    41,    42,    43,    14,    45,    -1,
      -1,    -1,    -1,    -1,    51,    52,    -1,    54,    -1,    56,
       3,     4,    59,    60,     7,     8,    -1,    -1,    -1,    -1,
      37,    38,    39,    -1,    -1,    -1,    -1,    -1,    21,    46,
      47,    -1,    49,    50,    -1,    -1,    -1,    30,    31,     3,
       4,    -1,    35,     7,     8,    -1,    -1,    40,    41,    42,
      43,    -1,    45,    -1,    -1,    -1,    -1,    -1,    51,    52,
      -1,    54,    -1,    56,    -1,    -1,    59,    60,     3,     4,
      -1,    35,     7,     8,    -1,    -1,    40,    41,    42,    43,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    51,    52,    -1,
      54,    -1,    56,    -1,    -1,    59,    60,     3,     4,    -1,
      35,     7,     8,    -1,    -1,    40,    41,    42,    43,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    51,    52,    -1,    54,
      -1,    56,    -1,    -1,    59,    60,    -1,     9,    -1,    35,
      -1,    -1,    14,    -1,    40,    41,    42,    43,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    51,    52,    -1,    -1,    -1,
      56,    -1,    -1,    59,    60,    37,    38,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    49,    50
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    44,    68,    96,    97,     0,    44,    69,     1,     3,
       4,     7,     8,    16,    17,    34,    35,    40,    41,    42,
      43,    45,    51,    52,    54,    56,    59,    60,    70,    71,
      75,    77,    79,    97,   104,   108,   109,   110,   112,    60,
      62,    76,   111,   112,   112,   112,    60,    60,    56,    60,
     109,   112,   109,    77,   109,   110,   112,     1,   103,   104,
      64,    66,    72,    81,    96,   113,   117,    72,    78,     9,
      14,    37,    38,    39,    46,    48,    49,    50,   106,   107,
     109,    11,    51,    52,    53,    54,    55,    58,    12,    13,
      40,    41,    45,   105,   102,   103,   104,   103,     3,     4,
      42,    43,    73,    74,    49,    98,   102,   102,   104,     1,
      48,    61,   115,   119,   115,    97,    80,    97,     5,   104,
       4,   104,   104,   104,   104,   104,    35,   109,   109,   109,
     109,   109,   109,    13,   104,   115,    63,    60,   109,   115,
     115,   104,    97,    37,     1,   104,     1,    18,    20,    21,
      22,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      36,    65,    82,    84,    91,    95,   104,   113,   114,   117,
      47,   111,     1,     4,    99,   100,     4,    60,    83,     4,
      60,    60,    60,    97,    60,    81,    81,    81,   101,   104,
      81,    97,    81,    85,    80,   116,   117,    97,   104,   115,
       1,   119,   104,   101,    62,     4,   104,   104,    82,     4,
      84,    86,    81,    60,    92,   102,   114,    97,    97,     1,
       4,   115,    81,   103,    61,   115,   115,    25,    37,   117,
     104,    10,    93,    97,    63,    97,    97,    60,     4,    97,
     119,    94,    82,   113,    82,   104,   115,   104,   117,   103,
     108,    19,    87,    88,   115,    97,   117,    97,   115,    97,
      97,     1,    23,    24,    89,    97,    82,    97,    86,    82,
     114,     7,     8,    51,    52,    77,    90,    47,   118,    86,
     115,     7,     7,   118,    97,   115,    97,    97,    80,    97,
      82,    80,    82
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
#define YYERROR		goto yyerrlab1

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
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
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
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
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

#if YYMAXDEPTH == 0
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
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



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
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


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
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

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
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 172 "awkgram.y"
    {
			check_funcs();
		}
    break;

  case 4:
#line 180 "awkgram.y"
    {
		begin_or_end_rule = parsing_end_rule = FALSE;
		yyerrok;
	  }
    break;

  case 5:
#line 185 "awkgram.y"
    {
		begin_or_end_rule = parsing_end_rule = FALSE;
		/*
		 * If errors, give up, don't produce an infinite
		 * stream of syntax error messages.
		 */
  		/* yyerrok; */
  	  }
    break;

  case 6:
#line 197 "awkgram.y"
    {
		yyvsp[-1].nodeval->rnode = yyvsp[0].nodeval;
	  }
    break;

  case 7:
#line 201 "awkgram.y"
    {
		if (yyvsp[-1].nodeval->lnode != NULL) {
			/* pattern rule with non-empty pattern */
			yyvsp[-1].nodeval->rnode = node(NULL, Node_K_print_rec, NULL);
		} else {
			/* an error */
			if (begin_or_end_rule)
				warning(_("%s blocks must have an action part"),
					(parsing_end_rule ? "END" : "BEGIN"));
			else
				warning(_("each rule must have a pattern or an action part"));
			errcount++;
		}
	  }
    break;

  case 8:
#line 216 "awkgram.y"
    {
		can_return = FALSE;
		if (yyvsp[-1].nodeval)
			func_install(yyvsp[-1].nodeval, yyvsp[0].nodeval);
		yyerrok;
	  }
    break;

  case 9:
#line 226 "awkgram.y"
    {
		yyval.nodeval = append_pattern(&expression_value, (NODE *) NULL);
	  }
    break;

  case 10:
#line 230 "awkgram.y"
    {
		yyval.nodeval = append_pattern(&expression_value, yyvsp[0].nodeval);
	  }
    break;

  case 11:
#line 234 "awkgram.y"
    {
		NODE *r;

		getnode(r);
		r->type = Node_line_range;
		r->condpair = node(yyvsp[-2].nodeval, Node_cond_pair, yyvsp[0].nodeval);
		r->triggered = FALSE;
		yyval.nodeval = append_pattern(&expression_value, r);
	  }
    break;

  case 12:
#line 244 "awkgram.y"
    {
		begin_or_end_rule = TRUE;
		yyval.nodeval = append_pattern(&begin_block, (NODE *) NULL);
	  }
    break;

  case 13:
#line 249 "awkgram.y"
    {
		begin_or_end_rule = parsing_end_rule = TRUE;
		yyval.nodeval = append_pattern(&end_block, (NODE *) NULL);
	  }
    break;

  case 14:
#line 257 "awkgram.y"
    { yyval.nodeval = yyvsp[-3].nodeval; }
    break;

  case 15:
#line 262 "awkgram.y"
    { yyval.sval = yyvsp[0].sval; }
    break;

  case 16:
#line 264 "awkgram.y"
    { yyval.sval = yyvsp[0].sval; }
    break;

  case 17:
#line 266 "awkgram.y"
    {
		yyerror(_("`%s' is a built-in function, it cannot be redefined"),
			tokstart);
		errcount++;
		yyval.sval = builtin_func;
		/* yyerrok; */
	  }
    break;

  case 20:
#line 282 "awkgram.y"
    {
			param_counter = 0;
		}
    break;

  case 21:
#line 286 "awkgram.y"
    {
			NODE *t;

			t = make_param(yyvsp[-4].sval);
			t->flags |= FUNC;
			yyval.nodeval = append_right(t, yyvsp[-2].nodeval);
			can_return = TRUE;
			/* check for duplicate parameter names */
			if (dup_parms(yyval.nodeval))
				errcount++;
		}
    break;

  case 22:
#line 305 "awkgram.y"
    { ++want_regexp; }
    break;

  case 23:
#line 307 "awkgram.y"
    {
		  NODE *n;
		  size_t len = strlen(yyvsp[0].sval);

		  if (do_lint && (yyvsp[0].sval)[0] == '*') {
			/* possible C comment */
			if ((yyvsp[0].sval)[len-1] == '*')
				lintwarn(_("regexp constant `/%s/' looks like a C comment, but is not"), tokstart);
		  }
		  getnode(n);
		  n->type = Node_regex;
		  n->re_exp = make_string(yyvsp[0].sval, len);
		  n->re_reg = make_regexp(yyvsp[0].sval, len, FALSE);
		  n->re_text = NULL;
		  n->re_flags = CONST;
		  yyval.nodeval = n;
		}
    break;

  case 26:
#line 333 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 27:
#line 335 "awkgram.y"
    {
		if (yyvsp[0].nodeval == NULL)
			yyval.nodeval = yyvsp[-1].nodeval;
		else {
			if (do_lint && isnoeffect(yyvsp[0].nodeval->type))
				lintwarn(_("statement may have no effect"));
			if (yyvsp[-1].nodeval == NULL)
				yyval.nodeval = yyvsp[0].nodeval;
			else
	    			yyval.nodeval = append_right(
					(yyvsp[-1].nodeval->type == Node_statement_list ? yyvsp[-1].nodeval
					  : node(yyvsp[-1].nodeval, Node_statement_list, (NODE *) NULL)),
					(yyvsp[0].nodeval->type == Node_statement_list ? yyvsp[0].nodeval
					  : node(yyvsp[0].nodeval, Node_statement_list, (NODE *) NULL)));
		}
	    	yyerrok;
	  }
    break;

  case 28:
#line 353 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 31:
#line 363 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 32:
#line 365 "awkgram.y"
    { yyval.nodeval = yyvsp[-1].nodeval; }
    break;

  case 33:
#line 367 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 34:
#line 369 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-6].nodeval, Node_K_switch, yyvsp[-2].nodeval); }
    break;

  case 35:
#line 371 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-3].nodeval, Node_K_while, yyvsp[0].nodeval); }
    break;

  case 36:
#line 373 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_K_do, yyvsp[-5].nodeval); }
    break;

  case 37:
#line 375 "awkgram.y"
    {
		/*
		 * Efficiency hack.  Recognize the special case of
		 *
		 * 	for (iggy in foo)
		 * 		delete foo[iggy]
		 *
		 * and treat it as if it were
		 *
		 * 	delete foo
		 *
		 * Check that the body is a `delete a[i]' statement,
		 * and that both the loop var and array names match.
		 */
		if (yyvsp[0].nodeval != NULL && yyvsp[0].nodeval->type == Node_K_delete) {
			NODE *arr, *sub;

			assert(yyvsp[0].nodeval->rnode->type == Node_expression_list);
			arr = yyvsp[0].nodeval->lnode;	/* array var */
			sub = yyvsp[0].nodeval->rnode->lnode;	/* index var */

			if (   (arr->type == Node_var_new
				|| arr->type == Node_var_array
				|| arr->type == Node_param_list)
			    && (sub->type == Node_var_new
				|| sub->type == Node_var
				|| sub->type == Node_param_list)
			    && strcmp(yyvsp[-5].sval, sub->vname) == 0
			    && strcmp(yyvsp[-3].sval, arr->vname) == 0) {
				yyvsp[0].nodeval->type = Node_K_delete_loop;
				yyval.nodeval = yyvsp[0].nodeval;
			}
			else
				goto regular_loop;
		} else {
	regular_loop:
			yyval.nodeval = node(yyvsp[0].nodeval, Node_K_arrayfor,
				make_for_loop(variable(yyvsp[-5].sval, CAN_FREE, Node_var),
				(NODE *) NULL, variable(yyvsp[-3].sval, CAN_FREE, Node_var_array)));
		}
	  }
    break;

  case 38:
#line 417 "awkgram.y"
    {
		yyval.nodeval = node(yyvsp[0].nodeval, Node_K_for, (NODE *) make_for_loop(yyvsp[-9].nodeval, yyvsp[-6].nodeval, yyvsp[-3].nodeval));
	  }
    break;

  case 39:
#line 421 "awkgram.y"
    {
		yyval.nodeval = node(yyvsp[0].nodeval, Node_K_for,
			(NODE *) make_for_loop(yyvsp[-8].nodeval, (NODE *) NULL, yyvsp[-3].nodeval));
	  }
    break;

  case 40:
#line 427 "awkgram.y"
    { yyval.nodeval = node((NODE *) NULL, Node_K_break, (NODE *) NULL); }
    break;

  case 41:
#line 430 "awkgram.y"
    { yyval.nodeval = node((NODE *) NULL, Node_K_continue, (NODE *) NULL); }
    break;

  case 42:
#line 432 "awkgram.y"
    { NODETYPE type;

		  if (begin_or_end_rule)
			yyerror(_("`%s' used in %s action"), "next",
				(parsing_end_rule ? "END" : "BEGIN"));
		  type = Node_K_next;
		  yyval.nodeval = node((NODE *) NULL, type, (NODE *) NULL);
		}
    break;

  case 43:
#line 441 "awkgram.y"
    {
		  if (do_traditional) {
			/*
			 * can't use yyerror, since may have overshot
			 * the source line
			 */
			errcount++;
			error(_("`nextfile' is a gawk extension"));
		  }
		  if (do_lint)
			lintwarn(_("`nextfile' is a gawk extension"));
		  if (begin_or_end_rule) {
			/* same thing */
			errcount++;
			error(_("`%s' used in %s action"), "nextfile",
				(parsing_end_rule ? "END" : "BEGIN"));
		  }
		  yyval.nodeval = node((NODE *) NULL, Node_K_nextfile, (NODE *) NULL);
		}
    break;

  case 44:
#line 461 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-1].nodeval, Node_K_exit, (NODE *) NULL); }
    break;

  case 45:
#line 463 "awkgram.y"
    {
		  if (! can_return)
			yyerror(_("`return' used outside function context"));
		}
    break;

  case 46:
#line 468 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-1].nodeval, Node_K_return, (NODE *) NULL); }
    break;

  case 48:
#line 481 "awkgram.y"
    { in_print = TRUE; in_parens = 0; }
    break;

  case 49:
#line 482 "awkgram.y"
    {
		/*
		 * Optimization: plain `print' has no expression list, so $3 is null.
		 * If $3 is an expression list with one element (rnode == null)
		 * and lnode is a field spec for field 0, we have `print $0'.
		 * For both, use Node_K_print_rec, which is faster for these two cases.
		 */
		if (yyvsp[-3].nodetypeval == Node_K_print &&
		    (yyvsp[-1].nodeval == NULL
		     || (yyvsp[-1].nodeval->type == Node_expression_list
			&& yyvsp[-1].nodeval->rnode == NULL
			&& yyvsp[-1].nodeval->lnode->type == Node_field_spec
			&& yyvsp[-1].nodeval->lnode->lnode->type == Node_val
			&& yyvsp[-1].nodeval->lnode->lnode->numbr == 0.0))
		) {
			static int warned = FALSE;

			yyval.nodeval = node(NULL, Node_K_print_rec, yyvsp[0].nodeval);

			if (do_lint && yyvsp[-1].nodeval == NULL && begin_or_end_rule && ! warned) {
				warned = TRUE;
				lintwarn(
	_("plain `print' in BEGIN or END rule should probably be `print \"\"'"));
			}
		} else {
			yyval.nodeval = node(yyvsp[-1].nodeval, yyvsp[-3].nodetypeval, yyvsp[0].nodeval);
			if (yyval.nodeval->type == Node_K_printf)
				count_args(yyval.nodeval);
		}
	  }
    break;

  case 50:
#line 513 "awkgram.y"
    { yyval.nodeval = node(variable(yyvsp[-3].sval, CAN_FREE, Node_var_array), Node_K_delete, yyvsp[-1].nodeval); }
    break;

  case 51:
#line 515 "awkgram.y"
    {
		  if (do_lint)
			lintwarn(_("`delete array' is a gawk extension"));
		  if (do_traditional) {
			/*
			 * can't use yyerror, since may have overshot
			 * the source line
			 */
			errcount++;
			error(_("`delete array' is a gawk extension"));
		  }
		  yyval.nodeval = node(variable(yyvsp[0].sval, CAN_FREE, Node_var_array), Node_K_delete, (NODE *) NULL);
		}
    break;

  case 52:
#line 529 "awkgram.y"
    {
		  /* this is for tawk compatibility. maybe the warnings should always be done. */
		  if (do_lint)
			lintwarn(_("`delete(array)' is a non-portable tawk extension"));
		  if (do_traditional) {
			/*
			 * can't use yyerror, since may have overshot
			 * the source line
			 */
			errcount++;
			error(_("`delete(array)' is a non-portable tawk extension"));
		  }
		  yyval.nodeval = node(variable(yyvsp[-1].sval, CAN_FREE, Node_var_array), Node_K_delete, (NODE *) NULL);
		}
    break;

  case 53:
#line 544 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 54:
#line 549 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 55:
#line 551 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 56:
#line 556 "awkgram.y"
    {
		if (yyvsp[0].nodeval == NULL) {
			yyval.nodeval = NULL;
		} else {
			NODE *dflt = NULL;
			NODE *head = yyvsp[0].nodeval;
			NODE *curr;
	
			const char **case_values = NULL;
	
			int maxcount = 128;
			int case_count = 0;
			int i;
	
			emalloc(case_values, const char **, sizeof(char*) * maxcount, "switch_body");
			for (curr = yyvsp[0].nodeval; curr != NULL; curr = curr->rnode) {
				/* Assure that case statement values are unique. */
				if (curr->lnode->type == Node_K_case) {
					char *caseval;
	
					if (curr->lnode->lnode->type == Node_regex)
						caseval = curr->lnode->lnode->re_exp->stptr;
					else
						caseval = force_string(tree_eval(curr->lnode->lnode))->stptr;
	
					for (i = 0; i < case_count; i++)
						if (strcmp(caseval, case_values[i]) == 0)
							yyerror(_("duplicate case values in switch body: %s"), caseval);
	
					if (case_count >= maxcount) {
						maxcount += 128;
						erealloc(case_values, const char **, sizeof(char*) * maxcount, "switch_body");
					}
					case_values[case_count++] = caseval;
				} else {
					/* Otherwise save a pointer to the default node.  */
					if (dflt != NULL)
						yyerror(_("Duplicate `default' detected in switch body"));
					dflt = curr;
				}
			}
	
			free(case_values);
	
			/* Create the switch body. */
			yyval.nodeval = node(head, Node_switch_body, dflt);
		}
	}
    break;

  case 57:
#line 608 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 58:
#line 610 "awkgram.y"
    {
		if (yyvsp[0].nodeval == NULL)
			yyval.nodeval = yyvsp[-1].nodeval;
		else {
			if (do_lint && isnoeffect(yyvsp[0].nodeval->type))
				lintwarn(_("statement may have no effect"));
			if (yyvsp[-1].nodeval == NULL)
				yyval.nodeval = node(yyvsp[0].nodeval, Node_case_list, (NODE *) NULL);
			else
				yyval.nodeval = append_right(
					(yyvsp[-1].nodeval->type == Node_case_list ? yyvsp[-1].nodeval : node(yyvsp[-1].nodeval, Node_case_list, (NODE *) NULL)),
					(yyvsp[0].nodeval->type == Node_case_list ? yyvsp[0].nodeval : node(yyvsp[0].nodeval, Node_case_list, (NODE *) NULL))
				);
		}
	    	yyerrok;
	  }
    break;

  case 59:
#line 627 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 60:
#line 632 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-3].nodeval, Node_K_case, yyvsp[0].nodeval); }
    break;

  case 61:
#line 634 "awkgram.y"
    { yyval.nodeval = node((NODE *) NULL, Node_K_default, yyvsp[0].nodeval); }
    break;

  case 62:
#line 639 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 63:
#line 641 "awkgram.y"
    {
		yyvsp[0].nodeval->numbr = -(force_number(yyvsp[0].nodeval));
		yyval.nodeval = yyvsp[0].nodeval;
	  }
    break;

  case 64:
#line 646 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 65:
#line 648 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 66:
#line 650 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 70:
#line 665 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-3].nodeval, Node_expression_list, yyvsp[-1].nodeval); }
    break;

  case 71:
#line 670 "awkgram.y"
    {
		in_print = FALSE;
		in_parens = 0;
		yyval.nodeval = NULL;
	  }
    break;

  case 72:
#line 675 "awkgram.y"
    { in_print = FALSE; in_parens = 0; }
    break;

  case 73:
#line 676 "awkgram.y"
    {
		yyval.nodeval = node(yyvsp[0].nodeval, yyvsp[-2].nodetypeval, (NODE *) NULL);
		if (yyvsp[-2].nodetypeval == Node_redirect_twoway
		    && yyvsp[0].nodeval->type == Node_K_getline
		    && yyvsp[0].nodeval->rnode->type == Node_redirect_twoway)
			yyerror(_("multistage two-way pipelines don't work"));
	  }
    break;

  case 74:
#line 687 "awkgram.y"
    {
		yyval.nodeval = node(yyvsp[-3].nodeval, Node_K_if, 
			node(yyvsp[0].nodeval, Node_if_branches, (NODE *) NULL));
	  }
    break;

  case 75:
#line 693 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-6].nodeval, Node_K_if,
				node(yyvsp[-3].nodeval, Node_if_branches, yyvsp[0].nodeval)); }
    break;

  case 80:
#line 709 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 81:
#line 711 "awkgram.y"
    { yyval.nodeval = node(yyvsp[0].nodeval, Node_redirect_input, (NODE *) NULL); }
    break;

  case 82:
#line 716 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 83:
#line 718 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 84:
#line 723 "awkgram.y"
    { yyval.nodeval = make_param(yyvsp[0].sval); }
    break;

  case 85:
#line 725 "awkgram.y"
    { yyval.nodeval = append_right(yyvsp[-2].nodeval, make_param(yyvsp[0].sval)); yyerrok; }
    break;

  case 86:
#line 727 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 87:
#line 729 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 88:
#line 731 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 89:
#line 737 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 90:
#line 739 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 91:
#line 744 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 92:
#line 746 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 93:
#line 751 "awkgram.y"
    { yyval.nodeval = node(yyvsp[0].nodeval, Node_expression_list, (NODE *) NULL); }
    break;

  case 94:
#line 753 "awkgram.y"
    {
			yyval.nodeval = append_right(yyvsp[-2].nodeval,
				node(yyvsp[0].nodeval, Node_expression_list, (NODE *) NULL));
			yyerrok;
		}
    break;

  case 95:
#line 759 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 96:
#line 761 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 97:
#line 763 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 98:
#line 765 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 99:
#line 770 "awkgram.y"
    {
		  if (do_lint && yyvsp[0].nodeval->type == Node_regex)
			lintwarn(_("regular expression on right of assignment"));
		  yyval.nodeval = node(yyvsp[-2].nodeval, yyvsp[-1].nodetypeval, yyvsp[0].nodeval);
		}
    break;

  case 100:
#line 776 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_and, yyvsp[0].nodeval); }
    break;

  case 101:
#line 778 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_or, yyvsp[0].nodeval); }
    break;

  case 102:
#line 780 "awkgram.y"
    {
		  if (yyvsp[-2].nodeval->type == Node_regex)
			warning(_("regular expression on left of `~' or `!~' operator"));
		  yyval.nodeval = node(yyvsp[-2].nodeval, yyvsp[-1].nodetypeval, mk_rexp(yyvsp[0].nodeval));
		}
    break;

  case 103:
#line 786 "awkgram.y"
    { yyval.nodeval = node(variable(yyvsp[0].sval, CAN_FREE, Node_var_array), Node_in_array, yyvsp[-2].nodeval); }
    break;

  case 104:
#line 788 "awkgram.y"
    {
		  if (do_lint && yyvsp[0].nodeval->type == Node_regex)
			lintwarn(_("regular expression on right of comparison"));
		  yyval.nodeval = node(yyvsp[-2].nodeval, yyvsp[-1].nodetypeval, yyvsp[0].nodeval);
		}
    break;

  case 105:
#line 794 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-4].nodeval, Node_cond_exp, node(yyvsp[-2].nodeval, Node_if_branches, yyvsp[0].nodeval));}
    break;

  case 106:
#line 796 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 107:
#line 801 "awkgram.y"
    { yyval.nodetypeval = yyvsp[0].nodetypeval; }
    break;

  case 108:
#line 803 "awkgram.y"
    { yyval.nodetypeval = yyvsp[0].nodetypeval; }
    break;

  case 109:
#line 805 "awkgram.y"
    { yyval.nodetypeval = Node_assign_quotient; }
    break;

  case 110:
#line 810 "awkgram.y"
    { yyval.nodetypeval = yyvsp[0].nodetypeval; }
    break;

  case 111:
#line 812 "awkgram.y"
    { yyval.nodetypeval = Node_less; }
    break;

  case 113:
#line 817 "awkgram.y"
    { yyval.nodetypeval = Node_greater; }
    break;

  case 114:
#line 822 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 115:
#line 824 "awkgram.y"
    {
		  yyval.nodeval = node(node(make_number(0.0),
				 Node_field_spec,
				 (NODE *) NULL),
		            Node_nomatch,
			    yyvsp[0].nodeval);
		}
    break;

  case 116:
#line 832 "awkgram.y"
    { yyval.nodeval = node(variable(yyvsp[0].sval, CAN_FREE, Node_var_array), Node_in_array, yyvsp[-3].nodeval); }
    break;

  case 117:
#line 834 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 118:
#line 836 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-1].nodeval, Node_concat, yyvsp[0].nodeval); }
    break;

  case 120:
#line 843 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_exp, yyvsp[0].nodeval); }
    break;

  case 121:
#line 845 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_times, yyvsp[0].nodeval); }
    break;

  case 122:
#line 847 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_quotient, yyvsp[0].nodeval); }
    break;

  case 123:
#line 849 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_mod, yyvsp[0].nodeval); }
    break;

  case 124:
#line 851 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_plus, yyvsp[0].nodeval); }
    break;

  case 125:
#line 853 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-2].nodeval, Node_minus, yyvsp[0].nodeval); }
    break;

  case 126:
#line 855 "awkgram.y"
    {
		  if (do_lint && parsing_end_rule && yyvsp[0].nodeval == NULL)
			lintwarn(_("non-redirected `getline' undefined inside END action"));
		  yyval.nodeval = node(yyvsp[-1].nodeval, Node_K_getline, yyvsp[0].nodeval);
		}
    break;

  case 127:
#line 861 "awkgram.y"
    {
		  yyval.nodeval = node(yyvsp[0].nodeval, Node_K_getline,
			 node(yyvsp[-3].nodeval, yyvsp[-2].nodetypeval, (NODE *) NULL));
		}
    break;

  case 128:
#line 866 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-1].nodeval, Node_postincrement, (NODE *) NULL); }
    break;

  case 129:
#line 868 "awkgram.y"
    { yyval.nodeval = node(yyvsp[-1].nodeval, Node_postdecrement, (NODE *) NULL); }
    break;

  case 130:
#line 873 "awkgram.y"
    { yyval.nodeval = node(yyvsp[0].nodeval, Node_not, (NODE *) NULL); }
    break;

  case 131:
#line 875 "awkgram.y"
    { yyval.nodeval = yyvsp[-1].nodeval; }
    break;

  case 132:
#line 878 "awkgram.y"
    { yyval.nodeval = snode(yyvsp[-1].nodeval, Node_builtin, (int) yyvsp[-3].lval); }
    break;

  case 133:
#line 880 "awkgram.y"
    { yyval.nodeval = snode(yyvsp[-1].nodeval, Node_builtin, (int) yyvsp[-3].lval); }
    break;

  case 134:
#line 882 "awkgram.y"
    {
		if (do_lint)
			lintwarn(_("call of `length' without parentheses is not portable"));
		yyval.nodeval = snode((NODE *) NULL, Node_builtin, (int) yyvsp[0].lval);
		if (do_posix)
			warning(_("call of `length' without parentheses is deprecated by POSIX"));
	  }
    break;

  case 135:
#line 890 "awkgram.y"
    {
		yyval.nodeval = node(yyvsp[-1].nodeval, Node_func_call, make_string(yyvsp[-3].sval, strlen(yyvsp[-3].sval)));
		yyval.nodeval->funcbody = NULL;
		func_use(yyvsp[-3].sval, FUNC_USE);
		param_sanity(yyvsp[-1].nodeval);
		free(yyvsp[-3].sval);
	  }
    break;

  case 137:
#line 899 "awkgram.y"
    { yyval.nodeval = node(yyvsp[0].nodeval, Node_preincrement, (NODE *) NULL); }
    break;

  case 138:
#line 901 "awkgram.y"
    { yyval.nodeval = node(yyvsp[0].nodeval, Node_predecrement, (NODE *) NULL); }
    break;

  case 139:
#line 903 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 140:
#line 905 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 141:
#line 908 "awkgram.y"
    {
		  if (yyvsp[0].nodeval->type == Node_val && (yyvsp[0].nodeval->flags & (STRCUR|STRING)) == 0) {
			yyvsp[0].nodeval->numbr = -(force_number(yyvsp[0].nodeval));
			yyval.nodeval = yyvsp[0].nodeval;
		  } else
			yyval.nodeval = node(yyvsp[0].nodeval, Node_unary_minus, (NODE *) NULL);
		}
    break;

  case 142:
#line 916 "awkgram.y"
    {
		  /*
		   * was: $$ = $2
		   * POSIX semantics: force a conversion to numeric type
		   */
		  yyval.nodeval = node (make_number(0.0), Node_plus, yyvsp[0].nodeval);
		}
    break;

  case 143:
#line 927 "awkgram.y"
    { yyval.nodeval = NULL; }
    break;

  case 144:
#line 929 "awkgram.y"
    { yyval.nodeval = yyvsp[0].nodeval; }
    break;

  case 145:
#line 934 "awkgram.y"
    { yyval.nodeval = variable(yyvsp[0].sval, CAN_FREE, Node_var_new); }
    break;

  case 146:
#line 936 "awkgram.y"
    {
		NODE *n;

		if ((n = lookup(yyvsp[-3].sval)) != NULL && ! isarray(n))
			yyerror(_("use of non-array as array"));
		else if (yyvsp[-1].nodeval == NULL) {
			fatal(_("invalid subscript expression"));
		} else if (yyvsp[-1].nodeval->rnode == NULL) {
			yyval.nodeval = node(variable(yyvsp[-3].sval, CAN_FREE, Node_var_array), Node_subscript, yyvsp[-1].nodeval->lnode);
			freenode(yyvsp[-1].nodeval);
		} else
			yyval.nodeval = node(variable(yyvsp[-3].sval, CAN_FREE, Node_var_array), Node_subscript, yyvsp[-1].nodeval);
	  }
    break;

  case 147:
#line 950 "awkgram.y"
    { yyval.nodeval = node(yyvsp[0].nodeval, Node_field_spec, (NODE *) NULL); }
    break;

  case 149:
#line 958 "awkgram.y"
    { yyerrok; }
    break;

  case 150:
#line 962 "awkgram.y"
    { yyerrok; }
    break;

  case 153:
#line 971 "awkgram.y"
    { yyerrok; }
    break;

  case 154:
#line 975 "awkgram.y"
    { yyerrok; }
    break;

  case 155:
#line 978 "awkgram.y"
    { yyerrok; }
    break;


    }

/* Line 991 of yacc.c.  */
#line 2604 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
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



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
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
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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


#line 981 "awkgram.y"


struct token {
	const char *operator;		/* text to match */
	NODETYPE value;		/* node type */
	int class;		/* lexical class */
	unsigned flags;		/* # of args. allowed and compatability */
#	define	ARGS	0xFF	/* 0, 1, 2, 3 args allowed (any combination */
#	define	A(n)	(1<<(n))
#	define	VERSION_MASK	0xFF00	/* old awk is zero */
#	define	NOT_OLD		0x0100	/* feature not in old awk */
#	define	NOT_POSIX	0x0200	/* feature not in POSIX */
#	define	GAWKX		0x0400	/* gawk extension */
#	define	RESX		0x0800	/* Bell Labs Research extension */
	NODE *(*ptr) P((NODE *));	/* function that implements this keyword */
};

/* Tokentab is sorted ascii ascending order, so it can be binary searched. */
/* Function pointers come from declarations in awk.h. */

static const struct token tokentab[] = {
{"BEGIN",	Node_illegal,	 LEX_BEGIN,	0,		0},
{"END",		Node_illegal,	 LEX_END,	0,		0},
#ifdef ARRAYDEBUG
{"adump",	Node_builtin,    LEX_BUILTIN,	GAWKX|A(1),	do_adump},
#endif
{"and",		Node_builtin,    LEX_BUILTIN,	GAWKX|A(2),	do_and},
{"asort",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(1)|A(2),	do_asort},
{"asorti",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(1)|A(2),	do_asorti},
{"atan2",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(2),	do_atan2},
{"bindtextdomain",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(1)|A(2),	do_bindtextdomain},
{"break",	Node_K_break,	 LEX_BREAK,	0,		0},
#ifdef ALLOW_SWITCH
{"case",	Node_K_case,	 LEX_CASE,	GAWKX,		0},
#endif
{"close",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(1)|A(2),	do_close},
{"compl",	Node_builtin,    LEX_BUILTIN,	GAWKX|A(1),	do_compl},
{"continue",	Node_K_continue, LEX_CONTINUE,	0,		0},
{"cos",		Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(1),	do_cos},
{"dcgettext",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(1)|A(2)|A(3),	do_dcgettext},
{"dcngettext",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(1)|A(2)|A(3)|A(4)|A(5),	do_dcngettext},
#ifdef ALLOW_SWITCH
{"default",	Node_K_default,	 LEX_DEFAULT,	GAWKX,		0},
#endif
{"delete",	Node_K_delete,	 LEX_DELETE,	NOT_OLD,	0},
{"do",		Node_K_do,	 LEX_DO,	NOT_OLD,	0},
{"else",	Node_illegal,	 LEX_ELSE,	0,		0},
{"exit",	Node_K_exit,	 LEX_EXIT,	0,		0},
{"exp",		Node_builtin,	 LEX_BUILTIN,	A(1),		do_exp},
{"extension",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(2),	do_ext},
{"fflush",	Node_builtin,	 LEX_BUILTIN,	RESX|A(0)|A(1), do_fflush},
{"for",		Node_K_for,	 LEX_FOR,	0,		0},
{"func",	Node_K_function, LEX_FUNCTION,	NOT_POSIX|NOT_OLD,	0},
{"function",	Node_K_function, LEX_FUNCTION,	NOT_OLD,	0},
{"gensub",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(3)|A(4), do_gensub},
{"getline",	Node_K_getline,	 LEX_GETLINE,	NOT_OLD,	0},
{"gsub",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(2)|A(3), do_gsub},
{"if",		Node_K_if,	 LEX_IF,	0,		0},
{"in",		Node_illegal,	 LEX_IN,	0,		0},
{"index",	Node_builtin,	 LEX_BUILTIN,	A(2),		do_index},
{"int",		Node_builtin,	 LEX_BUILTIN,	A(1),		do_int},
{"length",	Node_builtin,	 LEX_LENGTH,	A(0)|A(1),	do_length},
{"log",		Node_builtin,	 LEX_BUILTIN,	A(1),		do_log},
{"lshift",	Node_builtin,    LEX_BUILTIN,	GAWKX|A(2),	do_lshift},
{"match",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(2)|A(3), do_match},
{"mktime",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(1),	do_mktime},
{"next",	Node_K_next,	 LEX_NEXT,	0,		0},
{"nextfile",	Node_K_nextfile, LEX_NEXTFILE,	GAWKX,		0},
{"or",		Node_builtin,    LEX_BUILTIN,	GAWKX|A(2),	do_or},
{"print",	Node_K_print,	 LEX_PRINT,	0,		0},
{"printf",	Node_K_printf,	 LEX_PRINTF,	0,		0},
{"rand",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(0),	do_rand},
{"return",	Node_K_return,	 LEX_RETURN,	NOT_OLD,	0},
{"rshift",	Node_builtin,    LEX_BUILTIN,	GAWKX|A(2),	do_rshift},
{"sin",		Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(1),	do_sin},
{"split",	Node_builtin,	 LEX_BUILTIN,	A(2)|A(3),	do_split},
{"sprintf",	Node_builtin,	 LEX_BUILTIN,	0,		do_sprintf},
{"sqrt",	Node_builtin,	 LEX_BUILTIN,	A(1),		do_sqrt},
{"srand",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(0)|A(1), do_srand},
#if defined(GAWKDEBUG) || defined(ARRAYDEBUG) /* || ... */
{"stopme",	Node_builtin,    LEX_BUILTIN,	GAWKX|A(0),	stopme},
#endif
{"strftime",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(0)|A(1)|A(2), do_strftime},
{"strtonum",	Node_builtin,    LEX_BUILTIN,	GAWKX|A(1),	do_strtonum},
{"sub",		Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(2)|A(3), do_sub},
{"substr",	Node_builtin,	 LEX_BUILTIN,	A(2)|A(3),	do_substr},
#ifdef ALLOW_SWITCH
{"switch",	Node_K_switch,	 LEX_SWITCH,	GAWKX,		0},
#endif
{"system",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(1),	do_system},
{"systime",	Node_builtin,	 LEX_BUILTIN,	GAWKX|A(0),	do_systime},
{"tolower",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(1),	do_tolower},
{"toupper",	Node_builtin,	 LEX_BUILTIN,	NOT_OLD|A(1),	do_toupper},
{"while",	Node_K_while,	 LEX_WHILE,	0,		0},
{"xor",		Node_builtin,    LEX_BUILTIN,	GAWKX|A(2),	do_xor},
};

#ifdef MBS_SUPPORT
/* Variable containing the current shift state.  */
static mbstate_t cur_mbstate;
/* Ring buffer containing current characters.  */
#define MAX_CHAR_IN_RING_BUFFER 8
#define RING_BUFFER_SIZE (MAX_CHAR_IN_RING_BUFFER * MB_LEN_MAX)
static char cur_char_ring[RING_BUFFER_SIZE];
/* Index for ring buffers.  */
static int cur_ring_idx;
/* This macro means that last nextc() return a singlebyte character
   or 1st byte of a multibyte character.  */
#define nextc_is_1stbyte (cur_char_ring[cur_ring_idx] == 1)
#endif /* MBS_SUPPORT */

/* getfname --- return name of a builtin function (for pretty printing) */

const char *
getfname(register NODE *(*fptr)(NODE *))
{
	register int i, j;

	j = sizeof(tokentab) / sizeof(tokentab[0]);
	/* linear search, no other way to do it */
	for (i = 0; i < j; i++) 
		if (tokentab[i].ptr == fptr)
			return tokentab[i].operator;

	return NULL;
}

/* yyerror --- print a syntax error message, show where */

/*
 * Function identifier purposely indented to avoid mangling
 * by ansi2knr.  Sigh.
 */

static void
#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
  yyerror(const char *m, ...)
#else
/* VARARGS0 */
  yyerror(va_alist)
  va_dcl
#endif
{
	va_list args;
	const char *mesg = NULL;
	register char *bp, *cp;
	char *scan;
	char *buf;
	int count;
	static char end_of_file_line[] = "(END OF FILE)";
	char save;

	errcount++;
	/* Find the current line in the input file */
	if (lexptr && lexeme) {
		if (thisline == NULL) {
			cp = lexeme;
			if (*cp == '\n') {
				cp--;
				mesg = _("unexpected newline or end of string");
			}
			for (; cp != lexptr_begin && *cp != '\n'; --cp)
				continue;
			if (*cp == '\n')
				cp++;
			thisline = cp;
		}
		/* NL isn't guaranteed */
		bp = lexeme;
		while (bp < lexend && *bp && *bp != '\n')
			bp++;
	} else {
		thisline = end_of_file_line;
		bp = thisline + strlen(thisline);
	}

	/*
	 * Saving and restoring *bp keeps valgrind happy,
	 * since the guts of glibc uses strlen, even though
	 * we're passing an explict precision. Sigh.
	 */
	save = *bp;
	*bp = '\0';

	msg("%.*s", (int) (bp - thisline), thisline);

	*bp = save;

#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
	va_start(args, m);
	if (mesg == NULL)
		mesg = m;
#else
	va_start(args);
	if (mesg == NULL)
		mesg = va_arg(args, char *);
#endif
	count = (bp - thisline) + strlen(mesg) + 2 + 1;
	emalloc(buf, char *, count, "yyerror");

	bp = buf;

	if (lexptr != NULL) {
		scan = thisline;
		while (scan < lexeme)
			if (*scan++ == '\t')
				*bp++ = '\t';
			else
				*bp++ = ' ';
		*bp++ = '^';
		*bp++ = ' ';
	}
	strcpy(bp, mesg);
	err("", buf, args);
	va_end(args);
	free(buf);
}

/* get_src_buf --- read the next buffer of source program */

static char *
get_src_buf()
{
	static int samefile = FALSE;
	static int nextfile = 0;
	static char *buf = NULL;
	static int fd;
	int n;
	register char *scan;
	static size_t len = 0;
	static int did_newline = FALSE;
	int newfile;
	struct stat sbuf;

#	define	SLOP	128	/* enough space to hold most source lines */

again:
	newfile = FALSE;
	if (nextfile > numfiles)
		return NULL;

	if (srcfiles[nextfile].stype == CMDLINE) {
		if (len == 0) {
			len = strlen(srcfiles[nextfile].val);
			if (len == 0) {
				/*
				 * Yet Another Special case:
				 *	gawk '' /path/name
				 * Sigh.
				 */
				static int warned = FALSE;

				if (do_lint && ! warned) {
					warned = TRUE;
					lintwarn(_("empty program text on command line"));
				}
				++nextfile;
				goto again;
			}
			sourceline = 1;
			lexptr = lexptr_begin = srcfiles[nextfile].val;
			lexend = lexptr + len;
		} else if (! did_newline && *(lexptr-1) != '\n') {
			/*
			 * The following goop is to ensure that the source
			 * ends with a newline and that the entire current
			 * line is available for error messages.
			 */
			int offset;

			did_newline = TRUE;
			offset = lexptr - lexeme;
			for (scan = lexeme; scan > lexptr_begin; scan--)
				if (*scan == '\n') {
					scan++;
					break;
				}
			len = lexptr - scan;
			emalloc(buf, char *, len+1, "get_src_buf");
			memcpy(buf, scan, len);
			thisline = buf;
			lexptr = buf + len;
			*lexptr = '\n';
			lexeme = lexptr - offset;
			lexptr_begin = buf;
			lexend = lexptr + 1;
		} else {
			len = 0;
			lexeme = lexptr = lexptr_begin = NULL;
		}
		if (lexptr == NULL && ++nextfile <= numfiles)
			goto again;
		return lexptr;
	}
	if (! samefile) {
		source = srcfiles[nextfile].val;
		if (source == NULL) {
			if (buf != NULL) {
				free(buf);
				buf = NULL;
			}
			len = 0;
			return lexeme = lexptr = lexptr_begin = NULL;
		}
		fd = pathopen(source);
		if (fd <= INVALID_HANDLE) {
			char *in;

			/* suppress file name and line no. in error mesg */
			in = source;
			source = NULL;
			fatal(_("can't open source file `%s' for reading (%s)"),
				in, strerror(errno));
		}
		len = optimal_bufsize(fd, & sbuf);
		newfile = TRUE;
		if (buf != NULL)
			free(buf);
		emalloc(buf, char *, len + SLOP, "get_src_buf");
		lexptr_begin = buf + SLOP;
		samefile = TRUE;
		sourceline = 1;
	} else {
		/*
		 * Here, we retain the current source line (up to length SLOP)
		 * in the beginning of the buffer that was overallocated above
		 */
		int offset;
		int linelen;

		offset = lexptr - lexeme;
		for (scan = lexeme; scan > lexptr_begin; scan--)
			if (*scan == '\n') {
				scan++;
				break;
			}
		linelen = lexptr - scan;
		if (linelen > SLOP)
			linelen = SLOP;
		thisline = buf + SLOP - linelen;
		memcpy(thisline, scan, linelen);
		lexeme = buf + SLOP - offset;
		lexptr_begin = thisline;
	}
	n = read(fd, buf + SLOP, len);
	if (n == -1)
		fatal(_("can't read sourcefile `%s' (%s)"),
			source, strerror(errno));
	if (n == 0) {
		if (newfile) {
			static int warned = FALSE;

			if (do_lint && ! warned) {
				warned = TRUE;
				lintwarn(_("source file `%s' is empty"), source);
			}
		}
		if (fd != fileno(stdin)) /* safety */
			close(fd);
		samefile = FALSE;
		nextfile++;
		if (lexeme)
			*lexeme = '\0';
		len = 0;
		goto again;
	}
	lexptr = buf + SLOP;
	lexend = lexptr + n;
	return buf;
}

/* tokadd --- add a character to the token buffer */

#define	tokadd(x) (*tok++ = (x), tok == tokend ? tokexpand() : tok)

/* tokexpand --- grow the token buffer */

char *
tokexpand()
{
	static int toksize = 60;
	int tokoffset;

	tokoffset = tok - tokstart;
	toksize *= 2;
	if (tokstart != NULL)
		erealloc(tokstart, char *, toksize, "tokexpand");
	else
		emalloc(tokstart, char *, toksize, "tokexpand");
	tokend = tokstart + toksize;
	tok = tokstart + tokoffset;
	return tok;
}

/* nextc --- get the next input character */

#ifdef MBS_SUPPORT

static int
nextc(void)
{
	if (gawk_mb_cur_max > 1)	{
		/* Update the buffer index.  */
		cur_ring_idx = (cur_ring_idx == RING_BUFFER_SIZE - 1)? 0 :
			cur_ring_idx + 1;

		/* Did we already check the current character?  */
		if (cur_char_ring[cur_ring_idx] == 0) {
			/* No, we need to check the next character on the buffer.  */
			int idx, work_ring_idx = cur_ring_idx;
			mbstate_t tmp_state;
			size_t mbclen;

			if (!lexptr || lexptr >= lexend)
				if (!get_src_buf()) {
					return EOF;
				}

			for (idx = 0 ; lexptr + idx < lexend ; idx++) {
				tmp_state = cur_mbstate;
				mbclen = mbrlen(lexptr, idx + 1, &tmp_state);

				if (mbclen == 1 || mbclen == (size_t)-1 || mbclen == 0) {
					/* It is a singlebyte character, non-complete multibyte
					   character or EOF.  We treat it as a singlebyte
					   character.  */
					cur_char_ring[work_ring_idx] = 1;
					break;
				} else if (mbclen == (size_t)-2) {
					/* It is not a complete multibyte character.  */
					cur_char_ring[work_ring_idx] = idx + 1;
				} else {
					/* mbclen > 1 */
					cur_char_ring[work_ring_idx] = mbclen;
					break;
				}
				work_ring_idx = (work_ring_idx == RING_BUFFER_SIZE - 1)?
					0 : work_ring_idx + 1;
			}
			cur_mbstate = tmp_state;

			/* Put a mark on the position on which we write next character.  */
			work_ring_idx = (work_ring_idx == RING_BUFFER_SIZE - 1)?
				0 : work_ring_idx + 1;
			cur_char_ring[work_ring_idx] = 0;
		}

		return (int) (unsigned char) *lexptr++;
	}
	else {
		int c;

		if (lexptr && lexptr < lexend)
			c = (int) (unsigned char) *lexptr++;
		else if (get_src_buf())
			c = (int) (unsigned char) *lexptr++;
		else
			c = EOF;

		return c;
	}
}

#else /* MBS_SUPPORT */

#if GAWKDEBUG
int
nextc(void)
{
	int c;

	if (lexptr && lexptr < lexend)
		c = (int) (unsigned char) *lexptr++;
	else if (get_src_buf())
		c = (int) (unsigned char) *lexptr++;
	else
		c = EOF;

	return c;
}
#else
#define	nextc()	((lexptr && lexptr < lexend) ? \
		    ((int) (unsigned char) *lexptr++) : \
		    (get_src_buf() ? ((int) (unsigned char) *lexptr++) : EOF) \
		)
#endif

#endif /* MBS_SUPPORT */

/* pushback --- push a character back on the input */

#ifdef MBS_SUPPORT

static void
pushback(void)
{
	if (gawk_mb_cur_max > 1) {
		cur_ring_idx = (cur_ring_idx == 0)? RING_BUFFER_SIZE - 1 :
			cur_ring_idx - 1;
		(lexptr && lexptr > lexptr_begin ? lexptr-- : lexptr);
	} else
		(lexptr && lexptr > lexptr_begin ? lexptr-- : lexptr);
}

#else

#define pushback() (lexptr && lexptr > lexptr_begin ? lexptr-- : lexptr)

#endif /* MBS_SUPPORT */

/* allow_newline --- allow newline after &&, ||, ? and : */

static void
allow_newline(void)
{
	int c;

	for (;;) {
		c = nextc();
		if (c == EOF)
			break;
		if (c == '#') {
			while ((c = nextc()) != '\n' && c != EOF)
				continue;
			if (c == EOF)
				break;
		}
		if (c == '\n')
			sourceline++;
		if (! ISSPACE(c)) {
			pushback();
			break;
		}
	}
}

/* yylex --- Read the input and turn it into tokens. */

static int
yylex(void)
{
	register int c;
	int seen_e = FALSE;		/* These are for numbers */
	int seen_point = FALSE;
	int esc_seen;		/* for literal strings */
	int low, mid, high;
	static int did_newline = FALSE;
	char *tokkey;
	static int lasttok = 0, eof_warned = FALSE;
	int inhex = FALSE;
	int intlstr = FALSE;

	if (nextc() == EOF) {
		if (lasttok != NEWLINE) {
			lasttok = NEWLINE;
			if (do_lint && ! eof_warned) {
				lintwarn(_("source file does not end in newline"));
				eof_warned = TRUE;
			}
			return NEWLINE;	/* fake it */
		}
		return 0;
	}
	pushback();
#if defined OS2 || defined __EMX__
	/*
	 * added for OS/2's extproc feature of cmd.exe
	 * (like #! in BSD sh)
	 */
	if (strncasecmp(lexptr, "extproc ", 8) == 0) {
		while (*lexptr && *lexptr != '\n')
			lexptr++;
	}
#endif
	lexeme = lexptr;
	thisline = NULL;
	if (want_regexp) {
		int in_brack = 0;	/* count brackets, [[:alnum:]] allowed */
		/*
		 * Counting brackets is non-trivial. [[] is ok,
		 * and so is [\]], with a point being that /[/]/ as a regexp
		 * constant has to work.
		 *
		 * Do not count [ or ] if either one is preceded by a \.
		 * A `[' should be counted if
		 *  a) it is the first one so far (in_brack == 0)
		 *  b) it is the `[' in `[:'
		 * A ']' should be counted if not preceded by a \, since
		 * it is either closing `:]' or just a plain list.
		 * According to POSIX, []] is how you put a ] into a set.
		 * Try to handle that too.
		 *
		 * The code for \ handles \[ and \].
		 */

		want_regexp = FALSE;
		tok = tokstart;
		for (;;) {
			c = nextc();
#ifdef MBS_SUPPORT
			if (gawk_mb_cur_max == 1 || nextc_is_1stbyte)
#endif
			switch (c) {
			case '[':
				/* one day check for `.' and `=' too */
				if (nextc() == ':' || in_brack == 0)
					in_brack++;
				pushback();
				break;
			case ']':
				if (tokstart[0] == '['
				    && (tok == tokstart + 1
					|| (tok == tokstart + 2
					    && tokstart[1] == '^')))
					/* do nothing */;
				else
					in_brack--;
				break;
			case '\\':
				if ((c = nextc()) == EOF) {
					yyerror(_("unterminated regexp ends with `\\' at end of file"));
					goto end_regexp; /* kludge */
				} else if (c == '\n') {
					sourceline++;
					continue;
				} else {
					tokadd('\\');
					tokadd(c);
					continue;
				}
				break;
			case '/':	/* end of the regexp */
				if (in_brack > 0)
					break;
end_regexp:
				tokadd('\0');
				yylval.sval = tokstart;
				return lasttok = REGEXP;
			case '\n':
				pushback();
				yyerror(_("unterminated regexp"));
				goto end_regexp;	/* kludge */
			case EOF:
				yyerror(_("unterminated regexp at end of file"));
				goto end_regexp;	/* kludge */
			}
			tokadd(c);
		}
	}
retry:
	while ((c = nextc()) == ' ' || c == '\t')
		continue;

	lexeme = lexptr ? lexptr - 1 : lexptr;
	thisline = NULL;
	tok = tokstart;
	yylval.nodetypeval = Node_illegal;

#ifdef MBS_SUPPORT
	if (gawk_mb_cur_max == 1 || nextc_is_1stbyte)
#endif
	switch (c) {
	case EOF:
		if (lasttok != NEWLINE) {
			lasttok = NEWLINE;
			if (do_lint && ! eof_warned) {
				lintwarn(_("source file does not end in newline"));
				eof_warned = TRUE;
			}
			return NEWLINE;	/* fake it */
		}
		return 0;

	case '\n':
		sourceline++;
		return lasttok = NEWLINE;

	case '#':		/* it's a comment */
		while ((c = nextc()) != '\n') {
			if (c == EOF) {
				if (lasttok != NEWLINE) {
					lasttok = NEWLINE;
					if (do_lint && ! eof_warned) {
						lintwarn(
				_("source file does not end in newline"));
						eof_warned = TRUE;
					}
					return NEWLINE;	/* fake it */
				}
				return 0;
			}
		}
		sourceline++;
		return lasttok = NEWLINE;

	case '\\':
#ifdef RELAXED_CONTINUATION
		/*
		 * This code puports to allow comments and/or whitespace
		 * after the `\' at the end of a line used for continuation.
		 * Use it at your own risk. We think it's a bad idea, which
		 * is why it's not on by default.
		 */
		if (! do_traditional) {
			/* strip trailing white-space and/or comment */
			while ((c = nextc()) == ' ' || c == '\t')
				continue;
			if (c == '#') {
				if (do_lint)
					lintwarn(
		_("use of `\\ #...' line continuation is not portable"));
				while ((c = nextc()) != '\n')
					if (c == EOF)
						break;
			}
			pushback();
		}
#endif /* RELAXED_CONTINUATION */
		if (nextc() == '\n') {
			sourceline++;
			goto retry;
		} else {
			yyerror(_("backslash not last character on line"));
			exit(1);
		}
		break;

	case ':':
	case '?':
		if (! do_posix)
			allow_newline();
		return lasttok = c;

		/*
		 * in_parens is undefined unless we are parsing a print
		 * statement (in_print), but why bother with a check?
		 */
	case ')':
		in_parens--;
		return lasttok = c;

	case '(':	
		in_parens++;
		/* FALL THROUGH */
	case '$':
	case ';':
	case '{':
	case ',':
	case '[':
	case ']':
		return lasttok = c;

	case '*':
		if ((c = nextc()) == '=') {
			yylval.nodetypeval = Node_assign_times;
			return lasttok = ASSIGNOP;
		} else if (do_posix) {
			pushback();
			return lasttok = '*';
		} else if (c == '*') {
			/* make ** and **= aliases for ^ and ^= */
			static int did_warn_op = FALSE, did_warn_assgn = FALSE;

			if (nextc() == '=') {
				if (! did_warn_assgn) {
					did_warn_assgn = TRUE;
					if (do_lint)
						lintwarn(_("POSIX does not allow operator `**='"));
					if (do_lint_old)
						warning(_("old awk does not support operator `**='"));
				}
				yylval.nodetypeval = Node_assign_exp;
				return ASSIGNOP;
			} else {
				pushback();
				if (! did_warn_op) {
					did_warn_op = TRUE;
					if (do_lint)
						lintwarn(_("POSIX does not allow operator `**'"));
					if (do_lint_old)
						warning(_("old awk does not support operator `**'"));
				}
				return lasttok = '^';
			}
		}
		pushback();
		return lasttok = '*';

	case '/':
		if (nextc() == '=') {
			pushback();
			return lasttok = SLASH_BEFORE_EQUAL;
		}
		pushback();
		return lasttok = '/';

	case '%':
		if (nextc() == '=') {
			yylval.nodetypeval = Node_assign_mod;
			return lasttok = ASSIGNOP;
		}
		pushback();
		return lasttok = '%';

	case '^':
	{
		static int did_warn_op = FALSE, did_warn_assgn = FALSE;

		if (nextc() == '=') {
			if (do_lint_old && ! did_warn_assgn) {
				did_warn_assgn = TRUE;
				warning(_("operator `^=' is not supported in old awk"));
			}
			yylval.nodetypeval = Node_assign_exp;
			return lasttok = ASSIGNOP;
		}
		pushback();
		if (do_lint_old && ! did_warn_op) {
			did_warn_op = TRUE;
			warning(_("operator `^' is not supported in old awk"));
		}
		return lasttok = '^';
	}

	case '+':
		if ((c = nextc()) == '=') {
			yylval.nodetypeval = Node_assign_plus;
			return lasttok = ASSIGNOP;
		}
		if (c == '+')
			return lasttok = INCREMENT;
		pushback();
		return lasttok = '+';

	case '!':
		if ((c = nextc()) == '=') {
			yylval.nodetypeval = Node_notequal;
			return lasttok = RELOP;
		}
		if (c == '~') {
			yylval.nodetypeval = Node_nomatch;
			return lasttok = MATCHOP;
		}
		pushback();
		return lasttok = '!';

	case '<':
		if (nextc() == '=') {
			yylval.nodetypeval = Node_leq;
			return lasttok = RELOP;
		}
		yylval.nodetypeval = Node_less;
		pushback();
		return lasttok = '<';

	case '=':
		if (nextc() == '=') {
			yylval.nodetypeval = Node_equal;
			return lasttok = RELOP;
		}
		yylval.nodetypeval = Node_assign;
		pushback();
		return lasttok = ASSIGN;

	case '>':
		if ((c = nextc()) == '=') {
			yylval.nodetypeval = Node_geq;
			return lasttok = RELOP;
		} else if (c == '>') {
			yylval.nodetypeval = Node_redirect_append;
			return lasttok = IO_OUT;
		}
		pushback();
		if (in_print && in_parens == 0) {
			yylval.nodetypeval = Node_redirect_output;
			return lasttok = IO_OUT;
		}
		yylval.nodetypeval = Node_greater;
		return lasttok = '>';

	case '~':
		yylval.nodetypeval = Node_match;
		return lasttok = MATCHOP;

	case '}':
		/*
		 * Added did newline stuff.  Easier than
		 * hacking the grammar.
		 */
		if (did_newline) {
			did_newline = FALSE;
			return lasttok = c;
		}
		did_newline++;
		--lexptr;	/* pick up } next time */
		return lasttok = NEWLINE;

	case '"':
	string:
		esc_seen = FALSE;
		while ((c = nextc()) != '"') {
			if (c == '\n') {
				pushback();
				yyerror(_("unterminated string"));
				exit(1);
			}
#ifdef MBS_SUPPORT
			if (gawk_mb_cur_max == 1 || nextc_is_1stbyte)
#endif
			if (c == '\\') {
				c = nextc();
				if (c == '\n') {
					sourceline++;
					continue;
				}
				esc_seen = TRUE;
				tokadd('\\');
			}
			if (c == EOF) {
				pushback();
				yyerror(_("unterminated string"));
				exit(1);
			}
			tokadd(c);
		}
		yylval.nodeval = make_str_node(tokstart,
					tok - tokstart, esc_seen ? SCAN : 0);
		yylval.nodeval->flags |= PERM;
		if (intlstr) {
			yylval.nodeval->flags |= INTLSTR;
			intlstr = FALSE;
			if (do_intl)
				dumpintlstr(yylval.nodeval->stptr,
						yylval.nodeval->stlen);
 		}
		return lasttok = YSTRING;

	case '-':
		if ((c = nextc()) == '=') {
			yylval.nodetypeval = Node_assign_minus;
			return lasttok = ASSIGNOP;
		}
		if (c == '-')
			return lasttok = DECREMENT;
		pushback();
		return lasttok = '-';

	case '.':
		c = nextc();
		pushback();
		if (! ISDIGIT(c))
			return lasttok = '.';
		else
			c = '.';
		/* FALL THROUGH */
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		/* It's a number */
		for (;;) {
			int gotnumber = FALSE;

			tokadd(c);
			switch (c) {
			case 'x':
			case 'X':
				if (do_traditional)
					goto done;
				if (tok == tokstart + 2)
					inhex = TRUE;
				break;
			case '.':
				if (seen_point) {
					gotnumber = TRUE;
					break;
				}
				seen_point = TRUE;
				break;
			case 'e':
			case 'E':
				if (inhex)
					break;
				if (seen_e) {
					gotnumber = TRUE;
					break;
				}
				seen_e = TRUE;
				if ((c = nextc()) == '-' || c == '+')
					tokadd(c);
				else
					pushback();
				break;
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			case 'c':
			case 'C':
			case 'D':
			case 'd':
			case 'f':
			case 'F':
				if (do_traditional || ! inhex)
					goto done;
				/* fall through */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				break;
			default:
			done:
				gotnumber = TRUE;
			}
			if (gotnumber)
				break;
			c = nextc();
		}
		if (c != EOF)
			pushback();
		else if (do_lint && ! eof_warned) {
			lintwarn(_("source file does not end in newline"));
			eof_warned = TRUE;
		}
		tokadd('\0');
		if (! do_traditional && isnondecimal(tokstart)) {
			static short warned = FALSE;
			if (do_lint && ! warned) {
				warned = TRUE;
				lintwarn("numeric constant `%.*s' treated as octal or hexadecimal",
					strlen(tokstart)-1, tokstart);
			}
			yylval.nodeval = make_number(nondec2awknum(tokstart, strlen(tokstart)));
		} else
			yylval.nodeval = make_number(atof(tokstart));
		yylval.nodeval->flags |= PERM;
		return lasttok = YNUMBER;

	case '&':
		if ((c = nextc()) == '&') {
			yylval.nodetypeval = Node_and;
			allow_newline();
			return lasttok = LEX_AND;
		}
		pushback();
		return lasttok = '&';

	case '|':
		if ((c = nextc()) == '|') {
			yylval.nodetypeval = Node_or;
			allow_newline();
			return lasttok = LEX_OR;
		} else if (! do_traditional && c == '&') {
			yylval.nodetypeval = Node_redirect_twoway;
			return lasttok = (in_print && in_parens == 0 ? IO_OUT : IO_IN);
		}
		pushback();
		if (in_print && in_parens == 0) {
			yylval.nodetypeval = Node_redirect_pipe;
			return lasttok = IO_OUT;
		} else {
			yylval.nodetypeval = Node_redirect_pipein;
			return lasttok = IO_IN;
		}
	}

	if (c != '_' && ! ISALPHA(c)) {
		yyerror(_("invalid char '%c' in expression"), c);
		exit(1);
	}

	/*
	 * Lots of fog here.  Consider:
	 *
	 * print "xyzzy"$_"foo"
	 *
	 * Without the check for ` lasttok != '$'' ', this is parsed as
	 *
	 * print "xxyzz" $(_"foo")
	 *
	 * With the check, it is "correctly" parsed as three
	 * string concatenations.  Sigh.  This seems to be
	 * "more correct", but this is definitely one of those
	 * occasions where the interactions are funny.
	 */
	if (! do_traditional && c == '_' && lasttok != '$') {
		if ((c = nextc()) == '"') {
			intlstr = TRUE;
			goto string;
		}
		pushback();
		c = '_';
	}

	/* it's some type of name-type-thing.  Find its length. */
	tok = tokstart;
	while (is_identchar(c)) {
		tokadd(c);
		c = nextc();
	}
	tokadd('\0');
	emalloc(tokkey, char *, tok - tokstart, "yylex");
	memcpy(tokkey, tokstart, tok - tokstart);
	if (c != EOF)
		pushback();
	else if (do_lint && ! eof_warned) {
		lintwarn(_("source file does not end in newline"));
		eof_warned = TRUE;
	}

	/* See if it is a special token. */
	low = 0;
	high = (sizeof(tokentab) / sizeof(tokentab[0])) - 1;
	while (low <= high) {
		int i;

		mid = (low + high) / 2;
		c = *tokstart - tokentab[mid].operator[0];
		i = c ? c : strcmp(tokstart, tokentab[mid].operator);

		if (i < 0)		/* token < mid */
			high = mid - 1;
		else if (i > 0)		/* token > mid */
			low = mid + 1;
		else {
			if (do_lint) {
				if (tokentab[mid].flags & GAWKX)
					lintwarn(_("`%s' is a gawk extension"),
						tokentab[mid].operator);
				if (tokentab[mid].flags & RESX)
					lintwarn(_("`%s' is a Bell Labs extension"),
						tokentab[mid].operator);
				if (tokentab[mid].flags & NOT_POSIX)
					lintwarn(_("POSIX does not allow `%s'"),
						tokentab[mid].operator);
			}
			if (do_lint_old && (tokentab[mid].flags & NOT_OLD))
				warning(_("`%s' is not supported in old awk"),
						tokentab[mid].operator);
			if ((do_traditional && (tokentab[mid].flags & GAWKX))
			    || (do_posix && (tokentab[mid].flags & NOT_POSIX)))
				break;
			if (tokentab[mid].class == LEX_BUILTIN
			    || tokentab[mid].class == LEX_LENGTH
			   )
				yylval.lval = mid;
			else
				yylval.nodetypeval = tokentab[mid].value;

			free(tokkey);
			return lasttok = tokentab[mid].class;
		}
	}

	yylval.sval = tokkey;
	if (*lexptr == '(')
		return lasttok = FUNC_CALL;
	else {
		static short goto_warned = FALSE;

#define SMART_ALECK	1
		if (SMART_ALECK && do_lint
		    && ! goto_warned && strcasecmp(tokkey, "goto") == 0) {
			goto_warned = TRUE;
			lintwarn(_("`goto' considered harmful!\n"));
		}
		return lasttok = NAME;
	}
}

/* node_common --- common code for allocating a new node */

static NODE *
node_common(NODETYPE op)
{
	register NODE *r;

	getnode(r);
	r->type = op;
	r->flags = MALLOC;
	/* if lookahead is NL, lineno is 1 too high */
	if (lexeme && *lexeme == '\n')
		r->source_line = sourceline - 1;
	else
		r->source_line = sourceline;
	r->source_file = source;
	return r;
}

/* node --- allocates a node with defined lnode and rnode. */

NODE *
node(NODE *left, NODETYPE op, NODE *right)
{
	register NODE *r;

	r = node_common(op);
	r->lnode = left;
	r->rnode = right;
	return r;
}

/* snode ---	allocate a node with defined subnode and builtin for builtin
		functions. Checks for arg. count and supplies defaults where
		possible. */

static NODE *
snode(NODE *subn, NODETYPE op, int idx)
{
	register NODE *r;
	register NODE *n;
	int nexp = 0;
	int args_allowed;

	r = node_common(op);

	/* traverse expression list to see how many args. given */
	for (n = subn; n != NULL; n = n->rnode) {
		nexp++;
		if (nexp > 5)
			break;
	}

	/* check against how many args. are allowed for this builtin */
	args_allowed = tokentab[idx].flags & ARGS;
	if (args_allowed && (args_allowed & A(nexp)) == 0)
		fatal(_("%d is invalid as number of arguments for %s"),
				nexp, tokentab[idx].operator);

	r->builtin = tokentab[idx].ptr;

	/* special case processing for a few builtins */
	if (nexp == 0 && r->builtin == do_length) {
		subn = node(node(make_number(0.0), Node_field_spec, (NODE *) NULL),
		            Node_expression_list,
			    (NODE *) NULL);
	} else if (r->builtin == do_match) {
		static short warned = FALSE;

		if (subn->rnode->lnode->type != Node_regex)
			subn->rnode->lnode = mk_rexp(subn->rnode->lnode);

		if (subn->rnode->rnode != NULL) {	/* 3rd argument there */
			if (do_lint && ! warned) {
				warned = TRUE;
				lintwarn(_("match: third argument is a gawk extension"));
			}
			if (do_traditional)
				fatal(_("match: third argument is a gawk extension"));
		}
	} else if (r->builtin == do_sub || r->builtin == do_gsub) {
		if (subn->lnode->type != Node_regex)
			subn->lnode = mk_rexp(subn->lnode);
		if (nexp == 2)
			append_right(subn, node(node(make_number(0.0),
						     Node_field_spec,
						     (NODE *) NULL),
					        Node_expression_list,
						(NODE *) NULL));
		else if (subn->rnode->rnode->lnode->type == Node_val) {
			if (do_lint)
				lintwarn(_("%s: string literal as last arg of substitute has no effect"),
					(r->builtin == do_sub) ? "sub" : "gsub");
		} else if (! isassignable(subn->rnode->rnode->lnode)) {
			yyerror(_("%s third parameter is not a changeable object"),
				(r->builtin == do_sub) ? "sub" : "gsub");
		}
	} else if (r->builtin == do_gensub) {
		if (subn->lnode->type != Node_regex)
			subn->lnode = mk_rexp(subn->lnode);
		if (nexp == 3)
			append_right(subn, node(node(make_number(0.0),
						     Node_field_spec,
						     (NODE *) NULL),
					        Node_expression_list,
						(NODE *) NULL));
	} else if (r->builtin == do_split) {
		if (nexp == 2)
			append_right(subn,
			    node(FS_node, Node_expression_list, (NODE *) NULL));
		n = subn->rnode->rnode->lnode;
		if (n->type != Node_regex)
			subn->rnode->rnode->lnode = mk_rexp(n);
		if (nexp == 2)
			subn->rnode->rnode->lnode->re_flags |= FS_DFLT;
	} else if (r->builtin == do_close) {
		static short warned = FALSE;

		if ( nexp == 2) {
			if (do_lint && nexp == 2 && ! warned) {
				warned = TRUE;
				lintwarn(_("close: second argument is a gawk extension"));
			}
			if (do_traditional)
				fatal(_("close: second argument is a gawk extension"));
		}
	} else if (do_intl					/* --gen-po */
			&& r->builtin == do_dcgettext		/* dcgettext(...) */
			&& subn->lnode->type == Node_val	/* 1st arg is constant */
			&& (subn->lnode->flags & STRCUR) != 0) {	/* it's a string constant */
		/* ala xgettext, dcgettext("some string" ...) dumps the string */
		NODE *str = subn->lnode;

		if ((str->flags & INTLSTR) != 0)
			warning(_("use of dcgettext(_\"...\") is incorrect: remove leading underscore"));
			/* don't dump it, the lexer already did */
		else
			dumpintlstr(str->stptr, str->stlen);
	} else if (do_intl					/* --gen-po */
			&& r->builtin == do_dcngettext		/* dcngettext(...) */
			&& subn->lnode->type == Node_val	/* 1st arg is constant */
			&& (subn->lnode->flags & STRCUR) != 0	/* it's a string constant */
			&& subn->rnode->lnode->type == Node_val	/* 2nd arg is constant too */
			&& (subn->rnode->lnode->flags & STRCUR) != 0) {	/* it's a string constant */
		/* ala xgettext, dcngettext("some string", "some plural" ...) dumps the string */
		NODE *str1 = subn->lnode;
		NODE *str2 = subn->rnode->lnode;

		if (((str1->flags | str2->flags) & INTLSTR) != 0)
			warning(_("use of dcngettext(_\"...\") is incorrect: remove leading underscore"));
		else
			dumpintlstr2(str1->stptr, str1->stlen, str2->stptr, str2->stlen);
	}

	r->subnode = subn;
	if (r->builtin == do_sprintf) {
		count_args(r);
		r->lnode->printf_count = r->printf_count; /* hack */
	}
	return r;
}

/* make_for_loop --- build a for loop */

static NODE *
make_for_loop(NODE *init, NODE *cond, NODE *incr)
{
	register FOR_LOOP_HEADER *r;
	NODE *n;

	emalloc(r, FOR_LOOP_HEADER *, sizeof(FOR_LOOP_HEADER), "make_for_loop");
	getnode(n);
	n->type = Node_illegal;
	r->init = init;
	r->cond = cond;
	r->incr = incr;
	n->sub.nodep.r.hd = r;
	return n;
}

/* dup_parms --- return TRUE if there are duplicate parameters */

static int
dup_parms(NODE *func)
{
	register NODE *np;
	const char *fname, **names;
	int count, i, j, dups;
	NODE *params;

	if (func == NULL)	/* error earlier */
		return TRUE;

	fname = func->param;
	count = func->param_cnt;
	params = func->rnode;

	if (count == 0)		/* no args, no problem */
		return FALSE;

	if (params == NULL)	/* error earlier */
		return TRUE;

	emalloc(names, const char **, count * sizeof(char *), "dup_parms");

	i = 0;
	for (np = params; np != NULL; np = np->rnode) {
		if (np->param == NULL) { /* error earlier, give up, go home */
			free(names);
			return TRUE;
		}
		names[i++] = np->param;
	}

	dups = 0;
	for (i = 1; i < count; i++) {
		for (j = 0; j < i; j++) {
			if (strcmp(names[i], names[j]) == 0) {
				dups++;
				error(
	_("function `%s': parameter #%d, `%s', duplicates parameter #%d"),
					fname, i+1, names[j], j+1);
			}
		}
	}

	free(names);
	return (dups > 0 ? TRUE : FALSE);
}

/* parms_shadow --- check if parameters shadow globals */

static int
parms_shadow(const char *fname, NODE *func)
{
	int count, i;
	int ret = FALSE;

	if (fname == NULL || func == NULL)	/* error earlier */
		return FALSE;

	count = func->lnode->param_cnt;

	if (count == 0)		/* no args, no problem */
		return FALSE;

	/*
	 * Use warning() and not lintwarn() so that can warn
	 * about all shadowed parameters.
	 */
	for (i = 0; i < count; i++) {
		if (lookup(func->parmlist[i]) != NULL) {
			warning(
	_("function `%s': parameter `%s' shadows global variable"),
					fname, func->parmlist[i]);
			ret = TRUE;
		}
	}

	return ret;
}

/*
 * install:
 * Install a name in the symbol table, even if it is already there.
 * Caller must check against redefinition if that is desired. 
 */

NODE *
install(char *name, NODE *value)
{
	register NODE *hp;
	register size_t len;
	register int bucket;

	var_count++;
	len = strlen(name);
	bucket = hash(name, len, (unsigned long) HASHSIZE);
	getnode(hp);
	hp->type = Node_hashnode;
	hp->hnext = variables[bucket];
	variables[bucket] = hp;
	hp->hlength = len;
	hp->hvalue = value;
	hp->hname = name;
	hp->hvalue->vname = name;
	return hp->hvalue;
}

/* lookup --- find the most recent hash node for name installed by install */

NODE *
lookup(const char *name)
{
	register NODE *bucket;
	register size_t len;

	len = strlen(name);
	for (bucket = variables[hash(name, len, (unsigned long) HASHSIZE)];
			bucket != NULL; bucket = bucket->hnext)
		if (bucket->hlength == len && STREQN(bucket->hname, name, len))
			return bucket->hvalue;

	return NULL;
}

/* var_comp --- compare two variable names */

static int
var_comp(const void *v1, const void *v2)
{
	const NODE *const *npp1, *const *npp2;
	const NODE *n1, *n2;
	int minlen;

	npp1 = (const NODE *const *) v1;
	npp2 = (const NODE *const *) v2;
	n1 = *npp1;
	n2 = *npp2;

	if (n1->hlength > n2->hlength)
		minlen = n1->hlength;
	else
		minlen = n2->hlength;

	return strncmp(n1->hname, n2->hname, minlen);
}

/* valinfo --- dump var info */

static void
valinfo(NODE *n, FILE *fp)
{
	if (n->flags & STRING) {
		fprintf(fp, "string (");
		pp_string_fp(fp, n->stptr, n->stlen, '"', FALSE);
		fprintf(fp, ")\n");
	} else if (n->flags & NUMBER)
		fprintf(fp, "number (%.17g)\n", n->numbr);
	else if (n->flags & STRCUR) {
		fprintf(fp, "string value (");
		pp_string_fp(fp, n->stptr, n->stlen, '"', FALSE);
		fprintf(fp, ")\n");
	} else if (n->flags & NUMCUR)
		fprintf(fp, "number value (%.17g)\n", n->numbr);
	else
		fprintf(fp, "?? flags %s\n", flags2str(n->flags));
}


/* dump_vars --- dump the symbol table */

void
dump_vars(const char *fname)
{
	int i, j;
	NODE **table;
	NODE *p;
	FILE *fp;

	emalloc(table, NODE **, var_count * sizeof(NODE *), "dump_vars");

	if (fname == NULL)
		fp = stderr;
	else if ((fp = fopen(fname, "w")) == NULL) {
		warning(_("could not open `%s' for writing (%s)"), fname, strerror(errno));
		warning(_("sending profile to standard error"));
		fp = stderr;
	}

	for (i = j = 0; i < HASHSIZE; i++)
		for (p = variables[i]; p != NULL; p = p->hnext)
			table[j++] = p;

	assert(j == var_count);

	/* Shazzam! */
	qsort(table, j, sizeof(NODE *), var_comp);

	for (i = 0; i < j; i++) {
		p = table[i];
		if (p->hvalue->type == Node_func)
			continue;
		fprintf(fp, "%.*s: ", (int) p->hlength, p->hname);
		if (p->hvalue->type == Node_var_array)
			fprintf(fp, "array, %ld elements\n", p->hvalue->table_size);
		else if (p->hvalue->type == Node_var_new)
			fprintf(fp, "unused variable\n");
		else if (p->hvalue->type == Node_var)
			valinfo(p->hvalue->var_value, fp);
		else {
			NODE **lhs = get_lhs(p->hvalue, NULL, FALSE);

			valinfo(*lhs, fp);
		}
	}

	if (fp != stderr && fclose(fp) != 0)
		warning(_("%s: close failed (%s)"), fname, strerror(errno));

	free(table);
}

/* release_all_vars --- free all variable memory */

void
release_all_vars()
{
	int i;
	NODE *p, *next;

	for (i = 0; i < HASHSIZE; i++)
		for (p = variables[i]; p != NULL; p = next) {
			next = p->hnext;

			if (p->hvalue->type == Node_func)
				continue;
			else if (p->hvalue->type == Node_var_array)
				assoc_clear(p->hvalue);
			else if (p->hvalue->type != Node_var_new) {
				NODE **lhs = get_lhs(p->hvalue, NULL, FALSE);

				unref(*lhs);
			}
			unref(p);
	}
}

/* finfo --- for use in comparison and sorting of function names */

struct finfo {
	const char *name;
	size_t nlen;
	NODE *func;
};

/* fcompare --- comparison function for qsort */

static int
fcompare(const void *p1, const void *p2)
{
	const struct finfo *f1, *f2;
	int minlen;

	f1 = (const struct finfo *) p1;
	f2 = (const struct finfo *) p2;

	if (f1->nlen > f2->nlen)
		minlen = f2->nlen;
	else
		minlen = f1->nlen;

	return strncmp(f1->name, f2->name, minlen);
}

/* dump_funcs --- print all functions */

void
dump_funcs()
{
	int i, j;
	NODE *p;
	static struct finfo *tab = NULL;

	if (func_count == 0)
		return;

	/*
	 * Walk through symbol table countng functions.
	 * Could be more than func_count if there are
	 * extension functions.
	 */
	for (i = j = 0; i < HASHSIZE; i++) {
		for (p = variables[i]; p != NULL; p = p->hnext) {
			if (p->hvalue->type == Node_func) {
				j++;
			}
		}
	}

	if (tab == NULL)
		emalloc(tab, struct finfo *, j * sizeof(struct finfo), "dump_funcs");

	/* now walk again, copying info */
	for (i = j = 0; i < HASHSIZE; i++) {
		for (p = variables[i]; p != NULL; p = p->hnext) {
			if (p->hvalue->type == Node_func) {
				tab[j].name = p->hname;
				tab[j].nlen = p->hlength;
				tab[j].func = p->hvalue;
				j++;
			}
		}
	}


	/* Shazzam! */
	qsort(tab, j, sizeof(struct finfo), fcompare);

	for (i = 0; i < j; i++)
		pp_func(tab[i].name, tab[i].nlen, tab[i].func);

	free(tab);
}

/* shadow_funcs --- check all functions for parameters that shadow globals */

void
shadow_funcs()
{
	int i, j;
	NODE *p;
	struct finfo *tab;
	static int calls = 0;
	int shadow = FALSE;

	if (func_count == 0)
		return;

	if (calls++ != 0)
		fatal(_("shadow_funcs() called twice!"));

	emalloc(tab, struct finfo *, func_count * sizeof(struct finfo), "shadow_funcs");

	for (i = j = 0; i < HASHSIZE; i++) {
		for (p = variables[i]; p != NULL; p = p->hnext) {
			if (p->hvalue->type == Node_func) {
				tab[j].name = p->hname;
				tab[j].nlen = p->hlength;
				tab[j].func = p->hvalue;
				j++;
			}
		}
	}

	assert(j == func_count);

	/* Shazzam! */
	qsort(tab, func_count, sizeof(struct finfo), fcompare);

	for (i = 0; i < j; i++)
		shadow |= parms_shadow(tab[i].name, tab[i].func);

	free(tab);

	/* End with fatal if the user requested it.  */
	if (shadow && lintfunc != warning)
		lintwarn(_("there were shadowed variables."));
}

/*
 * append_right:
 * Add new to the rightmost branch of LIST.  This uses n^2 time, so we make
 * a simple attempt at optimizing it.
 */

static NODE *
append_right(NODE *list, NODE *new)
{
	register NODE *oldlist;
	static NODE *savefront = NULL, *savetail = NULL;

	if (list == NULL || new == NULL)
		return list;

	oldlist = list;
	if (savefront == oldlist)
		list = savetail; /* Be careful: maybe list->rnode != NULL */
	else
		savefront = oldlist;

	while (list->rnode != NULL)
		list = list->rnode;
	savetail = list->rnode = new;
	return oldlist;
}

/*
 * append_pattern:
 * A wrapper around append_right, used for rule lists.
 */
static inline NODE *
append_pattern(NODE **list, NODE *patt)
{
	NODE *n = node(patt, Node_rule_node, (NODE *) NULL);

	if (*list == NULL)
		*list = n;
	else {
		NODE *n1 = node(n, Node_rule_list, (NODE *) NULL);
		if ((*list)->type != Node_rule_list)
			*list = node(*list, Node_rule_list, n1);
		else
			(void) append_right(*list, n1);
	}
	return n;
}

/*
 * func_install:
 * check if name is already installed;  if so, it had better have Null value,
 * in which case def is added as the value. Otherwise, install name with def
 * as value. 
 *
 * Extra work, build up and save a list of the parameter names in a table
 * and hang it off params->parmlist. This is used to set the `vname' field
 * of each function parameter during a function call. See eval.c.
 */

static void
func_install(NODE *params, NODE *def)
{
	NODE *r, *n, *thisfunc;
	char **pnames, *names, *sp;
	size_t pcount = 0, space = 0;
	int i;

	/* check for function foo(foo) { ... }.  bleah. */
	for (n = params->rnode; n != NULL; n = n->rnode) {
		if (strcmp(n->param, params->param) == 0)
			fatal(_("function `%s': can't use function name as parameter name"),
					params->param); 
	}

	thisfunc = NULL;	/* turn off warnings */

	/* symbol table managment */
	pop_var(params, FALSE);
	r = lookup(params->param);
	if (r != NULL) {
		fatal(_("function name `%s' previously defined"), params->param);
	} else if (params->param == builtin_func)	/* not a valid function name */
		goto remove_params;

	/* install the function */
	thisfunc = node(params, Node_func, def);
	(void) install(params->param, thisfunc);

	/* figure out amount of space to allocate for variable names */
	for (n = params->rnode; n != NULL; n = n->rnode) {
		pcount++;
		space += strlen(n->param) + 1;
	}

	/* allocate it and fill it in */
	if (pcount != 0) {
		emalloc(names, char *, space, "func_install");
		emalloc(pnames, char **, pcount * sizeof(char *), "func_install");
		sp = names;
		for (i = 0, n = params->rnode; i < pcount; i++, n = n->rnode) {
			pnames[i] = sp;
			strcpy(sp, n->param);
			sp += strlen(n->param) + 1;
		}
		thisfunc->parmlist = pnames;
	} else {
		thisfunc->parmlist = NULL;
	}

	/* update lint table info */
	func_use(params->param, FUNC_DEFINE);

	func_count++;	/* used by profiling / pretty printer */

remove_params:
	/* remove params from symbol table */
	pop_params(params->rnode);
}

/* pop_var --- remove a variable from the symbol table */

static void
pop_var(NODE *np, int freeit)
{
	register NODE *bucket, **save;
	register size_t len;
	char *name;

	name = np->param;
	len = strlen(name);
	save = &(variables[hash(name, len, (unsigned long) HASHSIZE)]);
	for (bucket = *save; bucket != NULL; bucket = bucket->hnext) {
		if (len == bucket->hlength && STREQN(bucket->hname, name, len)) {
			var_count--;
			*save = bucket->hnext;
			freenode(bucket);
			if (freeit)
				free(np->param);
			return;
		}
		save = &(bucket->hnext);
	}
}

/* pop_params --- remove list of function parameters from symbol table */

/*
 * pop parameters out of the symbol table. do this in reverse order to
 * avoid reading freed memory if there were duplicated parameters.
 */
static void
pop_params(NODE *params)
{
	if (params == NULL)
		return;
	pop_params(params->rnode);
	pop_var(params, TRUE);
}

/* make_param --- make NAME into a function parameter */

static NODE *
make_param(char *name)
{
	NODE *r;

	getnode(r);
	r->type = Node_param_list;
	r->rnode = NULL;
	r->param = name;
	r->param_cnt = param_counter++;
	return (install(name, r));
}

static struct fdesc {
	char *name;
	short used;
	short defined;
	struct fdesc *next;
} *ftable[HASHSIZE];

/* func_use --- track uses and definitions of functions */

static void
func_use(const char *name, enum defref how)
{
	struct fdesc *fp;
	int len;
	int ind;

	len = strlen(name);
	ind = hash(name, len, HASHSIZE);

	for (fp = ftable[ind]; fp != NULL; fp = fp->next) {
		if (strcmp(fp->name, name) == 0) {
			if (how == FUNC_DEFINE)
				fp->defined++;
			else
				fp->used++;
			return;
		}
	}

	/* not in the table, fall through to allocate a new one */

	emalloc(fp, struct fdesc *, sizeof(struct fdesc), "func_use");
	memset(fp, '\0', sizeof(struct fdesc));
	emalloc(fp->name, char *, len + 1, "func_use");
	strcpy(fp->name, name);
	if (how == FUNC_DEFINE)
		fp->defined++;
	else
		fp->used++;
	fp->next = ftable[ind];
	ftable[ind] = fp;
}

/* check_funcs --- verify functions that are called but not defined */

static void
check_funcs()
{
	struct fdesc *fp, *next;
	int i;

	for (i = 0; i < HASHSIZE; i++) {
		for (fp = ftable[i]; fp != NULL; fp = fp->next) {
#ifdef REALLYMEAN
			/* making this the default breaks old code. sigh. */
			if (fp->defined == 0) {
				error(
		_("function `%s' called but never defined"), fp->name);
				errcount++;
			}
#else
			if (do_lint && fp->defined == 0)
				lintwarn(
		_("function `%s' called but never defined"), fp->name);
#endif
			if (do_lint && fp->used == 0) {
				lintwarn(_("function `%s' defined but never called"),
					fp->name);
			}
		}
	}

	/* now let's free all the memory */
	for (i = 0; i < HASHSIZE; i++) {
		for (fp = ftable[i]; fp != NULL; fp = next) {
			next = fp->next;
			free(fp->name);
			free(fp);
		}
	}
}

/* param_sanity --- look for parameters that are regexp constants */

static void
param_sanity(NODE *arglist)
{
	NODE *argp, *arg;
	int i;

	for (i = 1, argp = arglist; argp != NULL; argp = argp->rnode, i++) {
		arg = argp->lnode;
		if (arg->type == Node_regex)
			warning(_("regexp constant for parameter #%d yields boolean value"), i);
	}
}

/* variable --- make sure NAME is in the symbol table */

NODE *
variable(char *name, int can_free, NODETYPE type)
{
	register NODE *r;

	if ((r = lookup(name)) != NULL) {
		if (r->type == Node_func)
			fatal(_("function `%s' called with space between name and `(',\n%s"),
				r->vname,
				_("or used as a variable or an array"));
	} else {
		/* not found */
		if (! do_traditional && STREQ(name, "PROCINFO"))
			r = load_procinfo();
		else if (STREQ(name, "ENVIRON"))
			r = load_environ();
		else {
			/*
			 * This is the only case in which we may not free the string.
			 */
			NODE *n;

			if (type == Node_var)
				n = node(Nnull_string, type, (NODE *) NULL);
			else
				n = node((NODE *) NULL, type, (NODE *) NULL);

			return install(name, n);
		}
	}
	if (can_free)
		free(name);
	return r;
}

/* mk_rexp --- make a regular expression constant */

static NODE *
mk_rexp(NODE *exp)
{
	NODE *n;

	if (exp->type == Node_regex)
		return exp;

	getnode(n);
	n->type = Node_dynregex;
	n->re_exp = exp;
	n->re_text = NULL;
	n->re_reg = NULL;
	n->re_flags = 0;
	return n;
}

/* isnoeffect --- when used as a statement, has no side effects */

/*
 * To be completely general, we should recursively walk the parse
 * tree, to make sure that all the subexpressions also have no effect.
 * Instead, we just weaken the actual warning that's printed, up above
 * in the grammar.
 */

static int
isnoeffect(NODETYPE type)
{
	switch (type) {
	case Node_times:
	case Node_quotient:
	case Node_mod:
	case Node_plus:
	case Node_minus:
	case Node_subscript:
	case Node_concat:
	case Node_exp:
	case Node_unary_minus:
	case Node_field_spec:
	case Node_and:
	case Node_or:
	case Node_equal:
	case Node_notequal:
	case Node_less:
	case Node_greater:
	case Node_leq:
	case Node_geq:
	case Node_match:
	case Node_nomatch:
	case Node_not:
	case Node_val:
	case Node_in_array:
	case Node_NF:
	case Node_NR:
	case Node_FNR:
	case Node_FS:
	case Node_RS:
	case Node_FIELDWIDTHS:
	case Node_IGNORECASE:
	case Node_OFS:
	case Node_ORS:
	case Node_OFMT:
	case Node_CONVFMT:
	case Node_BINMODE:
	case Node_LINT:
	case Node_TEXTDOMAIN:
		return TRUE;
	default:
		break;	/* keeps gcc -Wall happy */
	}

	return FALSE;
}

/* isassignable --- can this node be assigned to? */

static int
isassignable(register NODE *n)
{
	switch (n->type) {
	case Node_var_new:
	case Node_var:
	case Node_FIELDWIDTHS:
	case Node_RS:
	case Node_FS:
	case Node_FNR:
	case Node_NR:
	case Node_NF:
	case Node_IGNORECASE:
	case Node_OFMT:
	case Node_CONVFMT:
	case Node_ORS:
	case Node_OFS:
	case Node_LINT:
	case Node_BINMODE:
	case Node_TEXTDOMAIN:
	case Node_field_spec:
	case Node_subscript:
		return TRUE;
	case Node_param_list:
		return ((n->flags & FUNC) == 0);  /* ok if not func name */
	default:
		break;	/* keeps gcc -Wall happy */
	}
	return FALSE;
}

/* stopme --- for debugging */

NODE *
stopme(NODE *tree ATTRIBUTE_UNUSED)
{
	return 0;
}

/* dumpintlstr --- write out an initial .po file entry for the string */

static void
dumpintlstr(const char *str, size_t len)
{
	char *cp;

	/* See the GNU gettext distribution for details on the file format */

	if (source != NULL) {
		/* ala the gettext sources, remove leading `./'s */
		for (cp = source; cp[0] == '.' && cp[1] == '/'; cp += 2)
			continue;
		printf("#: %s:%d\n", cp, sourceline);
	}

	printf("msgid ");
	pp_string_fp(stdout, str, len, '"', TRUE);
	putchar('\n');
	printf("msgstr \"\"\n\n");
	fflush(stdout);
}

/* dumpintlstr2 --- write out an initial .po file entry for the string and its plural */

static void
dumpintlstr2(const char *str1, size_t len1, const char *str2, size_t len2)
{
	char *cp;

	/* See the GNU gettext distribution for details on the file format */

	if (source != NULL) {
		/* ala the gettext sources, remove leading `./'s */
		for (cp = source; cp[0] == '.' && cp[1] == '/'; cp += 2)
			continue;
		printf("#: %s:%d\n", cp, sourceline);
	}

	printf("msgid ");
	pp_string_fp(stdout, str1, len1, '"', TRUE);
	putchar('\n');
	printf("msgid_plural ");
	pp_string_fp(stdout, str2, len2, '"', TRUE);
	putchar('\n');
	printf("msgstr[0] \"\"\nmsgstr[1] \"\"\n\n");
	fflush(stdout);
}

/* count_args --- count the number of printf arguments */

static void
count_args(NODE *tree)
{
	size_t count = 0;
	NODE *save_tree;

	assert(tree->type == Node_K_printf
		|| (tree->type == Node_builtin && tree->builtin == do_sprintf));
	save_tree = tree;

	tree = tree->lnode;	/* printf format string */

	for (count = 0; tree != NULL; tree = tree->rnode)
		count++;

	save_tree->printf_count = count;
}

/* isarray --- can this type be subscripted? */

static int
isarray(NODE *n)
{
	switch (n->type) {
	case Node_var_new:
	case Node_var_array:
		return TRUE;
	case Node_param_list:
		return ((n->flags & FUNC) == 0);
	case Node_array_ref:
		cant_happen();
		break;
	default:
		break;	/* keeps gcc -Wall happy */
	}

	return FALSE;
}


