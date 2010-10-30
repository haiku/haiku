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




/* Copy the first part of user declarations.  */
#line 23 "printrcy.y"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprint/gutenprint-intl-internal.h>
#include <gutenprintui2/gutenprintui.h>
#include "gutenprintui-internal.h"
#include "printrc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern int mylineno;

extern int yylex(void);
char *quotestrip(const char *i);
char *endstrip(const char *i);

extern char* yytext;

static int yyerror( const char *s )
{
  fprintf(stderr,"stdin:%d: %s before '%s'\n", mylineno, s, yytext);
  return 0;
}

static stpui_plist_t *current_printer = NULL;



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
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 199 "printrcy.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

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
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   75

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  42
/* YYNRULES -- Number of rules. */
#define YYNRULES  70
/* YYNRULES -- Number of states. */
#define YYNSTATES  114

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

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
      35,    36,    37,    38,    39,    40
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     7,    10,    13,    16,    19,    22,    25,
      28,    31,    34,    37,    40,    43,    46,    49,    52,    53,
      58,    63,    68,    73,    78,    83,    88,    90,    92,    94,
      96,    98,   100,   102,   105,   108,   110,   112,   114,   116,
     118,   120,   122,   124,   126,   128,   130,   132,   134,   136,
     138,   140,   143,   145,   149,   152,   154,   157,   160,   162,
     164,   167,   170,   172,   174,   177,   179,   183,   185,   187,
     189
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      82,     0,    -1,    12,     7,     7,    -1,    13,     7,    -1,
      26,     7,    -1,    27,     7,    -1,    28,     7,    -1,    29,
       7,    -1,    30,     3,    -1,    14,     4,    -1,    15,     3,
      -1,    16,     3,    -1,    17,     3,    -1,    19,     3,    -1,
      20,     3,    -1,    23,     3,    -1,    21,     3,    -1,    22,
       3,    -1,    -1,     8,    34,     6,     3,    -1,     8,    35,
       6,     7,    -1,     8,    36,     6,     7,    -1,     8,    37,
       6,     4,    -1,     8,    38,     6,     3,    -1,     8,    39,
       6,     6,    -1,     8,    40,     6,     7,    -1,    59,    -1,
      60,    -1,    61,    -1,    62,    -1,    64,    -1,    65,    -1,
      63,    -1,    25,    66,    -1,    68,    67,    -1,    58,    -1,
      43,    -1,    49,    -1,    50,    -1,    51,    -1,    52,    -1,
      53,    -1,    54,    -1,    56,    -1,    57,    -1,    55,    -1,
      44,    -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,
      70,    69,    -1,    58,    -1,    42,    70,    68,    -1,    72,
      71,    -1,    58,    -1,    10,     7,    -1,    11,     6,    -1,
      73,    -1,    74,    -1,    73,    74,    -1,     8,     7,    -1,
      75,    -1,    77,    -1,    79,    78,    -1,    58,    -1,    31,
      79,    33,    -1,    80,    -1,    76,    -1,    58,    -1,    24,
      81,    72,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   100,   100,   111,   118,   128,   138,   148,   158,   165,
     172,   179,   186,   193,   200,   207,   228,   235,   242,   245,
     262,   280,   298,   315,   332,   353,   376,   376,   376,   376,
     377,   377,   377,   380,   383,   383,   386,   386,   386,   386,
     387,   387,   387,   387,   387,   388,   388,   388,   388,   389,
     389,   392,   392,   395,   398,   398,   401,   405,   415,   415,
     418,   421,   432,   432,   435,   435,   438,   441,   441,   441,
     444
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tINT", "tDOUBLE", "tDIMENSION", 
  "tBOOLEAN", "tSTRING", "tWORD", "tGSWORD", "CURRENT_PRINTER", 
  "SHOW_ALL_PAPER_SIZES", "PRINTER", "DESTINATION", "SCALING", 
  "ORIENTATION", "AUTOSIZE_ROLL_PAPER", "UNIT", "DRIVER", "LEFT", "TOP", 
  "CUSTOM_PAGE_WIDTH", "CUSTOM_PAGE_HEIGHT", "OUTPUT_TYPE", "PRINTRC_HDR", 
  "PARAMETER", "QUEUE_NAME", "OUTPUT_FILENAME", "EXTRA_PRINTER_OPTIONS", 
  "CUSTOM_COMMAND", "COMMAND_TYPE", "GLOBAL_SETTINGS", "GLOBAL", 
  "END_GLOBAL_SETTINGS", "pINT", "pSTRING_LIST", "pFILE", "pDOUBLE", 
  "pDIMENSION", "pBOOLEAN", "pCURVE", "$accept", "Printer", "Destination", 
  "Queue_Name", "Output_Filename", "Extra_Printer_Options", 
  "Custom_Command", "Command_Type", "Scaling", "Orientation", 
  "Autosize_Roll_Paper", "Unit", "Left", "Top", "Output_Type", 
  "Custom_Page_Width", "Custom_Page_Height", "Empty", "Int_Param", 
  "String_List_Param", "File_Param", "Double_Param", "Dimension_Param", 
  "Boolean_Param", "Curve_Param", "Typed_Param", "Parameter", 
  "Parameters", "Standard_Value", "Standard_Values", "A_Printer", 
  "Printers", "Current_Printer", "Show_All_Paper_Sizes", "Global", 
  "Old_Globals", "New_Global_Setting", "Global_Setting", 
  "Global_Settings", "Global_Subblock", "Global_Block", "Thing", 0
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
     295
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    66,    66,    66,
      66,    66,    66,    67,    68,    68,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    70,    70,    71,    72,    72,    73,    74,    75,    75,
      76,    77,    78,    78,    79,    79,    80,    81,    81,    81,
      82
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     0,     4,
       4,     4,     4,     4,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     3,     2,     1,     2,     2,     1,     1,
       2,     2,     1,     1,     2,     1,     3,     1,     1,     1,
       3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    18,     0,     0,    18,    69,     0,    68,    67,    18,
       1,    56,    65,     0,     0,    60,    55,    70,     0,    66,
      58,    59,    62,    63,    64,    57,     0,    18,    54,    61,
       0,    52,    18,     2,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    36,
      46,    47,    48,    49,    50,    37,    38,    39,    40,    41,
      42,    45,    43,    44,    35,    53,    51,     3,     9,    10,
      11,    12,    13,    14,    16,    17,    15,     4,     5,     6,
       7,     8,     0,    34,     0,    26,    27,    28,    29,    32,
      30,    31,    33,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    19,    20,    21,
      22,    23,    24,    25
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    27,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,     5,    85,    86,
      87,    88,    89,    90,    91,    92,    83,    65,    66,    32,
      28,    17,     6,    15,    22,     7,    23,    24,    13,     8,
       9,     2
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -26
static const yysigned_char yypact[] =
{
     -22,    -2,     6,     0,   -26,   -26,     5,   -26,   -26,   -26,
     -26,   -26,   -26,    -7,    11,   -26,   -26,     7,    13,   -26,
     -26,   -26,   -26,   -26,   -26,   -26,    14,   -26,   -26,   -26,
      15,   -26,    17,   -26,    18,    20,    24,    32,    38,    39,
      45,    46,    47,    48,    49,    50,    51,    52,    57,   -26,
     -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,
     -26,   -26,   -26,   -26,   -26,    27,   -26,   -26,   -26,   -26,
     -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,
     -26,   -26,    10,   -26,   -25,   -26,   -26,   -26,   -26,   -26,
     -26,   -26,   -26,    55,    56,    58,    59,    60,    61,    62,
      66,    63,    64,    68,    70,    69,    67,   -26,   -26,   -26,
     -26,   -26,   -26,   -26
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,
     -26,   -26,   -26,   -26,   -26,   -26,   -26,    -4,   -26,   -26,
     -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,   -26,
     -26,   -26,    40,    41,   -26,   -26,   -26,   -26,   -26,   -26,
     -26,   -26
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      12,    18,     1,     3,    14,    16,    10,    11,     3,    93,
      94,    95,    96,    97,    98,    99,    14,    25,    84,    26,
      29,    30,    33,    31,    68,    67,    19,    69,    64,     4,
      34,    35,    36,    37,    38,    70,    39,    40,    41,    42,
      43,    71,    72,    44,    45,    46,    47,    48,    73,    74,
      75,    76,    82,    20,    21,     0,    77,    78,    79,    80,
      81,   100,   101,     0,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   113,   112
};

static const yysigned_char yycheck[] =
{
       4,     8,    24,    10,    11,     9,     0,     7,    10,    34,
      35,    36,    37,    38,    39,    40,    11,     6,     8,    12,
       7,     7,     7,    27,     4,     7,    33,     3,    32,    31,
      13,    14,    15,    16,    17,     3,    19,    20,    21,    22,
      23,     3,     3,    26,    27,    28,    29,    30,     3,     3,
       3,     3,    25,    13,    13,    -1,     7,     7,     7,     7,
       3,     6,     6,    -1,     6,     6,     6,     6,     6,     3,
       7,     7,     4,     3,     7,     6
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    24,    82,    10,    31,    58,    73,    76,    80,    81,
       0,     7,    58,    79,    11,    74,    58,    72,     8,    33,
      73,    74,    75,    77,    78,     6,    12,    42,    71,     7,
       7,    58,    70,     7,    13,    14,    15,    16,    17,    19,
      20,    21,    22,    23,    26,    27,    28,    29,    30,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    68,    69,     7,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     7,     7,     7,
       7,     3,    25,    67,     8,    59,    60,    61,    62,    63,
      64,    65,    66,    34,    35,    36,    37,    38,    39,    40,
       6,     6,     6,     6,     6,     6,     6,     3,     7,     7,
       4,     3,     6,     7
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
#line 101 "printrcy.y"
    {
	  current_printer = stpui_plist_create(yyvsp[-1].sval, yyvsp[0].sval);
	  g_free(yyvsp[-1].sval);
	  g_free(yyvsp[0].sval);
	}
    break;

  case 3:
#line 112 "printrcy.y"
    {
	  if (yyvsp[0].sval)
	    g_free(yyvsp[0].sval);
	}
    break;

  case 4:
#line 119 "printrcy.y"
    {
	  if (current_printer && yyvsp[0].sval)
	    {
	      stpui_plist_set_queue_name(current_printer, yyvsp[0].sval);
	      g_free(yyvsp[0].sval);
	    }
	}
    break;

  case 5:
#line 129 "printrcy.y"
    {
	  if (current_printer && yyvsp[0].sval)
	    {
	      stpui_plist_set_output_filename(current_printer, yyvsp[0].sval);
	      g_free(yyvsp[0].sval);
	    }
	}
    break;

  case 6:
#line 139 "printrcy.y"
    {
	  if (current_printer && yyvsp[0].sval)
	    {
	      stpui_plist_set_extra_printer_options(current_printer, yyvsp[0].sval);
	      g_free(yyvsp[0].sval);
	    }
	}
    break;

  case 7:
#line 149 "printrcy.y"
    {
	  if (current_printer && yyvsp[0].sval)
	    {
	      stpui_plist_set_custom_command(current_printer, yyvsp[0].sval);
	      g_free(yyvsp[0].sval);
	    }
	}
    break;

  case 8:
#line 159 "printrcy.y"
    {
	  if (current_printer)
	    stpui_plist_set_command_type(current_printer, yyvsp[0].ival);
	}
    break;

  case 9:
#line 166 "printrcy.y"
    {
	  if (current_printer)
	    current_printer->scaling = yyvsp[0].dval; 
	}
    break;

  case 10:
#line 173 "printrcy.y"
    {
	  if (current_printer)
	    current_printer->orientation = yyvsp[0].ival;
	}
    break;

  case 11:
#line 180 "printrcy.y"
    {
	  if (current_printer)
	    current_printer->auto_size_roll_feed_paper = yyvsp[0].ival;
	}
    break;

  case 12:
#line 187 "printrcy.y"
    {
	  if (current_printer)
	    current_printer->unit = yyvsp[0].ival; 
	}
    break;

  case 13:
#line 194 "printrcy.y"
    {
	  if (current_printer)
	    stp_set_left(current_printer->v, yyvsp[0].ival);
	}
    break;

  case 14:
#line 201 "printrcy.y"
    {
	  if (current_printer)
	    stp_set_top(current_printer->v, yyvsp[0].ival);
	}
    break;

  case 15:
#line 208 "printrcy.y"
    {
	  if (current_printer)
	    {
	      switch (yyvsp[0].ival)
		{
		case 0:
		  stp_set_string_parameter
		    (current_printer->v, "PrintingMode", "BW");
		  break;
		case 1:
		case 2:
		default:
		  stp_set_string_parameter
		    (current_printer->v, "PrintingMode", "Color");
		  break;
		}
	    }
	}
    break;

  case 16:
#line 229 "printrcy.y"
    {
	  if (current_printer)
	    stp_set_page_width(current_printer->v, yyvsp[0].ival);
	}
    break;

  case 17:
#line 236 "printrcy.y"
    {
	  if (current_printer)
	    stp_set_page_height(current_printer->v, yyvsp[0].ival);
	}
    break;

  case 19:
#line 246 "printrcy.y"
    {
	  if (current_printer)
	    {
	      stp_set_int_parameter(current_printer->v, yyvsp[-3].sval, yyvsp[0].ival);
	      if (strcmp(yyvsp[-1].sval, "False") == 0)
		stp_set_int_parameter_active(current_printer->v, yyvsp[-3].sval,
					     STP_PARAMETER_INACTIVE);
	      else
		stp_set_int_parameter_active(current_printer->v, yyvsp[-3].sval,
					     STP_PARAMETER_ACTIVE);
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	}
    break;

  case 20:
#line 263 "printrcy.y"
    {
	  if (current_printer)
	    {
	      stp_set_string_parameter(current_printer->v, yyvsp[-3].sval, yyvsp[0].sval);
	      if (strcmp(yyvsp[-1].sval, "False") == 0)
		stp_set_string_parameter_active(current_printer->v, yyvsp[-3].sval,
						STP_PARAMETER_INACTIVE);
	      else
		stp_set_string_parameter_active(current_printer->v, yyvsp[-3].sval,
						STP_PARAMETER_ACTIVE);
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	  g_free(yyvsp[0].sval);
	}
    break;

  case 21:
#line 281 "printrcy.y"
    {
	  if (current_printer)
	    {
	      stp_set_file_parameter(current_printer->v, yyvsp[-3].sval, yyvsp[0].sval);
	      if (strcmp(yyvsp[-1].sval, "False") == 0)
		stp_set_file_parameter_active(current_printer->v, yyvsp[-3].sval,
					      STP_PARAMETER_INACTIVE);
	      else
		stp_set_file_parameter_active(current_printer->v, yyvsp[-3].sval,
					      STP_PARAMETER_ACTIVE);
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	  g_free(yyvsp[0].sval);
	}
    break;

  case 22:
#line 299 "printrcy.y"
    {
	  if (current_printer)
	    {
	      stp_set_float_parameter(current_printer->v, yyvsp[-3].sval, yyvsp[0].dval);
	      if (strcmp(yyvsp[-1].sval, "False") == 0)
		stp_set_float_parameter_active(current_printer->v, yyvsp[-3].sval,
					       STP_PARAMETER_INACTIVE);
	      else
		stp_set_float_parameter_active(current_printer->v, yyvsp[-3].sval,
					       STP_PARAMETER_ACTIVE);
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	}
    break;

  case 23:
#line 316 "printrcy.y"
    {
	  if (current_printer)
	    {
	      stp_set_dimension_parameter(current_printer->v, yyvsp[-3].sval, yyvsp[0].ival);
	      if (strcmp(yyvsp[-1].sval, "False") == 0)
		stp_set_dimension_parameter_active(current_printer->v, yyvsp[-3].sval,
						   STP_PARAMETER_INACTIVE);
	      else
		stp_set_dimension_parameter_active(current_printer->v, yyvsp[-3].sval,
						   STP_PARAMETER_ACTIVE);
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	}
    break;

  case 24:
#line 333 "printrcy.y"
    {
	  if (current_printer)
	    {
	      if (strcmp(yyvsp[0].sval, "False") == 0)
		stp_set_boolean_parameter(current_printer->v, yyvsp[-3].sval, 0);
	      else
		stp_set_boolean_parameter(current_printer->v, yyvsp[-3].sval, 1);
	      if (strcmp(yyvsp[-1].sval, "False") == 0)
		stp_set_boolean_parameter_active(current_printer->v, yyvsp[-3].sval,
						 STP_PARAMETER_INACTIVE);
	      else
		stp_set_boolean_parameter_active(current_printer->v, yyvsp[-3].sval,
						 STP_PARAMETER_ACTIVE);
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	  g_free(yyvsp[0].sval);
	}
    break;

  case 25:
#line 354 "printrcy.y"
    {
	  if (current_printer)
	    {
	      stp_curve_t *curve = stp_curve_create_from_string(yyvsp[0].sval);
	      if (curve)
		{
		  stp_set_curve_parameter(current_printer->v, yyvsp[-3].sval, curve);
		  if (strcmp(yyvsp[-1].sval, "False") == 0)
		    stp_set_curve_parameter_active(current_printer->v, yyvsp[-3].sval,
						   STP_PARAMETER_INACTIVE);
		  else
		    stp_set_curve_parameter_active(current_printer->v, yyvsp[-3].sval,
						   STP_PARAMETER_ACTIVE);
		  stp_curve_destroy(curve);
		}
	    }
	  g_free(yyvsp[-3].sval);
	  g_free(yyvsp[-1].sval);
	  g_free(yyvsp[0].sval);
	}
    break;

  case 56:
#line 402 "printrcy.y"
    { stpui_printrc_current_printer = yyvsp[0].sval; }
    break;

  case 57:
#line 406 "printrcy.y"
    {
	  if (strcmp(yyvsp[0].sval, "True") == 0)
	    stpui_show_all_paper_sizes = 1;
	  else
	    stpui_show_all_paper_sizes = 0;
	  g_free(yyvsp[0].sval);
	}
    break;

  case 61:
#line 422 "printrcy.y"
    {
	  if (yyvsp[0].sval)
	    {
	      stpui_set_global_parameter(yyvsp[-1].sval, yyvsp[0].sval);
	      g_free(yyvsp[0].sval);
	    }
	  g_free(yyvsp[-1].sval);
	}
    break;


    }

/* Line 991 of yacc.c.  */
#line 1507 "printrcy.c"

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
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__) \
    && !defined __cplusplus
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


#line 447 "printrcy.y"


