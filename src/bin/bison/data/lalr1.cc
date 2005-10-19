m4_divert(-1)

# C++ skeleton for Bison

# Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA

m4_include(b4_pkgdatadir/[c++.m4])

# We do want M4 expansion after # for CPP macros.
m4_changecom()
m4_divert(0)dnl
m4_if(b4_defines_flag, 0, [],
[@output @output_header_name@
b4_copyright([C++ Skeleton parser for LALR(1) parsing with Bison],
             [2002, 2003, 2004, 2005])[
/* FIXME: This is wrong, we want computed header guards.
   I don't know why the macros are missing now. :( */
#ifndef PARSER_HEADER_H
# define PARSER_HEADER_H

#include <string>
#include <iostream>

/* Using locations.  */
#define YYLSP_NEEDED ]b4_locations_flag[

namespace yy
{
  class position;
  class location;
}

]b4_token_enums(b4_tokens)[

/* Copy the first part of user declarations.  */
]b4_pre_prologue[

]/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])[

#include "stack.hh"
#include "location.hh"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG ]b4_debug[
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE ]b4_error_verbose[
#endif

#if YYERROR_VERBOSE
# define YYERROR_VERBOSE_IF(x) x
#else
# define YYERROR_VERBOSE_IF(x) /* empty */
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE ]b4_token_table[
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
]m4_ifdef([b4_stype],
[b4_syncline([b4_stype_line], [b4_file_name])
union YYSTYPE b4_stype;
/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])],
[typedef int YYSTYPE;])[
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

/* Copy the second part of user declarations.  */
]b4_post_prologue[

]/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])[
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
do {							\
  if (N)						\
    {							\
      (Current).begin = (Rhs)[1].begin;			\
      (Current).end   = (Rhs)[N].end;			\
    }							\
  else							\
    {							\
      (Current).begin = (Current).end = (Rhs)[0].end;	\
    }							\
} while (0)
#endif

namespace yy
{
  class ]b4_parser_class_name[;

  template <typename P>
  struct traits
  {
  };

  template <>
  struct traits<]b4_parser_class_name[>
  {
    typedef ]b4_int_type_for([b4_translate])[ token_number_type;
    typedef ]b4_int_type_for([b4_rhs])[       rhs_number_type;
    typedef int state_type;
    typedef YYSTYPE semantic_type;
    typedef ]b4_location_type[ location_type;
  };
}

namespace yy
{
  /// A Bison parser.
  class ]b4_parser_class_name[
  {
    /// Symbol semantic values.
    typedef traits<]b4_parser_class_name[>::semantic_type semantic_type;
    /// Symbol locations.
    typedef traits<]b4_parser_class_name[>::location_type location_type;

  public:
    /// Build a parser object.
    ]b4_parser_class_name[ (]b4_parse_param_decl[) :
      yydebug_ (false),
      yycdebug_ (&std::cerr)]b4_parse_param_cons[
    {
    }

    virtual ~]b4_parser_class_name[ ()
    {
    }

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

    /// The current debugging stream.
    std::ostream& debug_stream () const;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);

  private:
    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Generate an error message.
    /// \param tok    the look-ahead token.
    virtual std::string yysyntax_error_ (YYERROR_VERBOSE_IF (int tok));

#if YYDEBUG
    /// \brief Report a symbol on the debug stream.
    /// \param yytype       The token type.
    /// \param yyvaluep     Its semantic value.
    /// \param yylocationp  Its location.
    virtual void yysymprint_ (int yytype,
			      const semantic_type* yyvaluep,
			      const location_type* yylocationp);
#endif /* ! YYDEBUG */


    /// State numbers.
    typedef traits<]b4_parser_class_name[>::state_type state_type;
    /// State stack type.
    typedef stack<state_type>    state_stack_type;
    /// Semantic value stack type.
    typedef stack<semantic_type> semantic_stack_type;
    /// location stack type.
    typedef stack<location_type> location_stack_type;

    /// The state stack.
    state_stack_type yystate_stack_;
    /// The semantic value stack.
    semantic_stack_type yysemantic_stack_;
    /// The location stack.
    location_stack_type yylocation_stack_;

    /// Internal symbol numbers.
    typedef traits<]b4_parser_class_name[>::token_number_type token_number_type;
    /* Tables.  */
    /// For a state, the index in \a yytable_ of its portion.
    static const ]b4_int_type_for([b4_pact])[ yypact_[];
    static const ]b4_int_type(b4_pact_ninf, b4_pact_ninf)[ yypact_ninf_;

    /// For a state, default rule to reduce.
    /// Unless\a  yytable_ specifies something else to do.
    /// Zero means the default is an error.
    static const ]b4_int_type_for([b4_defact])[ yydefact_[];

    static const ]b4_int_type_for([b4_pgoto])[ yypgoto_[];
    static const ]b4_int_type_for([b4_defgoto])[ yydefgoto_[];

    /// What to do in a state.
    /// \a yytable_[yypact_[s]]: what to do in state \a s.
    /// - if positive, shift that token.
    /// - if negative, reduce the rule which number is the opposite.
    /// - if zero, do what YYDEFACT says.
    static const ]b4_int_type_for([b4_table])[ yytable_[];
    static const ]b4_int_type(b4_table_ninf, b4_table_ninf)[ yytable_ninf_;

    static const ]b4_int_type_for([b4_check])[ yycheck_[];

    /// For a state, its accessing symbol.
    static const ]b4_int_type_for([b4_stos])[ yystos_[];

    /// For a rule, its LHS.
    static const ]b4_int_type_for([b4_r1])[ yyr1_[];
    /// For a rule, its RHS length.
    static const ]b4_int_type_for([b4_r2])[ yyr2_[];

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#endif

#if YYERROR_VERBOSE
    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    virtual std::string yytnamerr_ (const char *n);
#endif

#if YYDEBUG
    /// A type to store symbol numbers and -1.
    typedef traits<]b4_parser_class_name[>::rhs_number_type rhs_number_type;
    /// A `-1'-separated list of the rules' RHS.
    static const rhs_number_type yyrhs_[];
    /// For each rule, the index of the first RHS symbol in \a yyrhs_.
    static const ]b4_int_type_for([b4_prhs])[ yyprhs_[];
    /// For each rule, its source line number.
    static const ]b4_int_type_for([b4_rline])[ yyrline_[];
    /// For each scanner token number, its symbol number.
    static const ]b4_int_type_for([b4_toknum])[ yytoken_number_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yyreduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();
#endif

    /// Convert a scanner token number to a symbol number.
    inline token_number_type yytranslate_ (int token);

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg        Why this token is reclaimed.
    /// \param yytype       The symbol type.
    /// \param yyvaluep     Its semantic value.
    /// \param yylocationp  Its location.
    inline void yydestruct_ (const char* yymsg,
                             int yytype,
                             semantic_type* yyvaluep,
                             location_type* yylocationp);

    /// Pop \a n symbols the three stacks.
    inline void yypop_ (unsigned int n = 1);

    /* Constants.  */
    static const int yyeof_;
    /* LAST_ -- Last index in TABLE_.  */
    static const int yylast_;
    static const int yynnts_;
    static const int yyempty_;
    static const int yyfinal_;
    static const int yyterror_;
    static const int yyerrcode_;
    static const int yyntokens_;
    static const unsigned int yyuser_token_number_max_;
    static const token_number_type yyundef_token_;

    /* State.  */
    int yyn_;
    int yylen_;
    int yystate_;

    /* Error handling. */
    int yynerrs_;
    int yyerrstatus_;

    /* Debugging.  */
    int yydebug_;
    std::ostream* yycdebug_;

]b4_parse_param_vars[
  };
}

#endif /* ! defined PARSER_HEADER_H */]
])dnl
@output @output_parser_name@
b4_copyright([C++ Skeleton parser for LALR(1) parsing with Bison],
             [2002, 2003, 2004, 2005])
m4_if(b4_prefix[], [yy], [],
[
// Take the name prefix into account.
#define yylex   b4_prefix[]lex])
m4_if(b4_defines_flag, 0, [],
[
#include @output_header_name@])[

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* FIXME: INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* A pseudo ostream that takes yydebug_ into account. */
# define YYCDEBUG							\
  for (bool yydebugcond_ = yydebug_; yydebugcond_; yydebugcond_ = false)	\
    (*yycdebug_)

/* Enable debugging if requested.  */
#if YYDEBUG

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)	\
do {							\
  if (yydebug_)						\
    {							\
      *yycdebug_ << (Title) << ' ';			\
      yysymprint_ ((Type), (Value), (Location));	\
      *yycdebug_ << std::endl;				\
    }							\
} while (0)

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug_)				\
    yyreduce_print_ (Rule);		\
} while (0)

# define YY_STACK_PRINT()		\
do {					\
  if (yydebug_)				\
    yystack_print_ ();			\
} while (0)

#else /* !YYDEBUG */

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_REDUCE_PRINT(Rule)
# define YY_STACK_PRINT()

#endif /* !YYDEBUG */

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab

#if YYERROR_VERBOSE

/* Return YYSTR after stripping away unnecessary quotes and
   backslashes, so that it's suitable for yyerror.  The heuristic is
   that double-quoting is unnecessary unless the string contains an
   apostrophe, a comma, or backslash (other than backslash-backslash).
   YYSTR is taken from yytname.  */
std::string
yy::]b4_parser_class_name[::yytnamerr_ (const char *yystr)
{
  if (*yystr == '"')
    {
      std::string yyr = "";
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
	    yyr += *yyp;
	    break;

	  case '"':
	    return yyr;
	  }
    do_not_strip_quotes: ;
    }

  return yystr;
}

#endif

#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

void
yy::]b4_parser_class_name[::yysymprint_ (int yytype,
                         const semantic_type* yyvaluep, const location_type* yylocationp)
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
  (void) yylocationp;
  /* Backward compatibility, but should be removed eventually. */
  std::ostream& cdebug_ = *yycdebug_;
  (void) cdebug_;

  *yycdebug_ << (yytype < yyntokens_ ? "token" : "nterm")
	     << ' ' << yytname_[yytype] << " ("
             << *yylocationp << ": ";
  switch (yytype)
    {
]m4_map([b4_symbol_actions], m4_defn([b4_symbol_printers]))dnl
[      default:
        break;
    }
  *yycdebug_ << ')';
}
#endif /* ! YYDEBUG */

void
yy::]b4_parser_class_name[::yydestruct_ (const char* yymsg,
                         int yytype, semantic_type* yyvaluep, location_type* yylocationp)
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yymsg;
  (void) yyvaluep;
  (void) yylocationp;

  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
]m4_map([b4_symbol_actions], m4_defn([b4_symbol_destructors]))[
      default:
        break;
    }
}

void
yy::]b4_parser_class_name[::yypop_ (unsigned int n)
{
  yystate_stack_.pop (n);
  yysemantic_stack_.pop (n);
  yylocation_stack_.pop (n);
}

std::ostream&
yy::]b4_parser_class_name[::debug_stream () const
{
  return *yycdebug_;
}

void
yy::]b4_parser_class_name[::set_debug_stream (std::ostream& o)
{
  yycdebug_ = &o;
}


yy::]b4_parser_class_name[::debug_level_type
yy::]b4_parser_class_name[::debug_level () const
{
  return yydebug_;
}

void
yy::]b4_parser_class_name[::set_debug_level (debug_level_type l)
{
  yydebug_ = l;
}


int
yy::]b4_parser_class_name[::parse ()
{
  /* Look-ahead and look-ahead in internal form.  */
  int yylooka;
  int yyilooka;

  /// Semantic value of the look-ahead.
  semantic_type yylval;
  /// Location of the look-ahead.
  location_type yylloc;
  /// The locations where the error started and ended.
  location yyerror_range[2];

  /// $$.
  semantic_type yyval;
  /// @@$.
  location_type yyloc;

  int yyresult_;

  YYCDEBUG << "Starting parse" << std::endl;

  yynerrs_ = 0;
  yyerrstatus_ = 0;

  /* Start.  */
  yystate_ = 0;
  yylooka = yyempty_;

]m4_ifdef([b4_initial_action], [
m4_pushdef([b4_at_dollar],     [yylloc])dnl
m4_pushdef([b4_dollar_dollar], [yylval])dnl
  /* User initialization code. */
  b4_initial_action
m4_popdef([b4_dollar_dollar])dnl
m4_popdef([b4_at_dollar])dnl
/* Line __line__ of yacc.c.  */
b4_syncline([@oline@], [@ofile@])])dnl

[  /* Initialize the stacks.  The initial state will be pushed in
     yynewstate, since the latter expects the semantical and the
     location values to have been already stored, initialize these
     stacks with a primary value.  */
  yystate_stack_ = state_stack_type (0);
  yysemantic_stack_ = semantic_stack_type (0);
  yylocation_stack_ = location_stack_type (0);
  yysemantic_stack_.push (yylval);
  yylocation_stack_.push (yylloc);

  /* New state.  */
yynewstate:
  yystate_stack_.push (yystate_);
  YYCDEBUG << "Entering state " << yystate_ << std::endl;
  goto yybackup;

  /* Backup.  */
yybackup:

  /* Try to take a decision without look-ahead.  */
  yyn_ = yypact_[yystate_];
  if (yyn_ == yypact_ninf_)
    goto yydefault;

  /* Read a look-ahead token.  */
  if (yylooka == yyempty_)
    {
      YYCDEBUG << "Reading a token: ";
      yylooka = ]b4_c_function_call([yylex], [int],
[[YYSTYPE*], [&yylval]][]dnl
b4_location_if([, [[location*], [&yylloc]]])dnl
m4_ifdef([b4_lex_param], [, ]b4_lex_param))[;
    }


  /* Convert token to internal form.  */
  if (yylooka <= yyeof_)
    {
      yylooka = yyilooka = yyeof_;
      YYCDEBUG << "Now at end of input." << std::endl;
    }
  else
    {
      yyilooka = yytranslate_ (yylooka);
      YY_SYMBOL_PRINT ("Next token is", yyilooka, &yylval, &yylloc);
    }

  /* If the proper action on seeing token ILOOKA_ is to reduce or to
     detect an error, take that action.  */
  yyn_ += yyilooka;
  if (yyn_ < 0 || yylast_ < yyn_ || yycheck_[yyn_] != yyilooka)
    goto yydefault;

  /* Reduce or error.  */
  yyn_ = yytable_[yyn_];
  if (yyn_ < 0)
    {
      if (yyn_ == yytable_ninf_)
	goto yyerrlab;
      else
	{
	  yyn_ = -yyn_;
	  goto yyreduce;
	}
    }
  else if (yyn_ == 0)
    goto yyerrlab;

  /* Accept?  */
  if (yyn_ == yyfinal_)
    goto yyacceptlab;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yyilooka, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yylooka != yyeof_)
    yylooka = yyempty_;

  yysemantic_stack_.push (yylval);
  yylocation_stack_.push (yylloc);

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus_)
    --yyerrstatus_;

  yystate_ = yyn_;
  goto yynewstate;

/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn_ = yydefact_[yystate_];
  if (yyn_ == 0)
    goto yyerrlab;
  goto yyreduce;

/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  yylen_ = yyr2_[yyn_];
  /* If LEN_ is nonzero, implement the default value of the action:
     `$$ = $1'.  Otherwise, use the top of the stack.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  */
  if (yylen_)
    yyval = yysemantic_stack_[yylen_ - 1];
  else
    yyval = yysemantic_stack_[0];

  {
    slice<location_type, location_stack_type> slice (yylocation_stack_, yylen_);
    YYLLOC_DEFAULT (yyloc, slice, yylen_);
  }
  YY_REDUCE_PRINT (yyn_);
  switch (yyn_)
    {
      ]b4_actions[
      default: break;
    }

]/* Line __line__ of lalr1.cc.  */
b4_syncline([@oline@], [@ofile@])[

  yypop_ (yylen_);

  YY_STACK_PRINT ();

  yysemantic_stack_.push (yyval);
  yylocation_stack_.push (yyloc);

  /* Shift the result of the reduction.  */
  yyn_ = yyr1_[yyn_];
  yystate_ = yypgoto_[yyn_ - yyntokens_] + yystate_stack_[0];
  if (0 <= yystate_ && yystate_ <= yylast_
      && yycheck_[yystate_] == yystate_stack_[0])
    yystate_ = yytable_[yystate_];
  else
    yystate_ = yydefgoto_[yyn_ - yyntokens_];
  goto yynewstate;

/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus_)
    {
      ++yynerrs_;
      error (yylloc, yysyntax_error_ (YYERROR_VERBOSE_IF (yyilooka)));
    }

  yyerror_range[0] = yylloc;
  if (yyerrstatus_ == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yylooka <= yyeof_)
        {
	  /* Return failure if at end of input.  */
	  if (yylooka == yyeof_)
	    YYABORT;
        }
      else
        {
          yydestruct_ ("Error: discarding", yyilooka, &yylval, &yylloc);
          yylooka = yyempty_;
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
  if (false)
    goto yyerrorlab;

  yyerror_range[0] = yylocation_stack_[yylen_ - 1];
  yypop_ (yylen_);
  yystate_ = yystate_stack_[0];
  goto yyerrlab1;

/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus_ = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn_ = yypact_[yystate_];
      if (yyn_ != yypact_ninf_)
	{
	  yyn_ += yyterror_;
	  if (0 <= yyn_ && yyn_ <= yylast_ && yycheck_[yyn_] == yyterror_)
	    {
	      yyn_ = yytable_[yyn_];
	      if (0 < yyn_)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yystate_stack_.height () == 1)
	YYABORT;

      yyerror_range[0] = yylocation_stack_[0];
      yydestruct_ ("Error: popping",
                   yystos_[yystate_],
                   &yysemantic_stack_[0], &yylocation_stack_[0]);
      yypop_ ();
      yystate_ = yystate_stack_[0];
      YY_STACK_PRINT ();
    }

  if (yyn_ == yyfinal_)
    goto yyacceptlab;

  yyerror_range[1] = yylloc;
  // Using YYLLOC is tempting, but would change the location of
  // the look-ahead.  YYLOC is available though.
  YYLLOC_DEFAULT (yyloc, yyerror_range - 1, 2);
  yysemantic_stack_.push (yylval);
  yylocation_stack_.push (yyloc);

  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos_[yyn_],
		   &yysemantic_stack_[0], &yylocation_stack_[0]);

  yystate_ = yyn_;
  goto yynewstate;

  /* Accept.  */
yyacceptlab:
  yyresult_ = 0;
  goto yyreturn;

  /* Abort.  */
yyabortlab:
  yyresult_ = 1;
  goto yyreturn;

yyreturn:
  if (yylooka != yyeof_ && yylooka != yyempty_)
    yydestruct_ ("Cleanup: discarding lookahead", yyilooka, &yylval, &yylloc);

  while (yystate_stack_.height () != 1)
    {
      yydestruct_ ("Cleanup: popping",
		   yystos_[yystate_stack_[0]],
		   &yysemantic_stack_[0],
		   &yylocation_stack_[0]);
      yypop_ ();
    }

  return yyresult_;
}

// Generate an error message.
std::string
yy::]b4_parser_class_name[::yysyntax_error_ (YYERROR_VERBOSE_IF (int tok))
{
  std::string res;
#if YYERROR_VERBOSE
  yyn_ = yypact_[yystate_];
  if (yypact_ninf_ < yyn_ && yyn_ < yylast_)
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  */
      int yyxbegin = yyn_ < 0 ? -yyn_ : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = yylast_ - yyn_;
      int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
      int count = 0;
      for (int x = yyxbegin; x < yyxend; ++x)
        if (yycheck_[x + yyn_] == x && x != yyterror_)
          ++count;

      // FIXME: This method of building the message is not compatible
      // with internationalization.  It should work like yacc.c does it.
      // That is, first build a string that looks like this:
      // "syntax error, unexpected %s or %s or %s"
      // Then, invoke YY_ on this string.
      // Finally, use the string as a format to output
      // yytname_[tok], etc.
      // Until this gets fixed, this message appears in English only.
      res = "syntax error, unexpected ";
      res += yytnamerr_ (yytname_[tok]);
      if (count < 5)
        {
          count = 0;
          for (int x = yyxbegin; x < yyxend; ++x)
            if (yycheck_[x + yyn_] == x && x != yyterror_)
              {
                res += (!count++) ? ", expecting " : " or ";
                res += yytnamerr_ (yytname_[x]);
              }
        }
    }
  else
#endif
    res = YY_("syntax error");
  return res;
}


/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
const ]b4_int_type(b4_pact_ninf, b4_pact_ninf) yy::b4_parser_class_name::yypact_ninf_ = b4_pact_ninf[;
const ]b4_int_type_for([b4_pact])[
yy::]b4_parser_class_name[::yypact_[] =
{
  ]b4_pact[
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error.  */
const ]b4_int_type_for([b4_defact])[
yy::]b4_parser_class_name[::yydefact_[] =
{
  ]b4_defact[
};

/* YYPGOTO[NTERM-NUM].  */
const ]b4_int_type_for([b4_pgoto])[
yy::]b4_parser_class_name[::yypgoto_[] =
{
  ]b4_pgoto[
};

/* YYDEFGOTO[NTERM-NUM].  */
const ]b4_int_type_for([b4_defgoto])[
yy::]b4_parser_class_name[::yydefgoto_[] =
{
  ]b4_defgoto[
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.  */
const ]b4_int_type(b4_table_ninf, b4_table_ninf) yy::b4_parser_class_name::yytable_ninf_ = b4_table_ninf[;
const ]b4_int_type_for([b4_table])[
yy::]b4_parser_class_name[::yytable_[] =
{
  ]b4_table[
};

/* YYCHECK.  */
const ]b4_int_type_for([b4_check])[
yy::]b4_parser_class_name[::yycheck_[] =
{
  ]b4_check[
};

/* STOS_[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
const ]b4_int_type_for([b4_stos])[
yy::]b4_parser_class_name[::yystos_[] =
{
  ]b4_stos[
};

#if YYDEBUG
/* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
   to YYLEX-NUM.  */
const ]b4_int_type_for([b4_toknum])[
yy::]b4_parser_class_name[::yytoken_number_[] =
{
  ]b4_toknum[
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
const ]b4_int_type_for([b4_r1])[
yy::]b4_parser_class_name[::yyr1_[] =
{
  ]b4_r1[
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
const ]b4_int_type_for([b4_r2])[
yy::]b4_parser_class_name[::yyr2_[] =
{
  ]b4_r2[
};

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at \a yyntokens_, nonterminals. */
const char*
const yy::]b4_parser_class_name[::yytname_[] =
{
  ]b4_tname[
};
#endif

#if YYDEBUG
/* YYRHS -- A `-1'-separated list of the rules' RHS. */
const yy::]b4_parser_class_name[::rhs_number_type
yy::]b4_parser_class_name[::yyrhs_[] =
{
  ]b4_rhs[
};

/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
const ]b4_int_type_for([b4_prhs])[
yy::]b4_parser_class_name[::yyprhs_[] =
{
  ]b4_prhs[
};

/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
const ]b4_int_type_for([b4_rline])[
yy::]b4_parser_class_name[::yyrline_[] =
{
  ]b4_rline[
};

// Print the state stack on the debug stream.
void
yy::]b4_parser_class_name[::yystack_print_ ()
{
  *yycdebug_ << "Stack now";
  for (state_stack_type::const_iterator i = yystate_stack_.begin ();
       i != yystate_stack_.end (); ++i)
    *yycdebug_ << ' ' << *i;
  *yycdebug_ << std::endl;
}

// Report on the debug stream that the rule \a yyrule is going to be reduced.
void
yy::]b4_parser_class_name[::yyreduce_print_ (int yyrule)
{
  unsigned int yylno = yyrline_[yyrule];
  /* Print the symbols being reduced, and their result.  */
  *yycdebug_ << "Reducing stack by rule " << yyn_ - 1
             << " (line " << yylno << "), ";
  for (]b4_int_type_for([b4_prhs])[ i = yyprhs_[yyn_];
       0 <= yyrhs_[i]; ++i)
    *yycdebug_ << yytname_[yyrhs_[i]] << ' ';
  *yycdebug_ << "-> " << yytname_[yyr1_[yyn_]] << std::endl;
}
#endif // YYDEBUG

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
yy::]b4_parser_class_name[::token_number_type
yy::]b4_parser_class_name[::yytranslate_ (int token)
{
  static
  const token_number_type
  translate_table[] =
  {
    ]b4_translate[
  };
  if ((unsigned int) token <= yyuser_token_number_max_)
    return translate_table[token];
  else
    return yyundef_token_;
}

const int yy::]b4_parser_class_name[::yyeof_ = 0;
const int yy::]b4_parser_class_name[::yylast_ = ]b4_last[;
const int yy::]b4_parser_class_name[::yynnts_ = ]b4_nterms_number[;
const int yy::]b4_parser_class_name[::yyempty_ = -2;
const int yy::]b4_parser_class_name[::yyfinal_ = ]b4_final_state_number[;
const int yy::]b4_parser_class_name[::yyterror_ = 1;
const int yy::]b4_parser_class_name[::yyerrcode_ = 256;
const int yy::]b4_parser_class_name[::yyntokens_ = ]b4_tokens_number[;

const unsigned int yy::]b4_parser_class_name[::yyuser_token_number_max_ = ]b4_user_token_number_max[;
const yy::]b4_parser_class_name[::token_number_type yy::]b4_parser_class_name[::yyundef_token_ = ]b4_undef_token_number[;

]b4_epilogue
dnl
@output stack.hh
b4_copyright([stack handling for Bison C++ parsers], [2002, 2003, 2004, 2005])[

#ifndef BISON_STACK_HH
# define BISON_STACK_HH

#include <deque>

namespace yy
{
  template <class T, class S = std::deque<T> >
  class stack
  {
  public:

    // Hide our reversed order.
    typedef typename S::reverse_iterator iterator;
    typedef typename S::const_reverse_iterator const_iterator;

    stack () : seq_ ()
    {
    }

    stack (unsigned int n) : seq_ (n)
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

    inline const_iterator begin () const { return seq_.rbegin (); }
    inline const_iterator end () const { return seq_.rend (); }

  private:

    S seq_;
  };

  /// Present a slice of the top of a stack.
  template <class T, class S = stack<T> >
  class slice
  {
  public:

    slice (const S& stack,
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
b4_copyright([Position class for Bison C++ parsers], [2002, 2003, 2004, 2005])[

/**
 ** \file position.hh
 ** Define the position class.
 */

#ifndef BISON_POSITION_HH
# define BISON_POSITION_HH

# include <iostream>
# include <string>

namespace yy
{
  /// Abstract a position.
  class position
  {
  public:
    /// Initial column number.
    static const unsigned int initial_column = 0;
    /// Initial line number.
    static const unsigned int initial_line = 1;

    /** \name Ctor & dtor.
     ** \{ */
  public:
    /// Construct a position.
    position () :
      filename (0),
      line (initial_line),
      column (initial_column)
    {
    }
    /** \} */


    /** \name Line and Column related manipulators
     ** \{ */
  public:
    /// (line related) Advance to the COUNT next lines.
    inline void lines (int count = 1)
    {
      column = initial_column;
      line += count;
    }

    /// (column related) Advance to the COUNT next columns.
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
    /// File name to which this position refers.
    ]b4_filename_type[* filename;
    /// Current line number.
    unsigned int line;
    /// Current column number.
    unsigned int column;
  };

  /// Add and assign a position.
  inline const position&
  operator+= (position& res, const int width)
  {
    res.columns (width);
    return res;
  }

  /// Add two position objects.
  inline const position
  operator+ (const position& begin, const int width)
  {
    position res = begin;
    return res += width;
  }

  /// Add and assign a position.
  inline const position&
  operator-= (position& res, const int width)
  {
    return res += -width;
  }

  /// Add two position objects.
  inline const position
  operator- (const position& begin, const int width)
  {
    return begin + -width;
  }

  /** \brief Intercept output stream redirection.
   ** \param ostr the destination output stream
   ** \param pos a reference to the position to redirect
   */
  inline std::ostream&
  operator<< (std::ostream& ostr, const position& pos)
  {
    if (pos.filename)
      ostr << *pos.filename << ':';
    return ostr << pos.line << '.' << pos.column;
  }

}
#endif // not BISON_POSITION_HH]
@output location.hh
b4_copyright([Location class for Bison C++ parsers], [2002, 2003, 2004, 2005])[

/**
 ** \file location.hh
 ** Define the location class.
 */

#ifndef BISON_LOCATION_HH
# define BISON_LOCATION_HH

# include <iostream>
# include <string>
# include "position.hh"

namespace yy
{

  /// Abstract a location.
  class location
  {
    /** \name Ctor & dtor.
     ** \{ */
  public:
    /// Construct a location.
    location () :
      begin (),
      end ()
    {
    }
    /** \} */


    /** \name Line and Column related manipulators
     ** \{ */
  public:
    /// Reset initial location to final location.
    inline void step ()
    {
      begin = end;
    }

    /// Extend the current location to the COUNT next columns.
    inline void columns (unsigned int count = 1)
    {
      end += count;
    }

    /// Extend the current location to the COUNT next lines.
    inline void lines (unsigned int count = 1)
    {
      end.lines (count);
    }
    /** \} */


  public:
    /// Beginning of the located region.
    position begin;
    /// End of the located region.
    position end;
  };

  /// Join two location objects to create a location.
  inline const location operator+ (const location& begin, const location& end)
  {
    location res = begin;
    res.end = end.end;
    return res;
  }

  /// Add two location objects.
  inline const location operator+ (const location& begin, unsigned int width)
  {
    location res = begin;
    res.columns (width);
    return res;
  }

  /// Add and assign a location.
  inline location& operator+= (location& res, unsigned int width)
  {
    res.columns (width);
    return res;
  }

  /** \brief Intercept output stream redirection.
   ** \param ostr the destination output stream
   ** \param loc a reference to the location to redirect
   **
   ** Avoid duplicate information.
   */
  inline std::ostream& operator<< (std::ostream& ostr, const location& loc)
  {
    position last = loc.end - 1;
    ostr << loc.begin;
    if (last.filename
	&& (!loc.begin.filename
	    || *loc.begin.filename != *last.filename))
      ostr << '-' << last;
    else if (loc.begin.line != last.line)
      ostr << '-' << last.line  << '.' << last.column;
    else if (loc.begin.column != last.column)
      ostr << '-' << last.column;
    return ostr;
  }

}

#endif // not BISON_LOCATION_HH]
