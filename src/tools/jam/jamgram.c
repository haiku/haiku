#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28 2000/01/17 02:04:06 bde Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
static int yygrowstack();
#define YYPREFIX "yy"
#line 89 "jamgram.y"
#include "jam.h"

#include "lists.h"
#include "parse.h"
#include "scan.h"
#include "compile.h"
#include "newstr.h"
#include "rules.h"

# define YYMAXDEPTH 10000	/* for OSF and other less endowed yaccs */

# define F0 (LIST *(*)(PARSE *, LOL *))0
# define P0 (PARSE *)0
# define S0 (char *)0

# define pappend( l,r )    	parse_make( compile_append,l,r,P0,S0,S0,0 )
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
# define psetc( s,p )     	parse_make( compile_setcomp,p,P0,P0,s,S0,0 )
# define psete( s,l,s1,f ) 	parse_make( compile_setexec,l,P0,P0,s,s1,f )
# define pswitch( l,r )   	parse_make( compile_switch,l,r,P0,S0,S0,0 )
# define pwhile( l,r )   	parse_make( compile_while,l,r,P0,S0,S0,0 )

# define pnode( l,r )    	parse_make( F0,l,r,P0,S0,S0,0 )
# define psnode( s,l )     	parse_make( F0,l,P0,P0,s,S0,0 )

#line 54 "y.tab.c"
#define YYERRCODE 256
#define _BANG_t 257
#define _BANG_EQUALS_t 258
#define _AMPER_t 259
#define _AMPERAMPER_t 260
#define _LPAREN_t 261
#define _RPAREN_t 262
#define _PLUS_EQUALS_t 263
#define _COLON_t 264
#define _SEMIC_t 265
#define _LANGLE_t 266
#define _LANGLE_EQUALS_t 267
#define _EQUALS_t 268
#define _RANGLE_t 269
#define _RANGLE_EQUALS_t 270
#define _QUESTION_EQUALS_t 271
#define _LBRACKET_t 272
#define _RBRACKET_t 273
#define ACTIONS_t 274
#define BIND_t 275
#define CASE_t 276
#define DEFAULT_t 277
#define ELSE_t 278
#define EXISTING_t 279
#define FOR_t 280
#define IF_t 281
#define IGNORE_t 282
#define IN_t 283
#define INCLUDE_t 284
#define LOCAL_t 285
#define ON_t 286
#define PIECEMEAL_t 287
#define QUIETLY_t 288
#define RETURN_t 289
#define RULE_t 290
#define SWITCH_t 291
#define TOGETHER_t 292
#define UPDATED_t 293
#define WHILE_t 294
#define _LBRACE_t 295
#define _BAR_t 296
#define _BARBAR_t 297
#define _RBRACE_t 298
#define ARG 299
#define STRING 300
const short yylhs[] = {                                        -1,
    0,    0,    2,    2,    1,    1,    1,    1,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,   12,   13,    3,    7,    7,    7,    7,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    8,    8,   14,    6,    6,    4,   15,   15,
    5,   17,    5,   16,   16,   16,   10,   10,   18,   18,
   18,   18,   18,   18,   11,   11,
};
const short yylen[] = {                                         2,
    0,    1,    0,    1,    1,    2,    4,    6,    3,    3,
    3,    4,    6,    3,    7,    5,    5,    7,    5,    3,
    3,    0,    0,    9,    1,    1,    1,    2,    1,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    2,    3,    0,    2,    4,    1,    3,    1,    0,    2,
    1,    0,    4,    2,    4,    4,    0,    2,    1,    1,
    1,    1,    1,    1,    0,    2,
};
const short yydefred[] = {                                      0,
   52,   57,    0,    0,   49,   49,    0,   49,    0,   49,
    0,    0,   51,    0,    2,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    4,    0,    6,   26,   25,   27,    0,   49,
    0,    0,   49,    0,   49,    0,   64,   61,   63,   62,
   60,   59,    0,   58,   49,   41,    0,   49,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   10,
   50,    0,   49,   21,   14,   20,    0,    0,    9,   28,
    0,   49,   11,    0,    0,   54,   53,   49,    0,    0,
   42,   40,    0,    0,    0,   32,   33,    0,   34,   35,
    0,    0,    0,    7,    0,    0,    0,    0,    0,   49,
   47,   12,   49,   49,   66,   22,    0,    0,    0,    0,
   16,   44,   19,    0,   56,   55,    0,    0,    0,    8,
    0,   13,   23,   15,   18,   45,    0,   24,
};
const short yydgoto[] = {                                      14,
   33,   34,   16,   41,   23,   42,   43,  107,   24,   19,
   89,  127,  137,  108,   26,   46,   18,   54,
};
const short yysindex[] = {                                    -44,
    0,    0, -278, -232,    0,    0, -191,    0, -273,    0,
 -232,  -44,    0,    0,    0,  -44, -154, -245,  -66, -252,
 -232, -232, -250, -103, -228, -191, -249,   31, -206,   31,
 -230,  -90,    0, -219,    0,    0,    0,    0, -200,    0,
 -194, -180,    0, -191,    0, -187,    0,    0,    0,    0,
    0,    0, -172,    0,    0,    0,  -58,    0, -232, -232,
 -232, -232, -232, -232, -232, -232,  -44, -232, -232,    0,
    0,  -44,    0,    0,    0,    0, -169,  -44,    0,    0,
  -48,    0,    0, -152, -233,    0,    0,    0, -179, -175,
    0,    0,  -35,   -1,   -1,    0,    0,  -35,    0,    0,
 -176,   -6,   -6,    0, -141, -174, -161, -169, -150,    0,
    0,    0,    0,    0,    0,    0,  -44, -148,  -44, -138,
    0,    0,    0,  -91,    0,    0, -125, -115,   31,    0,
  -44,    0,    0,    0,    0,    0, -113,    0,
};
const short yyrindex[] = {                                    186,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -102,    0,    0,    0,    3,  -74,    0,    0,    0,
    0,    0, -108,    0,    0, -124,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -223,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -100,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -102,    0,    0,    0,
    0,    4,    0,    0,    0,    0, -101, -102,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -224, -168,  -78,    0,    0, -207,    0,    0,
    0, -213, -185,    0,    0,    0,    0, -101,    0,    0,
    0,    0,    0,    0,    0,    0, -102,    1,    4,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -276,    0,    0,    0,    0,    0,    0,    0,
};
const short yygindex[] = {                                      0,
    8,  -55,  -23,    5,    2,  -39,  118,   95,   36,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 330
const short yytable[] = {                                       3,
   17,   17,    5,    3,   74,   86,   76,   15,   28,   25,
   27,  101,   29,   17,   31,   72,  104,   17,   73,   45,
   20,    3,  109,   35,   21,   30,    1,   71,   22,   17,
   55,   17,   58,   31,   31,   31,   70,   31,    1,    1,
   44,   46,  111,   31,   81,   85,   32,   84,   38,   46,
   30,   30,   30,   13,   30,  113,   56,   57,   75,   90,
   30,  128,   92,  130,   77,   13,   13,   80,   17,   82,
   31,   31,   31,   17,  126,  136,   39,  105,   79,   17,
    1,   38,   38,   38,   83,   87,  114,   30,   30,   30,
   36,   36,  115,   36,   93,   94,   95,   96,   97,   98,
   99,  100,   88,  102,  103,  135,  106,   13,   36,   39,
   39,   39,  112,   37,  124,  116,   38,  125,   17,  117,
   17,  118,   39,  119,  120,  131,   36,   36,   36,  129,
   17,   40,   17,   48,   48,   48,  121,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,  123,   48,   29,
   29,   29,   48,   29,   59,   60,   61,   29,   29,   29,
   29,   29,   62,   63,   64,   65,   66,   59,   60,   61,
   48,   48,   48,  132,  133,   62,   63,   64,   65,   66,
   37,   37,  134,   37,  138,    1,   29,   29,   29,   49,
   49,   67,   68,   69,   65,    3,   43,   49,  110,   59,
   60,   61,  122,   91,   78,   68,   69,   62,   63,   64,
   65,   66,   47,    0,   36,   48,   37,   37,   37,   37,
   49,   50,   38,    0,   49,   51,   52,    1,   39,    2,
   62,   63,   53,   65,   66,    3,    4,   68,   69,    5,
    6,    7,    0,    0,    8,    9,   10,    0,    0,   11,
   12,   59,   60,   61,   13,    0,   59,    0,    0,   62,
   63,   64,   65,   66,   62,   63,   64,   65,   66,    0,
    0,    0,   17,    0,   17,    0,   17,    0,    5,    3,
   17,   17,    0,    0,   17,   17,   17,    0,    0,   17,
   17,   17,    0,    0,   17,   17,    0,    0,   17,   17,
    5,    3,    1,    0,    2,    0,    0,    0,    0,    0,
    3,    4,    0,    0,    5,    0,    7,    0,    0,    8,
    9,   10,    0,    0,   11,   12,    0,    0,    0,   13,
};
const short yycheck[] = {                                     276,
    0,    0,    0,    0,   28,   45,   30,    0,    7,    5,
    6,   67,    8,   12,   10,  265,   72,   16,  268,   18,
  299,  298,   78,   16,  257,  299,  272,   26,  261,   28,
  283,   30,  283,  258,  259,  260,  265,  262,  272,  272,
  286,  265,   82,  268,   40,   44,   11,   43,  262,  273,
  258,  259,  260,  299,  262,  289,   21,   22,  265,   55,
  268,  117,   58,  119,  295,  299,  299,  268,   67,  264,
  295,  296,  297,   72,  114,  131,  262,   73,  298,   78,
  272,  295,  296,  297,  265,  273,   85,  295,  296,  297,
  259,  260,   88,  262,   59,   60,   61,   62,   63,   64,
   65,   66,  275,   68,   69,  129,  276,  299,  263,  295,
  296,  297,  265,  268,  110,  295,  271,  113,  117,  295,
  119,  298,  277,  265,  299,  264,  295,  296,  297,  278,
  129,  286,  131,  258,  259,  260,  298,  262,  263,  264,
  265,  266,  267,  268,  269,  270,  271,  298,  273,  258,
  259,  260,  277,  262,  258,  259,  260,  266,  267,  268,
  269,  270,  266,  267,  268,  269,  270,  258,  259,  260,
  295,  296,  297,  265,  300,  266,  267,  268,  269,  270,
  259,  260,  298,  262,  298,    0,  295,  296,  297,  264,
  265,  295,  296,  297,  295,  298,  298,  272,   81,  258,
  259,  260,  108,  262,  295,  296,  297,  266,  267,  268,
  269,  270,  279,   -1,  263,  282,  295,  296,  297,  268,
  287,  288,  271,   -1,  299,  292,  293,  272,  277,  274,
  266,  267,  299,  269,  270,  280,  281,  296,  297,  284,
  285,  286,   -1,   -1,  289,  290,  291,   -1,   -1,  294,
  295,  258,  259,  260,  299,   -1,  258,   -1,   -1,  266,
  267,  268,  269,  270,  266,  267,  268,  269,  270,   -1,
   -1,   -1,  272,   -1,  274,   -1,  276,   -1,  276,  276,
  280,  281,   -1,   -1,  284,  285,  286,   -1,   -1,  289,
  290,  291,   -1,   -1,  294,  295,   -1,   -1,  298,  299,
  298,  298,  272,   -1,  274,   -1,   -1,   -1,   -1,   -1,
  280,  281,   -1,   -1,  284,   -1,  286,   -1,   -1,  289,
  290,  291,   -1,   -1,  294,  295,   -1,   -1,   -1,  299,
};
#define YYFINAL 14
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 300
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"_BANG_t","_BANG_EQUALS_t",
"_AMPER_t","_AMPERAMPER_t","_LPAREN_t","_RPAREN_t","_PLUS_EQUALS_t","_COLON_t",
"_SEMIC_t","_LANGLE_t","_LANGLE_EQUALS_t","_EQUALS_t","_RANGLE_t",
"_RANGLE_EQUALS_t","_QUESTION_EQUALS_t","_LBRACKET_t","_RBRACKET_t","ACTIONS_t",
"BIND_t","CASE_t","DEFAULT_t","ELSE_t","EXISTING_t","FOR_t","IF_t","IGNORE_t",
"IN_t","INCLUDE_t","LOCAL_t","ON_t","PIECEMEAL_t","QUIETLY_t","RETURN_t",
"RULE_t","SWITCH_t","TOGETHER_t","UPDATED_t","WHILE_t","_LBRACE_t","_BAR_t",
"_BARBAR_t","_RBRACE_t","ARG","STRING",
};
const char * const yyrule[] = {
"$accept : run",
"run :",
"run : rules",
"block :",
"block : rules",
"rules : rule",
"rules : rule rules",
"rules : LOCAL_t list _SEMIC_t block",
"rules : LOCAL_t list _EQUALS_t list _SEMIC_t block",
"rule : _LBRACE_t block _RBRACE_t",
"rule : INCLUDE_t list _SEMIC_t",
"rule : arg lol _SEMIC_t",
"rule : arg assign list _SEMIC_t",
"rule : arg ON_t list assign list _SEMIC_t",
"rule : RETURN_t list _SEMIC_t",
"rule : FOR_t ARG IN_t list _LBRACE_t block _RBRACE_t",
"rule : SWITCH_t list _LBRACE_t cases _RBRACE_t",
"rule : IF_t expr _LBRACE_t block _RBRACE_t",
"rule : IF_t expr _LBRACE_t block _RBRACE_t ELSE_t rule",
"rule : WHILE_t expr _LBRACE_t block _RBRACE_t",
"rule : RULE_t ARG rule",
"rule : ON_t arg rule",
"$$1 :",
"$$2 :",
"rule : ACTIONS_t eflags ARG bindlist _LBRACE_t $$1 STRING $$2 _RBRACE_t",
"assign : _EQUALS_t",
"assign : _PLUS_EQUALS_t",
"assign : _QUESTION_EQUALS_t",
"assign : DEFAULT_t _EQUALS_t",
"expr : arg",
"expr : expr _EQUALS_t expr",
"expr : expr _BANG_EQUALS_t expr",
"expr : expr _LANGLE_t expr",
"expr : expr _LANGLE_EQUALS_t expr",
"expr : expr _RANGLE_t expr",
"expr : expr _RANGLE_EQUALS_t expr",
"expr : expr _AMPER_t expr",
"expr : expr _AMPERAMPER_t expr",
"expr : expr _BAR_t expr",
"expr : expr _BARBAR_t expr",
"expr : arg IN_t list",
"expr : _BANG_t expr",
"expr : _LPAREN_t expr _RPAREN_t",
"cases :",
"cases : case cases",
"case : CASE_t ARG _COLON_t block",
"lol : list",
"lol : list _COLON_t lol",
"list : listp",
"listp :",
"listp : listp arg",
"arg : ARG",
"$$3 :",
"arg : _LBRACKET_t $$3 func _RBRACKET_t",
"func : arg lol",
"func : ON_t arg arg lol",
"func : ON_t arg RETURN_t list",
"eflags :",
"eflags : eflags eflag",
"eflag : UPDATED_t",
"eflag : TOGETHER_t",
"eflag : IGNORE_t",
"eflag : QUIETLY_t",
"eflag : PIECEMEAL_t",
"eflag : EXISTING_t",
"bindlist :",
"bindlist : BIND_t list",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 132 "jamgram.y"
{ parse_save( yyvsp[0].parse ); }
break;
case 3:
#line 143 "jamgram.y"
{ yyval.parse = pnull(); }
break;
case 4:
#line 145 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; }
break;
case 5:
#line 149 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; }
break;
case 6:
#line 151 "jamgram.y"
{ yyval.parse = prules( yyvsp[-1].parse, yyvsp[0].parse ); }
break;
case 7:
#line 153 "jamgram.y"
{ yyval.parse = plocal( yyvsp[-2].parse, pnull(), yyvsp[0].parse ); }
break;
case 8:
#line 155 "jamgram.y"
{ yyval.parse = plocal( yyvsp[-4].parse, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 9:
#line 159 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; }
break;
case 10:
#line 161 "jamgram.y"
{ yyval.parse = pincl( yyvsp[-1].parse ); }
break;
case 11:
#line 163 "jamgram.y"
{ yyval.parse = prule( yyvsp[-2].parse, yyvsp[-1].parse ); }
break;
case 12:
#line 165 "jamgram.y"
{ yyval.parse = pset( yyvsp[-3].parse, yyvsp[-1].parse, yyvsp[-2].number ); }
break;
case 13:
#line 167 "jamgram.y"
{ yyval.parse = pset1( yyvsp[-5].parse, yyvsp[-3].parse, yyvsp[-1].parse, yyvsp[-2].number ); }
break;
case 14:
#line 169 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; }
break;
case 15:
#line 171 "jamgram.y"
{ yyval.parse = pfor( yyvsp[-5].string, yyvsp[-3].parse, yyvsp[-1].parse ); }
break;
case 16:
#line 173 "jamgram.y"
{ yyval.parse = pswitch( yyvsp[-3].parse, yyvsp[-1].parse ); }
break;
case 17:
#line 175 "jamgram.y"
{ yyval.parse = pif( yyvsp[-3].parse, yyvsp[-1].parse, pnull() ); }
break;
case 18:
#line 177 "jamgram.y"
{ yyval.parse = pif( yyvsp[-5].parse, yyvsp[-3].parse, yyvsp[0].parse ); }
break;
case 19:
#line 179 "jamgram.y"
{ yyval.parse = pwhile( yyvsp[-3].parse, yyvsp[-1].parse ); }
break;
case 20:
#line 181 "jamgram.y"
{ yyval.parse = psetc( yyvsp[-1].string, yyvsp[0].parse ); }
break;
case 21:
#line 183 "jamgram.y"
{ yyval.parse = pon( yyvsp[-1].parse, yyvsp[0].parse ); }
break;
case 22:
#line 185 "jamgram.y"
{ yymode( SCAN_STRING ); }
break;
case 23:
#line 187 "jamgram.y"
{ yymode( SCAN_NORMAL ); }
break;
case 24:
#line 189 "jamgram.y"
{ yyval.parse = psete( yyvsp[-6].string,yyvsp[-5].parse,yyvsp[-2].string,yyvsp[-7].number ); }
break;
case 25:
#line 197 "jamgram.y"
{ yyval.number = ASSIGN_SET; }
break;
case 26:
#line 199 "jamgram.y"
{ yyval.number = ASSIGN_APPEND; }
break;
case 27:
#line 201 "jamgram.y"
{ yyval.number = ASSIGN_DEFAULT; }
break;
case 28:
#line 203 "jamgram.y"
{ yyval.number = ASSIGN_DEFAULT; }
break;
case 29:
#line 211 "jamgram.y"
{ yyval.parse = peval( EXPR_EXISTS, yyvsp[0].parse, pnull() ); }
break;
case 30:
#line 213 "jamgram.y"
{ yyval.parse = peval( EXPR_EQUALS, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 31:
#line 215 "jamgram.y"
{ yyval.parse = peval( EXPR_NOTEQ, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 32:
#line 217 "jamgram.y"
{ yyval.parse = peval( EXPR_LESS, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 33:
#line 219 "jamgram.y"
{ yyval.parse = peval( EXPR_LESSEQ, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 34:
#line 221 "jamgram.y"
{ yyval.parse = peval( EXPR_MORE, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 35:
#line 223 "jamgram.y"
{ yyval.parse = peval( EXPR_MOREEQ, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 36:
#line 225 "jamgram.y"
{ yyval.parse = peval( EXPR_AND, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 37:
#line 227 "jamgram.y"
{ yyval.parse = peval( EXPR_AND, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 38:
#line 229 "jamgram.y"
{ yyval.parse = peval( EXPR_OR, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 39:
#line 231 "jamgram.y"
{ yyval.parse = peval( EXPR_OR, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 40:
#line 233 "jamgram.y"
{ yyval.parse = peval( EXPR_IN, yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 41:
#line 235 "jamgram.y"
{ yyval.parse = peval( EXPR_NOT, yyvsp[0].parse, pnull() ); }
break;
case 42:
#line 237 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; }
break;
case 43:
#line 247 "jamgram.y"
{ yyval.parse = P0; }
break;
case 44:
#line 249 "jamgram.y"
{ yyval.parse = pnode( yyvsp[-1].parse, yyvsp[0].parse ); }
break;
case 45:
#line 253 "jamgram.y"
{ yyval.parse = psnode( yyvsp[-2].string, yyvsp[0].parse ); }
break;
case 46:
#line 262 "jamgram.y"
{ yyval.parse = pnode( P0, yyvsp[0].parse ); }
break;
case 47:
#line 264 "jamgram.y"
{ yyval.parse = pnode( yyvsp[0].parse, yyvsp[-2].parse ); }
break;
case 48:
#line 274 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; yymode( SCAN_NORMAL ); }
break;
case 49:
#line 278 "jamgram.y"
{ yyval.parse = pnull(); yymode( SCAN_PUNCT ); }
break;
case 50:
#line 280 "jamgram.y"
{ yyval.parse = pappend( yyvsp[-1].parse, yyvsp[0].parse ); }
break;
case 51:
#line 284 "jamgram.y"
{ yyval.parse = plist( yyvsp[0].string ); }
break;
case 52:
#line 285 "jamgram.y"
{ yymode( SCAN_NORMAL ); }
break;
case 53:
#line 286 "jamgram.y"
{ yyval.parse = yyvsp[-1].parse; }
break;
case 54:
#line 295 "jamgram.y"
{ yyval.parse = prule( yyvsp[-1].parse, yyvsp[0].parse ); }
break;
case 55:
#line 297 "jamgram.y"
{ yyval.parse = pon( yyvsp[-2].parse, prule( yyvsp[-1].parse, yyvsp[0].parse ) ); }
break;
case 56:
#line 299 "jamgram.y"
{ yyval.parse = pon( yyvsp[-2].parse, yyvsp[0].parse ); }
break;
case 57:
#line 308 "jamgram.y"
{ yyval.number = 0; }
break;
case 58:
#line 310 "jamgram.y"
{ yyval.number = yyvsp[-1].number | yyvsp[0].number; }
break;
case 59:
#line 314 "jamgram.y"
{ yyval.number = RULE_UPDATED; }
break;
case 60:
#line 316 "jamgram.y"
{ yyval.number = RULE_TOGETHER; }
break;
case 61:
#line 318 "jamgram.y"
{ yyval.number = RULE_IGNORE; }
break;
case 62:
#line 320 "jamgram.y"
{ yyval.number = RULE_QUIETLY; }
break;
case 63:
#line 322 "jamgram.y"
{ yyval.number = RULE_PIECEMEAL; }
break;
case 64:
#line 324 "jamgram.y"
{ yyval.number = RULE_EXISTING; }
break;
case 65:
#line 333 "jamgram.y"
{ yyval.parse = pnull(); }
break;
case 66:
#line 335 "jamgram.y"
{ yyval.parse = yyvsp[0].parse; }
break;
#line 821 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
