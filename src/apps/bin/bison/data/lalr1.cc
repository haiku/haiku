m4_divert(-1)
# C++ skeleton for Bison
# Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307  USA

## ---------------- ##
## Default values.  ##
## ---------------- ##

# Default Parser class name.
m4_define_default([b4_parser_class_name], [Parser])



## ----------------- ##
## Semantic Values.  ##
## ----------------- ##


# b4_lhs_value([TYPE])
# --------------------
# Expansion of $<TYPE>$.
m4_define([b4_lhs_value],
[yyval[]m4_ifval([$1], [.$1])])


# b4_rhs_value(RULE-LENGTH, NUM, [TYPE])
# --------------------------------------
# Expansion of $<TYPE>NUM, where the current rule has RULE-LENGTH
# symbols on RHS.
m4_define([b4_rhs_value],
[semantic_stack_@{m4_eval([$1 - $2])@}m4_ifval([$3], [.$3])])

m4_define_default([b4_location_type], [Location])

# b4_lhs_location()
# -----------------
# Expansion of @$.
m4_define([b4_lhs_location],
[yyloc])


# b4_rhs_location(RULE-LENGTH, NUM)
# ---------------------------------
# Expansion of @NUM, where the current rule has RULE-LENGTH symbols
# on RHS.
m4_define([b4_rhs_location],
[location_stack_@{m4_eval([$1 - $2])@}])


m4_define([b4_inherit],
          [m4_ifdef([b4_root],
		    [: public b4_root
],
		    [])])

m4_define([b4_param],
	  [m4_ifdef([b4_root],
	            [,
            const Param& param],
		    [])])

m4_define([b4_constructor],
	  [m4_ifdef([b4_root],
		    [b4_root (param),
      ],
		    [])])


# b4_parse_param_decl
# -------------------
#  Constructor's extra arguments.
m4_define([b4_parse_param_decl],
          [m4_ifset([b4_parse_param], [, b4_c_ansi_formals(b4_parse_param)])])

# b4_parse_param_cons
# -------------------
#  constructor's extra initialisations.
m4_define([b4_parse_param_cons],
          [m4_ifset([b4_parse_param],
		    [,
      b4_cc_constructor_calls(b4_parse_param)])])
m4_define([b4_cc_constructor_calls],
	  [m4_map_sep([b4_cc_constructor_call], [,
      ], [$@])])
m4_define([b4_cc_constructor_call],
	  [$2($2)])

# b4_parse_param_vars
# -------------------
#  Extra instance variables.
m4_define([b4_parse_param_vars],
          [m4_ifset([b4_parse_param],
		    [
    /* User arguments.  */
b4_cc_var_decls(b4_parse_param)])])
m4_define([b4_cc_var_decls],
	  [m4_map_sep([b4_cc_var_decl], [
], [$@])])
m4_define([b4_cc_var_decl],
	  [    $1;])

# b4_cxx_destruct_def(IGNORED-ARGUMENTS)
# --------------------------------------
# Declare the destruct_ method.
m4_define([b4_cxx_destruct_def],
[void
yy::b4_parser_class_name::destruct_ (int yytype, SemanticType *yyvaluep, LocationType *yylocationp)[]dnl
])

# We do want M4 expansion after # for CPP macros.
m4_changecom()
m4_divert(0)dnl
m4_if(b4_defines_flag, 0, [],
[@output @output_header_name@
b4_copyright([C++ Skeleton parser for LALR(1) parsing with Bison],
             [2002, 2003, 2004])[
/* FIXME: This is wrong, we want computed header guards.
   I don't know why the macros are missing now. :( */
#ifndef PARSER_HEADER_H
# define PARSER_HEADER_H

#include "stack.hh"
#include "location.hh"

#include <string>
#include <iostream>

/* Using locations.  */
#define YYLSP_NEEDED ]b4_locations_flag[

]b4_token_defines(b4_tokens)[

/* Copy the first part of user declarations.  */
]b4_pre_prologue[

]/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])[

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG ]b4_debug[
#endif

/* Enabling verbose error message.  */
#ifndef YYERROR_VERBOSE
# define YYERROR_VERBOSE ]b4_error_verbose[
#endif

#ifndef YYSTYPE
]m4_ifdef([b4_stype],
[b4_syncline([b4_stype_line], [b4_filename])
typedef union b4_stype yystype;
/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])],
[typedef int yystype;])[
# define YYSTYPE yystype
#endif

/* Copy the second part of user declarations.  */
]b4_post_prologue[

]/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])[
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N) \
   ((Current).end = Rhs[N].end)
#endif

namespace yy
{
  class ]b4_parser_class_name[;

  template < typename P >
  struct Traits
  {
  };

  template < >
  struct Traits< ]b4_parser_class_name[ >
  {
    typedef ]b4_int_type_for([b4_translate])[ TokenNumberType;
    typedef ]b4_int_type_for([b4_rhs])[       RhsNumberType;
    typedef int      StateType;
    typedef yystype  SemanticType;
    typedef ]b4_location_type[ LocationType;
  };
}

namespace yy
{
  class ]b4_parser_class_name b4_inherit[
  {
  public:

    typedef Traits< ]b4_parser_class_name[ >::TokenNumberType TokenNumberType;
    typedef Traits< ]b4_parser_class_name[ >::RhsNumberType   RhsNumberType;
    typedef Traits< ]b4_parser_class_name[ >::StateType       StateType;
    typedef Traits< ]b4_parser_class_name[ >::SemanticType    SemanticType;
    typedef Traits< ]b4_parser_class_name[ >::LocationType    LocationType;

    typedef Stack< StateType >    StateStack;
    typedef Stack< SemanticType > SemanticStack;
    typedef Stack< LocationType > LocationStack;

#if YYLSP_NEEDED
    ]b4_parser_class_name[ (bool debug,
	    LocationType initlocation][]b4_param[]b4_parse_param_decl[) :
      ]b4_constructor[][debug_ (debug),
      cdebug_ (std::cerr),
      initlocation_ (initlocation)]b4_parse_param_cons[
#else
    ]b4_parser_class_name[ (bool debug][]b4_param[]b4_parse_param_decl[) :
      ]b4_constructor[][debug_ (debug),
      cdebug_ (std::cerr)]b4_parse_param_cons[
#endif
    {
    }

    virtual ~]b4_parser_class_name[ ()
    {
    }

    virtual int parse ();

  private:

    virtual void lex_ ();
    virtual void error_ ();
    virtual void print_ ();
    virtual void report_syntax_error_ ();

    /* Stacks.  */
    StateStack    state_stack_;
    SemanticStack semantic_stack_;
    LocationStack location_stack_;

    /* Tables.  */
    static const ]b4_int_type_for([b4_pact])[ pact_[];
    static const ]b4_int_type(b4_pact_ninf, b4_pact_ninf)[ pact_ninf_;
    static const ]b4_int_type_for([b4_defact])[ defact_[];
    static const ]b4_int_type_for([b4_pgoto])[ pgoto_[];
    static const ]b4_int_type_for([b4_defgoto])[ defgoto_[];
    static const ]b4_int_type_for([b4_table])[ table_[];
    static const ]b4_int_type(b4_table_ninf, b4_table_ninf)[ table_ninf_;
    static const ]b4_int_type_for([b4_check])[ check_[];
    static const ]b4_int_type_for([b4_stos])[ stos_[];
    static const ]b4_int_type_for([b4_r1])[ r1_[];
    static const ]b4_int_type_for([b4_r2])[ r2_[];

#if YYDEBUG || YYERROR_VERBOSE
    static const char* const name_[];
#endif

    /* More tables, for debugging.  */
#if YYDEBUG
    static const RhsNumberType rhs_[];
    static const ]b4_int_type_for([b4_prhs])[ prhs_[];
    static const ]b4_int_type_for([b4_rline])[ rline_[];
    static const ]b4_int_type_for([b4_toknum])[ token_number_[];
    virtual void reduce_print_ (int yyrule);
    virtual void stack_print_ ();
#endif

    /* Even more tables.  */
    static inline TokenNumberType translate_ (int token);
    static inline void destruct_ (int yytype, SemanticType *yyvaluep,
				  LocationType *yylocationp);

    /* Constants.  */
    static const int eof_;
    /* LAST_ -- Last index in TABLE_.  */
    static const int last_;
    static const int nnts_;
    static const int empty_;
    static const int final_;
    static const int terror_;
    static const int errcode_;
    static const int ntokens_;
    static const unsigned int user_token_number_max_;
    static const TokenNumberType undef_token_;

    /* State.  */
    int n_;
    int len_;
    int state_;

    /* Error handling. */
    int nerrs_;
    int errstatus_;

    /* Debugging.  */
    int debug_;
    std::ostream &cdebug_;

    /* Lookahead and lookahead in internal form.  */
    int looka_;
    int ilooka_;

    /* Message.  */
    std::string message;

    /* Semantic value and location of lookahead token.  */
    SemanticType value;
    LocationType location;
    /* Beginning of the last erroneous token popped off.  */
    Position error_start_;

    /* @@$ and $$.  */
    SemanticType yyval;
    LocationType yyloc;

    /* Initial location.  */
    LocationType initlocation_;
]b4_parse_param_vars[
  };
}

#endif /* ! defined PARSER_HEADER_H */]
])dnl
@output @output_parser_name@
b4_copyright([C++ Skeleton parser for LALR(1) parsing with Bison],
             [2002, 2003, 2004])

m4_if(b4_defines_flag, 0, [], [#include @output_header_name@])[

/* Enable debugging if requested.  */
#if YYDEBUG
# define YYCDEBUG    if (debug_) cdebug_
# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (debug_)				\
    reduce_print_ (Rule);		\
} while (0)
# define YY_STACK_PRINT()		\
do {					\
  if (debug_)				\
    stack_print_ ();			\
} while (0)
#else /* !YYDEBUG */
# define YYCDEBUG    if (0) cdebug_
# define YY_REDUCE_PRINT(Rule)
# define YY_STACK_PRINT()
#endif /* !YYDEBUG */

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab

]b4_yydestruct_generate([b4_cxx_destruct_def])[

int
yy::]b4_parser_class_name[::parse ()
{
  nerrs_ = 0;
  errstatus_ = 0;

  /* Initialize the stacks.  The initial state will be pushed in
     yynewstate, since the latter expects the semantical and the
     location values to have been already stored, initialize these
     stacks with a primary value.  */
  state_stack_ = StateStack (0);
  semantic_stack_ = SemanticStack (1);
  location_stack_ = LocationStack (1);

  /* Start.  */
  state_ = 0;
  looka_ = empty_;
#if YYLSP_NEEDED
  location = initlocation_;
#endif
  YYCDEBUG << "Starting parse" << std::endl;

  /* New state.  */
 yynewstate:
  state_stack_.push (state_);
  YYCDEBUG << "Entering state " << state_ << std::endl;
  goto yybackup;

  /* Backup.  */
 yybackup:

  /* Try to take a decision without lookahead.  */
  n_ = pact_[state_];
  if (n_ == pact_ninf_)
    goto yydefault;

  /* Read a lookahead token.  */
  if (looka_ == empty_)
    {
      YYCDEBUG << "Reading a token: ";
      lex_ ();
    }

  /* Convert token to internal form.  */
  if (looka_ <= 0)
    {
      looka_ = eof_;
      ilooka_ = 0;
      YYCDEBUG << "Now at end of input." << std::endl;
    }
  else
    {
      ilooka_ = translate_ (looka_);
#if YYDEBUG
      if (debug_)
	{
	  YYCDEBUG << "Next token is " << looka_
		 << " (" << name_[ilooka_];
	  print_ ();
	  YYCDEBUG << ')' << std::endl;
	}
#endif
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  n_ += ilooka_;
  if (n_ < 0 || last_ < n_ || check_[n_] != ilooka_)
    goto yydefault;

  /* Reduce or error.  */
  n_ = table_[n_];
  if (n_ < 0)
    {
      if (n_ == table_ninf_)
	goto yyerrlab;
      else
	{
	  n_ = -n_;
	  goto yyreduce;
	}
    }
  else if (n_ == 0)
    goto yyerrlab;

  /* Accept?  */
  if (n_ == final_)
    goto yyacceptlab;

  /* Shift the lookahead token.  */
#if YYDEBUG
  YYCDEBUG << "Shifting token " << looka_
           << " (" << name_[ilooka_] << "), ";
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (looka_ != eof_)
    looka_ = empty_;

  semantic_stack_.push (value);
  location_stack_.push (location);

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (errstatus_)
    --errstatus_;

  state_ = n_;
  goto yynewstate;

/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
 yydefault:
  n_ = defact_[state_];
  if (n_ == 0)
    goto yyerrlab;
  goto yyreduce;

/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
 yyreduce:
  len_ = r2_[n_];
  /* If LEN_ is nonzero, implement the default value of the action:
     `$$ = $1'.  Otherwise, use the top of the stack.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  */
  if (len_)
    {
      yyval = semantic_stack_[len_ - 1];
      yyloc = location_stack_[len_ - 1];
    }
  else
    {
      yyval = semantic_stack_[0];
      yyloc = location_stack_[0];
    }

  if (len_)
    {
      Slice< LocationType, LocationStack > slice (location_stack_, len_);
      YYLLOC_DEFAULT (yyloc, slice, len_);
    }
  YY_REDUCE_PRINT (n_);
  switch (n_)
    {
      ]b4_actions[
    }

]/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])[

  state_stack_.pop (len_);
  semantic_stack_.pop (len_);
  location_stack_.pop (len_);

  YY_STACK_PRINT ();

  semantic_stack_.push (yyval);
  location_stack_.push (yyloc);

  /* Shift the result of the reduction.  */
  n_ = r1_[n_];
  state_ = pgoto_[n_ - ntokens_] + state_stack_[0];
  if (0 <= state_ && state_ <= last_ && check_[state_] == state_stack_[0])
    state_ = table_[state_];
  else
    state_ = defgoto_[n_ - ntokens_];
  goto yynewstate;

/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
 yyerrlab:
  /* If not already recovering from an error, report this error.  */
  report_syntax_error_ ();

  error_start_ = location.begin;
  if (errstatus_ == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (looka_ <= eof_)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (looka_ == eof_)
	     for (;;)
	       {
                 error_start_ = location_stack_[0].begin;
                 state_stack_.pop ();
                 semantic_stack_.pop ();
                 location_stack_.pop ();
		 if (state_stack_.height () == 1)
		   YYABORT;
//		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
                 destruct_ (stos_[state_stack_[0]],
                            &semantic_stack_[0],
                            &location_stack_[0]);
	       }
        }
      else
        {
#if YYDEBUG
          YYCDEBUG << "Discarding token " << looka_
  	           << " (" << name_[ilooka_] << ")." << std::endl;
#endif
          destruct_ (ilooka_, &value, &location);
          looka_ = empty_;
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

  state_stack_.pop (len_);
  semantic_stack_.pop (len_);
  error_start_ = location_stack_[len_ - 1].begin;
  location_stack_.pop (len_);
  state_ = state_stack_[0];
  goto yyerrlab1;

/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  errstatus_ = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      n_ = pact_[state_];
      if (n_ != pact_ninf_)
	{
	  n_ += terror_;
	  if (0 <= n_ && n_ <= last_ && check_[n_] == terror_)
	    {
	      n_ = table_[n_];
	      if (0 < n_)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (state_stack_.height () == 1)
	YYABORT;

#if YYDEBUG
      if (debug_)
	{
	  if (stos_[state_] < ntokens_)
	    {
	      YYCDEBUG << "Error: popping token "
		     << token_number_[stos_[state_]]
		     << " (" << name_[stos_[state_]];
# ifdef YYPRINT
	      YYPRINT (stderr, token_number_[stos_[state_]],
		       semantic_stack_.top ());
# endif
	      YYCDEBUG << ')' << std::endl;
	    }
	  else
	    {
	      YYCDEBUG << "Error: popping nonterminal ("
		     << name_[stos_[state_]] << ')' << std::endl;
	    }
	}
#endif
      destruct_ (stos_[state_], &semantic_stack_[0], &location_stack_[0]);
      error_start_ = location_stack_[0].begin;

      state_stack_.pop ();
      semantic_stack_.pop ();
      location_stack_.pop ();
      state_ = state_stack_[0];
      YY_STACK_PRINT ();
    }

  if (n_ == final_)
    goto yyacceptlab;

  YYCDEBUG << "Shifting error token, ";

  {
    Location errloc;
    errloc.begin = error_start_;
    errloc.end = location.end;
    semantic_stack_.push (value);
    location_stack_.push (errloc);
  }

  state_ = n_;
  goto yynewstate;

  /* Accept.  */
 yyacceptlab:
  return 0;

  /* Abort.  */
 yyabortlab:
  return 1;
}

void
yy::]b4_parser_class_name[::lex_ ()
{
#if YYLSP_NEEDED
  looka_ = yylex (&value, &location);
#else
  looka_ = yylex (&value);
#endif
}

/** Generate an error message, and invoke yyerror. */
void
yy::]b4_parser_class_name[::report_syntax_error_ ()
{
  /* If not already recovering from an error, report this error.  */
  if (!errstatus_)
    {
      ++nerrs_;

#if YYERROR_VERBOSE
      n_ = pact_[state_];
      if (pact_ninf_ < n_ && n_ < last_)
	{
	  message = "syntax error, unexpected ";
	  message += name_[ilooka_];
	  {
	    int count = 0;
	    for (int x = (n_ < 0 ? -n_ : 0); x < ntokens_ + nnts_; ++x)
	      if (check_[x + n_] == x && x != terror_)
		++count;
	    if (count < 5)
	      {
		count = 0;
		for (int x = (n_ < 0 ? -n_ : 0); x < ntokens_ + nnts_; ++x)
		  if (check_[x + n_] == x && x != terror_)
		    {
		      message += (!count++) ? ", expecting " : " or ";
		      message += name_[x];
		    }
	      }
	  }
	}
      else
#endif
	message = "syntax error";
      error_ ();
    }
}


/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
const ]b4_int_type(b4_pact_ninf, b4_pact_ninf) yy::b4_parser_class_name::pact_ninf_ = b4_pact_ninf[;
const ]b4_int_type_for([b4_pact])[
yy::]b4_parser_class_name[::pact_[] =
{
  ]b4_pact[
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error.  */
const ]b4_int_type_for([b4_defact])[
yy::]b4_parser_class_name[::defact_[] =
{
  ]b4_defact[
};

/* YYPGOTO[NTERM-NUM].  */
const ]b4_int_type_for([b4_pgoto])[
yy::]b4_parser_class_name[::pgoto_[] =
{
  ]b4_pgoto[
};

/* YYDEFGOTO[NTERM-NUM].  */
const ]b4_int_type_for([b4_defgoto])[
yy::]b4_parser_class_name[::defgoto_[] =
{
  ]b4_defgoto[
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.  */
const ]b4_int_type(b4_table_ninf, b4_table_ninf) yy::b4_parser_class_name::table_ninf_ = b4_table_ninf[;
const ]b4_int_type_for([b4_table])[
yy::]b4_parser_class_name[::table_[] =
{
  ]b4_table[
};

/* YYCHECK.  */
const ]b4_int_type_for([b4_check])[
yy::]b4_parser_class_name[::check_[] =
{
  ]b4_check[
};

/* STOS_[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
const ]b4_int_type_for([b4_stos])[
yy::]b4_parser_class_name[::stos_[] =
{
  ]b4_stos[
};

#if YYDEBUG
/* TOKEN_NUMBER_[YYLEX-NUM] -- Internal token number corresponding
   to YYLEX-NUM.  */
const ]b4_int_type_for([b4_toknum])[
yy::]b4_parser_class_name[::token_number_[] =
{
  ]b4_toknum[
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
const ]b4_int_type_for([b4_r1])[
yy::]b4_parser_class_name[::r1_[] =
{
  ]b4_r1[
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
const ]b4_int_type_for([b4_r2])[
yy::]b4_parser_class_name[::r2_[] =
{
  ]b4_r2[
};

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
const char*
const yy::]b4_parser_class_name[::name_[] =
{
  ]b4_tname[
};
#endif

#if YYDEBUG
/* YYRHS -- A `-1'-separated list of the rules' RHS. */
const yy::]b4_parser_class_name[::RhsNumberType
yy::]b4_parser_class_name[::rhs_[] =
{
  ]b4_rhs[
};

/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
const ]b4_int_type_for([b4_prhs])[
yy::]b4_parser_class_name[::prhs_[] =
{
  ]b4_prhs[
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
const ]b4_int_type_for([b4_rline])[
yy::]b4_parser_class_name[::rline_[] =
{
  ]b4_rline[
};

/** Print the state stack from its BOTTOM up to its TOP (included).  */

void
yy::]b4_parser_class_name[::stack_print_ ()
{
  cdebug_ << "state stack now";
  for (StateStack::ConstIterator i = state_stack_.begin ();
       i != state_stack_.end (); ++i)
    cdebug_ << ' ' << *i;
  cdebug_ << std::endl;
}

/** Report that the YYRULE is going to be reduced.  */

void
yy::]b4_parser_class_name[::reduce_print_ (int yyrule)
{
  unsigned int yylno = rline_[yyrule];
  /* Print the symbols being reduced, and their result.  */
  cdebug_ << "Reducing via rule " << n_ - 1 << " (line " << yylno << "), ";
  for (]b4_int_type_for([b4_prhs])[ i = prhs_[n_];
       0 <= rhs_[i]; ++i)
    cdebug_ << name_[rhs_[i]] << ' ';
  cdebug_ << "-> " << name_[r1_[n_]] << std::endl;
}
#endif // YYDEBUG

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
yy::]b4_parser_class_name[::TokenNumberType
yy::]b4_parser_class_name[::translate_ (int token)
{
  static
  const TokenNumberType
  translate_table[] =
  {
    ]b4_translate[
  };
  if ((unsigned int) token <= user_token_number_max_)
    return translate_table[token];
  else
    return undef_token_;
}

const int yy::]b4_parser_class_name[::eof_ = 0;
const int yy::]b4_parser_class_name[::last_ = ]b4_last[;
const int yy::]b4_parser_class_name[::nnts_ = ]b4_nterms_number[;
const int yy::]b4_parser_class_name[::empty_ = -2;
const int yy::]b4_parser_class_name[::final_ = ]b4_final_state_number[;
const int yy::]b4_parser_class_name[::terror_ = 1;
const int yy::]b4_parser_class_name[::errcode_ = 256;
const int yy::]b4_parser_class_name[::ntokens_ = ]b4_tokens_number[;

const unsigned int yy::]b4_parser_class_name[::user_token_number_max_ = ]b4_user_token_number_max[;
const yy::]b4_parser_class_name[::TokenNumberType yy::]b4_parser_class_name[::undef_token_ = ]b4_undef_token_number[;

]b4_epilogue
dnl
@output stack.hh
b4_copyright([Stack handling for Bison C++ parsers], [2002, 2003, 2004])[

#ifndef BISON_STACK_HH
# define BISON_STACK_HH

#include <deque>

namespace yy
{
  template < class T, class S = std::deque< T > >
  class Stack
  {
  public:

    typedef typename S::iterator Iterator;
    typedef typename S::const_iterator ConstIterator;

    Stack () : seq_ ()
    {
    }

    Stack (unsigned int n) : seq_ (n)
    {
    }

    inline
    T&
    operator [] (unsigned int i)
    {
      return seq_[i];
    }

    inline
    const T&
    operator [] (unsigned int i) const
    {
      return seq_[i];
    }

    inline
    void
    push (const T& t)
    {
      seq_.push_front (t);
    }

    inline
    void
    pop (unsigned int n = 1)
    {
      for (; n; --n)
	seq_.pop_front ();
    }

    inline
    unsigned int
    height () const
    {
      return seq_.size ();
    }

    inline ConstIterator begin () const { return seq_.begin (); }
    inline ConstIterator end () const { return seq_.end (); }

  private:

    S seq_;
  };

  template < class T, class S = Stack< T > >
  class Slice
  {
  public:

    Slice (const S& stack,
	   unsigned int range) : stack_ (stack),
				 range_ (range)
    {
    }

    inline
    const T&
    operator [] (unsigned int i) const
    {
      return stack_[range_ - i];
    }

  private:

    const S& stack_;
    unsigned int range_;
  };
}

#endif // not BISON_STACK_HH]
dnl
@output position.hh
b4_copyright([Position class for Bison C++ parsers], [2002, 2003, 2004])[

/**
 ** \file position.hh
 ** Define the Location class.
 */

#ifndef BISON_POSITION_HH
# define BISON_POSITION_HH

# include <iostream>
# include <string>

namespace yy
{
  /** \brief Abstract a Position. */
  class Position
  {
  public:
    /** \brief Initial column number. */
    static const unsigned int initial_column = 0;
    /** \brief Initial line number. */
    static const unsigned int initial_line = 1;

    /** \name Ctor & dtor.
     ** \{ */
  public:
    /** \brief Construct a Position. */
    Position () :
      filename (),
      line (initial_line),
      column (initial_column)
    {
    }
    /** \} */


    /** \name Line and Column related manipulators
     ** \{ */
  public:
    /** \brief (line related) Advance to the COUNT next lines. */
    inline void lines (int count = 1)
    {
      column = initial_column;
      line += count;
    }

    /** \brief (column related) Advance to the COUNT next columns. */
    inline void columns (int count = 1)
    {
      int leftmost = initial_column;
      int current  = column;
      if (leftmost <= current + count)
	column += count;
      else
	column = initial_column;
    }
    /** \} */

  public:
    /** \brief File name to which this position refers. */
    std::string filename;
    /** \brief Current line number. */
    unsigned int line;
    /** \brief Current column number. */
    unsigned int column;
  };

  /** \brief Add and assign a Position. */
  inline const Position&
  operator+= (Position& res, const int width)
  {
    res.columns (width);
    return res;
  }

  /** \brief Add two Position objects. */
  inline const Position
  operator+ (const Position& begin, const int width)
  {
    Position res = begin;
    return res += width;
  }

  /** \brief Add and assign a Position. */
  inline const Position&
  operator-= (Position& res, const int width)
  {
    return res += -width;
  }

  /** \brief Add two Position objects. */
  inline const Position
  operator- (const Position& begin, const int width)
  {
    return begin + -width;
  }

  /** \brief Intercept output stream redirection.
   ** \param ostr the destination output stream
   ** \param pos a reference to the Position to redirect
   */
  inline std::ostream&
  operator<< (std::ostream& ostr, const Position& pos)
  {
    if (!pos.filename.empty ())
      ostr << pos.filename << ':';
    return ostr << pos.line << '.' << pos.column;
  }

}
#endif // not BISON_POSITION_HH]
@output location.hh
b4_copyright([Location class for Bison C++ parsers], [2002, 2003, 2004])[

/**
 ** \file location.hh
 ** Define the Location class.
 */

#ifndef BISON_LOCATION_HH
# define BISON_LOCATION_HH

# include <iostream>
# include <string>
# include "position.hh"

namespace yy
{

  /** \brief Abstract a Location. */
  class Location
  {
    /** \name Ctor & dtor.
     ** \{ */
  public:
    /** \brief Construct a Location. */
    Location (void) :
      begin (),
      end ()
    {
    }
    /** \} */


    /** \name Line and Column related manipulators
     ** \{ */
  public:
    /** \brief Reset initial location to final location. */
    inline void step (void)
    {
      begin = end;
    }

    /** \brief Extend the current location to the COUNT next columns. */
    inline void columns (unsigned int count = 1)
    {
      end += count;
    }

    /** \brief Extend the current location to the COUNT next lines. */
    inline void lines (unsigned int count = 1)
    {
      end.lines (count);
    }
    /** \} */


  public:
    /** \brief Beginning of the located region. */
    Position begin;
    /** \brief End of the located region. */
    Position end;
  };

  /** \brief Join two Location objects to create a Location. */
  inline const Location operator+ (const Location& begin, const Location& end)
  {
    Location res = begin;
    res.end = end.end;
    return res;
  }

  /** \brief Add two Location objects */
  inline const Location operator+ (const Location& begin, unsigned int width)
  {
    Location res = begin;
    res.columns (width);
    return res;
  }

  /** \brief Add and assign a Location */
  inline Location &operator+= (Location& res, unsigned int width)
  {
    res.columns (width);
    return res;
  }

  /** \brief Intercept output stream redirection.
   ** \param ostr the destination output stream
   ** \param loc a reference to the Location to redirect
   **
   ** Avoid duplicate information.
   */
  inline std::ostream& operator<< (std::ostream& ostr, const Location& loc)
  {
    Position last = loc.end - 1;
    ostr << loc.begin;
    if (loc.begin.filename != last.filename)
      ostr << '-' << last;
    else if (loc.begin.line != last.line)
      ostr << '-' << last.line  << '.' << last.column;
    else if (loc.begin.column != last.column)
      ostr << '-' << last.column;
    return ostr;
  }

}

#endif // not BISON_LOCATION_HH]
