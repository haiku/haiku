/*
 * awkgram.y --- yacc/bison parser
 */

/* 
 * Copyright (C) 1986, 1988, 1989, 1991-2003 the Free Software Foundation, Inc.
 * 
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 * 
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

%{
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
%}

%union {
	long lval;
	AWKNUM fval;
	NODE *nodeval;
	NODETYPE nodetypeval;
	char *sval;
	NODE *(*ptrval) P((void));
}

%type <nodeval> function_prologue pattern action variable param_list
%type <nodeval> exp common_exp
%type <nodeval> simp_exp non_post_simp_exp
%type <nodeval> expression_list opt_expression_list print_expression_list
%type <nodeval> statements statement if_statement switch_body case_statements case_statement case_value opt_param_list 
%type <nodeval> simple_stmt opt_simple_stmt
%type <nodeval> opt_exp opt_variable regexp 
%type <nodeval> input_redir output_redir
%type <nodetypeval> print
%type <nodetypeval> assign_operator a_relop relop_or_less
%type <sval> func_name
%type <lval> lex_builtin

%token <sval> FUNC_CALL NAME REGEXP
%token <lval> ERROR
%token <nodeval> YNUMBER YSTRING
%token <nodetypeval> RELOP IO_OUT IO_IN
%token <nodetypeval> ASSIGNOP ASSIGN MATCHOP CONCAT_OP
%token <nodetypeval> LEX_BEGIN LEX_END LEX_IF LEX_ELSE LEX_RETURN LEX_DELETE
%token <nodetypeval> LEX_SWITCH LEX_CASE LEX_DEFAULT LEX_WHILE LEX_DO LEX_FOR LEX_BREAK LEX_CONTINUE
%token <nodetypeval> LEX_PRINT LEX_PRINTF LEX_NEXT LEX_EXIT LEX_FUNCTION
%token <nodetypeval> LEX_GETLINE LEX_NEXTFILE
%token <nodetypeval> LEX_IN
%token <lval> LEX_AND LEX_OR INCREMENT DECREMENT
%token <lval> LEX_BUILTIN LEX_LENGTH
%token NEWLINE

/* these are just yylval numbers */

/* Lowest to highest */
%right ASSIGNOP ASSIGN SLASH_BEFORE_EQUAL
%right '?' ':'
%left LEX_OR
%left LEX_AND
%left LEX_GETLINE
%nonassoc LEX_IN
%left FUNC_CALL LEX_BUILTIN LEX_LENGTH
%nonassoc ','
%nonassoc MATCHOP
%nonassoc RELOP '<' '>' IO_IN IO_OUT
%left CONCAT_OP
%left YSTRING YNUMBER
%left '+' '-'
%left '*' '/' '%'
%right '!' UNARY
%right '^'
%left INCREMENT DECREMENT
%left '$'
%left '(' ')'
%%

start
	: opt_nls program opt_nls
		{
			check_funcs();
		}
	;

program
	: /* empty */
	| program rule
	  {
		begin_or_end_rule = parsing_end_rule = FALSE;
		yyerrok;
	  }
	| program error
  	  {
		begin_or_end_rule = parsing_end_rule = FALSE;
		/*
		 * If errors, give up, don't produce an infinite
		 * stream of syntax error messages.
		 */
  		/* yyerrok; */
  	  }
	;

rule
	: pattern action
	  {
		$1->rnode = $2;
	  }
	| pattern statement_term
	  {
		if ($1->lnode != NULL) {
			/* pattern rule with non-empty pattern */
			$1->rnode = node(NULL, Node_K_print_rec, NULL);
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
	| function_prologue action
	  {
		can_return = FALSE;
		if ($1)
			func_install($1, $2);
		yyerrok;
	  }
	;

pattern
	: /* empty */
	  {
		$$ = append_pattern(&expression_value, (NODE *) NULL);
	  }
	| exp
	  {
		$$ = append_pattern(&expression_value, $1);
	  }
	| exp ',' exp
	  {
		NODE *r;

		getnode(r);
		r->type = Node_line_range;
		r->condpair = node($1, Node_cond_pair, $3);
		r->triggered = FALSE;
		$$ = append_pattern(&expression_value, r);
	  }
	| LEX_BEGIN
	  {
		begin_or_end_rule = TRUE;
		$$ = append_pattern(&begin_block, (NODE *) NULL);
	  }
	| LEX_END
	  {
		begin_or_end_rule = parsing_end_rule = TRUE;
		$$ = append_pattern(&end_block, (NODE *) NULL);
	  }
	;

action
	: l_brace statements r_brace opt_semi opt_nls
		{ $$ = $2; }
	;

func_name
	: NAME
		{ $$ = $1; }
	| FUNC_CALL
		{ $$ = $1; }
	| lex_builtin
	  {
		yyerror(_("`%s' is a built-in function, it cannot be redefined"),
			tokstart);
		errcount++;
		$$ = builtin_func;
		/* yyerrok; */
	  }
	;

lex_builtin
	: LEX_BUILTIN
	| LEX_LENGTH
	;
		
function_prologue
	: LEX_FUNCTION 
		{
			param_counter = 0;
		}
	  func_name '(' opt_param_list r_paren opt_nls
		{
			NODE *t;

			t = make_param($3);
			t->flags |= FUNC;
			$$ = append_right(t, $5);
			can_return = TRUE;
			/* check for duplicate parameter names */
			if (dup_parms($$))
				errcount++;
		}
	;

regexp
	/*
	 * In this rule, want_regexp tells yylex that the next thing
	 * is a regexp so it should read up to the closing slash.
	 */
	: a_slash
		{ ++want_regexp; }
	  REGEXP	/* The terminating '/' is consumed by yylex(). */
		{
		  NODE *n;
		  size_t len = strlen($3);

		  if (do_lint && ($3)[0] == '*') {
			/* possible C comment */
			if (($3)[len-1] == '*')
				lintwarn(_("regexp constant `/%s/' looks like a C comment, but is not"), tokstart);
		  }
		  getnode(n);
		  n->type = Node_regex;
		  n->re_exp = make_string($3, len);
		  n->re_reg = make_regexp($3, len, FALSE);
		  n->re_text = NULL;
		  n->re_flags = CONST;
		  $$ = n;
		}
	;

a_slash
	: '/'
	| SLASH_BEFORE_EQUAL
	;

statements
	: /* empty */
	  { $$ = NULL; }
	| statements statement
	  {
		if ($2 == NULL)
			$$ = $1;
		else {
			if (do_lint && isnoeffect($2->type))
				lintwarn(_("statement may have no effect"));
			if ($1 == NULL)
				$$ = $2;
			else
	    			$$ = append_right(
					($1->type == Node_statement_list ? $1
					  : node($1, Node_statement_list, (NODE *) NULL)),
					($2->type == Node_statement_list ? $2
					  : node($2, Node_statement_list, (NODE *) NULL)));
		}
	    	yyerrok;
	  }
	| statements error
	  { $$ = NULL; }
	;

statement_term
	: nls
	| semi opt_nls
	;

statement
	: semi opt_nls
		{ $$ = NULL; }
	| l_brace statements r_brace
		{ $$ = $2; }
	| if_statement
		{ $$ = $1; }
	| LEX_SWITCH '(' exp r_paren opt_nls l_brace switch_body opt_nls r_brace
		{ $$ = node($3, Node_K_switch, $7); }
	| LEX_WHILE '(' exp r_paren opt_nls statement
		{ $$ = node($3, Node_K_while, $6); }
	| LEX_DO opt_nls statement LEX_WHILE '(' exp r_paren opt_nls
		{ $$ = node($6, Node_K_do, $3); }
	| LEX_FOR '(' NAME LEX_IN NAME r_paren opt_nls statement
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
		if ($8 != NULL && $8->type == Node_K_delete) {
			NODE *arr, *sub;

			assert($8->rnode->type == Node_expression_list);
			arr = $8->lnode;	/* array var */
			sub = $8->rnode->lnode;	/* index var */

			if (   (arr->type == Node_var_new
				|| arr->type == Node_var_array
				|| arr->type == Node_param_list)
			    && (sub->type == Node_var_new
				|| sub->type == Node_var
				|| sub->type == Node_param_list)
			    && strcmp($3, sub->vname) == 0
			    && strcmp($5, arr->vname) == 0) {
				$8->type = Node_K_delete_loop;
				$$ = $8;
			}
			else
				goto regular_loop;
		} else {
	regular_loop:
			$$ = node($8, Node_K_arrayfor,
				make_for_loop(variable($3, CAN_FREE, Node_var),
				(NODE *) NULL, variable($5, CAN_FREE, Node_var_array)));
		}
	  }
	| LEX_FOR '(' opt_simple_stmt semi opt_nls exp semi opt_nls opt_simple_stmt r_paren opt_nls statement
	  {
		$$ = node($12, Node_K_for, (NODE *) make_for_loop($3, $6, $9));
	  }
	| LEX_FOR '(' opt_simple_stmt semi opt_nls semi opt_nls opt_simple_stmt r_paren opt_nls statement
	  {
		$$ = node($11, Node_K_for,
			(NODE *) make_for_loop($3, (NODE *) NULL, $8));
	  }
	| LEX_BREAK statement_term
	   /* for break, maybe we'll have to remember where to break to */
		{ $$ = node((NODE *) NULL, Node_K_break, (NODE *) NULL); }
	| LEX_CONTINUE statement_term
	   /* similarly */
		{ $$ = node((NODE *) NULL, Node_K_continue, (NODE *) NULL); }
	| LEX_NEXT statement_term
		{ NODETYPE type;

		  if (begin_or_end_rule)
			yyerror(_("`%s' used in %s action"), "next",
				(parsing_end_rule ? "END" : "BEGIN"));
		  type = Node_K_next;
		  $$ = node((NODE *) NULL, type, (NODE *) NULL);
		}
	| LEX_NEXTFILE statement_term
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
		  $$ = node((NODE *) NULL, Node_K_nextfile, (NODE *) NULL);
		}
	| LEX_EXIT opt_exp statement_term
		{ $$ = node($2, Node_K_exit, (NODE *) NULL); }
	| LEX_RETURN
		{
		  if (! can_return)
			yyerror(_("`return' used outside function context"));
		}
	  opt_exp statement_term
		{ $$ = node($3, Node_K_return, (NODE *) NULL); }
	| simple_stmt statement_term
	;

	/*
	 * A simple_stmt exists to satisfy a constraint in the POSIX
	 * grammar allowing them to occur as the 1st and 3rd parts
	 * in a `for (...;...;...)' loop.  This is a historical oddity
	 * inherited from Unix awk, not at all documented in the AK&W
	 * awk book.  We support it, as this was reported as a bug.
	 * We don't bother to document it though. So there.
	 */
simple_stmt
	: print { in_print = TRUE; in_parens = 0; } print_expression_list output_redir
	  {
		/*
		 * Optimization: plain `print' has no expression list, so $3 is null.
		 * If $3 is an expression list with one element (rnode == null)
		 * and lnode is a field spec for field 0, we have `print $0'.
		 * For both, use Node_K_print_rec, which is faster for these two cases.
		 */
		if ($1 == Node_K_print &&
		    ($3 == NULL
		     || ($3->type == Node_expression_list
			&& $3->rnode == NULL
			&& $3->lnode->type == Node_field_spec
			&& $3->lnode->lnode->type == Node_val
			&& $3->lnode->lnode->numbr == 0.0))
		) {
			static int warned = FALSE;

			$$ = node(NULL, Node_K_print_rec, $4);

			if (do_lint && $3 == NULL && begin_or_end_rule && ! warned) {
				warned = TRUE;
				lintwarn(
	_("plain `print' in BEGIN or END rule should probably be `print \"\"'"));
			}
		} else {
			$$ = node($3, $1, $4);
			if ($$->type == Node_K_printf)
				count_args($$);
		}
	  }
	| LEX_DELETE NAME '[' expression_list ']'
		{ $$ = node(variable($2, CAN_FREE, Node_var_array), Node_K_delete, $4); }
	| LEX_DELETE NAME
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
		  $$ = node(variable($2, CAN_FREE, Node_var_array), Node_K_delete, (NODE *) NULL);
		}
	| LEX_DELETE '(' NAME ')'
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
		  $$ = node(variable($3, CAN_FREE, Node_var_array), Node_K_delete, (NODE *) NULL);
		}
	| exp
		{ $$ = $1; }
	;

opt_simple_stmt
	: /* empty */
		{ $$ = NULL; }
	| simple_stmt
		{ $$ = $1; }
	;

switch_body
	: case_statements
	  {
		if ($1 == NULL) {
			$$ = NULL;
		} else {
			NODE *dflt = NULL;
			NODE *head = $1;
			NODE *curr;
	
			const char **case_values = NULL;
	
			int maxcount = 128;
			int case_count = 0;
			int i;
	
			emalloc(case_values, const char **, sizeof(char*) * maxcount, "switch_body");
			for (curr = $1; curr != NULL; curr = curr->rnode) {
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
			$$ = node(head, Node_switch_body, dflt);
		}
	}
	;

case_statements
	: /* empty */
	  { $$ = NULL; }
	| case_statements case_statement
	  {
		if ($2 == NULL)
			$$ = $1;
		else {
			if (do_lint && isnoeffect($2->type))
				lintwarn(_("statement may have no effect"));
			if ($1 == NULL)
				$$ = node($2, Node_case_list, (NODE *) NULL);
			else
				$$ = append_right(
					($1->type == Node_case_list ? $1 : node($1, Node_case_list, (NODE *) NULL)),
					($2->type == Node_case_list ? $2 : node($2, Node_case_list, (NODE *) NULL))
				);
		}
	    	yyerrok;
	  }
	| case_statements error
	  { $$ = NULL; }
	;

case_statement
	: LEX_CASE case_value colon opt_nls statements
		{ $$ = node($2, Node_K_case, $5); }
	| LEX_DEFAULT colon opt_nls statements
		{ $$ = node((NODE *) NULL, Node_K_default, $4); }
	;

case_value
	: YNUMBER
		{ $$ = $1; }
	| '-' YNUMBER    %prec UNARY
	  {
		$2->numbr = -(force_number($2));
		$$ = $2;
	  }
	| '+' YNUMBER    %prec UNARY
		{ $$ = $2; }
	| YSTRING
		{ $$ = $1; }
	| regexp
		{ $$ = $1; }
	;

print
	: LEX_PRINT
	| LEX_PRINTF
	;

	/*
	 * Note: ``print(x)'' is already parsed by the first rule,
	 * so there is no good in covering it by the second one too.
	 */
print_expression_list
	: opt_expression_list
	| '(' exp comma expression_list r_paren
		{ $$ = node($2, Node_expression_list, $4); }
	;

output_redir
	: /* empty */
	  {
		in_print = FALSE;
		in_parens = 0;
		$$ = NULL;
	  }
	| IO_OUT { in_print = FALSE; in_parens = 0; } common_exp
	  {
		$$ = node($3, $1, (NODE *) NULL);
		if ($1 == Node_redirect_twoway
		    && $3->type == Node_K_getline
		    && $3->rnode->type == Node_redirect_twoway)
			yyerror(_("multistage two-way pipelines don't work"));
	  }
	;

if_statement
	: LEX_IF '(' exp r_paren opt_nls statement
	  {
		$$ = node($3, Node_K_if, 
			node($6, Node_if_branches, (NODE *) NULL));
	  }
	| LEX_IF '(' exp r_paren opt_nls statement
	     LEX_ELSE opt_nls statement
		{ $$ = node($3, Node_K_if,
				node($6, Node_if_branches, $9)); }
	;

nls
	: NEWLINE
	| nls NEWLINE
	;

opt_nls
	: /* empty */
	| nls
	;

input_redir
	: /* empty */
		{ $$ = NULL; }
	| '<' simp_exp
		{ $$ = node($2, Node_redirect_input, (NODE *) NULL); }
	;

opt_param_list
	: /* empty */
		{ $$ = NULL; }
	| param_list
		{ $$ = $1; }
	;

param_list
	: NAME
		{ $$ = make_param($1); }
	| param_list comma NAME
		{ $$ = append_right($1, make_param($3)); yyerrok; }
	| error
		{ $$ = NULL; }
	| param_list error
		{ $$ = NULL; }
	| param_list comma error
		{ $$ = NULL; }
	;

/* optional expression, as in for loop */
opt_exp
	: /* empty */
		{ $$ = NULL; }
	| exp
		{ $$ = $1; }
	;

opt_expression_list
	: /* empty */
		{ $$ = NULL; }
	| expression_list
		{ $$ = $1; }
	;

expression_list
	: exp
		{ $$ = node($1, Node_expression_list, (NODE *) NULL); }
	| expression_list comma exp
		{
			$$ = append_right($1,
				node($3, Node_expression_list, (NODE *) NULL));
			yyerrok;
		}
	| error
		{ $$ = NULL; }
	| expression_list error
		{ $$ = NULL; }
	| expression_list error exp
		{ $$ = NULL; }
	| expression_list comma error
		{ $$ = NULL; }
	;

/* Expressions, not including the comma operator.  */
exp	: variable assign_operator exp %prec ASSIGNOP
		{
		  if (do_lint && $3->type == Node_regex)
			lintwarn(_("regular expression on right of assignment"));
		  $$ = node($1, $2, $3);
		}
	| exp LEX_AND exp
		{ $$ = node($1, Node_and, $3); }
	| exp LEX_OR exp
		{ $$ = node($1, Node_or, $3); }
	| exp MATCHOP exp
		{
		  if ($1->type == Node_regex)
			warning(_("regular expression on left of `~' or `!~' operator"));
		  $$ = node($1, $2, mk_rexp($3));
		}
	| exp LEX_IN NAME
		{ $$ = node(variable($3, CAN_FREE, Node_var_array), Node_in_array, $1); }
	| exp a_relop exp %prec RELOP
		{
		  if (do_lint && $3->type == Node_regex)
			lintwarn(_("regular expression on right of comparison"));
		  $$ = node($1, $2, $3);
		}
	| exp '?' exp ':' exp
		{ $$ = node($1, Node_cond_exp, node($3, Node_if_branches, $5));}
	| common_exp
		{ $$ = $1; }
	;

assign_operator
	: ASSIGN
		{ $$ = $1; }
	| ASSIGNOP
		{ $$ = $1; }
	| SLASH_BEFORE_EQUAL ASSIGN   /* `/=' */
		{ $$ = Node_assign_quotient; }
	;

relop_or_less
	: RELOP
		{ $$ = $1; }
	| '<'
		{ $$ = Node_less; }
	;
a_relop
	: relop_or_less
	| '>'
		{ $$ = Node_greater; }
	;

common_exp
	: regexp
		{ $$ = $1; }
	| '!' regexp %prec UNARY
		{
		  $$ = node(node(make_number(0.0),
				 Node_field_spec,
				 (NODE *) NULL),
		            Node_nomatch,
			    $2);
		}
	| '(' expression_list r_paren LEX_IN NAME
		{ $$ = node(variable($5, CAN_FREE, Node_var_array), Node_in_array, $2); }
	| simp_exp
		{ $$ = $1; }
	| common_exp simp_exp %prec CONCAT_OP
		{ $$ = node($1, Node_concat, $2); }
	;

simp_exp
	: non_post_simp_exp
	/* Binary operators in order of decreasing precedence.  */
	| simp_exp '^' simp_exp
		{ $$ = node($1, Node_exp, $3); }
	| simp_exp '*' simp_exp
		{ $$ = node($1, Node_times, $3); }
	| simp_exp '/' simp_exp
		{ $$ = node($1, Node_quotient, $3); }
	| simp_exp '%' simp_exp
		{ $$ = node($1, Node_mod, $3); }
	| simp_exp '+' simp_exp
		{ $$ = node($1, Node_plus, $3); }
	| simp_exp '-' simp_exp
		{ $$ = node($1, Node_minus, $3); }
	| LEX_GETLINE opt_variable input_redir
		{
		  if (do_lint && parsing_end_rule && $3 == NULL)
			lintwarn(_("non-redirected `getline' undefined inside END action"));
		  $$ = node($2, Node_K_getline, $3);
		}
	| simp_exp IO_IN LEX_GETLINE opt_variable
		{
		  $$ = node($4, Node_K_getline,
			 node($1, $2, (NODE *) NULL));
		}
	| variable INCREMENT
		{ $$ = node($1, Node_postincrement, (NODE *) NULL); }
	| variable DECREMENT
		{ $$ = node($1, Node_postdecrement, (NODE *) NULL); }
	;

non_post_simp_exp
	: '!' simp_exp %prec UNARY
		{ $$ = node($2, Node_not, (NODE *) NULL); }
	| '(' exp r_paren
		{ $$ = $2; }
	| LEX_BUILTIN
	  '(' opt_expression_list r_paren
		{ $$ = snode($3, Node_builtin, (int) $1); }
	| LEX_LENGTH '(' opt_expression_list r_paren
		{ $$ = snode($3, Node_builtin, (int) $1); }
	| LEX_LENGTH
	  {
		if (do_lint)
			lintwarn(_("call of `length' without parentheses is not portable"));
		$$ = snode((NODE *) NULL, Node_builtin, (int) $1);
		if (do_posix)
			warning(_("call of `length' without parentheses is deprecated by POSIX"));
	  }
	| FUNC_CALL '(' opt_expression_list r_paren
	  {
		$$ = node($3, Node_func_call, make_string($1, strlen($1)));
		$$->funcbody = NULL;
		func_use($1, FUNC_USE);
		param_sanity($3);
		free($1);
	  }
	| variable
	| INCREMENT variable
		{ $$ = node($2, Node_preincrement, (NODE *) NULL); }
	| DECREMENT variable
		{ $$ = node($2, Node_predecrement, (NODE *) NULL); }
	| YNUMBER
		{ $$ = $1; }
	| YSTRING
		{ $$ = $1; }

	| '-' simp_exp    %prec UNARY
		{
		  if ($2->type == Node_val && ($2->flags & (STRCUR|STRING)) == 0) {
			$2->numbr = -(force_number($2));
			$$ = $2;
		  } else
			$$ = node($2, Node_unary_minus, (NODE *) NULL);
		}
	| '+' simp_exp    %prec UNARY
		{
		  /*
		   * was: $$ = $2
		   * POSIX semantics: force a conversion to numeric type
		   */
		  $$ = node (make_number(0.0), Node_plus, $2);
		}
	;

opt_variable
	: /* empty */
		{ $$ = NULL; }
	| variable
		{ $$ = $1; }
	;

variable
	: NAME
		{ $$ = variable($1, CAN_FREE, Node_var_new); }
	| NAME '[' expression_list ']'
	  {
		NODE *n;

		if ((n = lookup($1)) != NULL && ! isarray(n))
			yyerror(_("use of non-array as array"));
		else if ($3 == NULL) {
			fatal(_("invalid subscript expression"));
		} else if ($3->rnode == NULL) {
			$$ = node(variable($1, CAN_FREE, Node_var_array), Node_subscript, $3->lnode);
			freenode($3);
		} else
			$$ = node(variable($1, CAN_FREE, Node_var_array), Node_subscript, $3);
	  }
	| '$' non_post_simp_exp
		{ $$ = node($2, Node_field_spec, (NODE *) NULL); }
	;

l_brace
	: '{' opt_nls
	;

r_brace
	: '}' opt_nls	{ yyerrok; }
	;

r_paren
	: ')' { yyerrok; }
	;

opt_semi
	: /* empty */
	| semi
	;

semi
	: ';'	{ yyerrok; }
	;

colon
	: ':'	{ yyerrok; }
	;

comma	: ',' opt_nls	{ yyerrok; }
	;

%%

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
