/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

# include "jam.h"

# include "lists.h"
# include "parse.h"
# include "compile.h"
# include "variable.h"
# include "expand.h"
# include "rules.h"
# include "newstr.h"
# include "search.h"

/*
 * compile.c - compile parsed jam statements
 *
 * External routines:
 *
 *	compile_append() - append list results of two statements
 *	compile_eval() - evaluate if to determine which leg to compile
 *	compile_foreach() - compile the "for x in y" statement
 *	compile_if() - compile 'if' rule
 *	compile_include() - support for 'include' - call include() on file
 *	compile_list() - expand and return a list 
 *	compile_local() - declare (and set) local variables
 *	compile_null() - do nothing -- a stub for parsing
 *	compile_on() - run rule under influence of on-target variables
 *	compile_rule() - compile a single user defined rule
 *	compile_rules() - compile a chain of rules
 *	compile_set() - compile the "set variable" statement
 *	compile_setcomp() - support for `rule` - save parse tree 
 *	compile_setexec() - support for `actions` - save execution string 
 *	compile_settings() - compile the "on =" (set variable on exec) statement
 *	compile_switch() - compile 'switch' rule
 *
 * Internal routines:
 *
 *	debug_compile() - printf with indent to show rule expansion.
 *	evaluate_rule() - execute a rule invocation
 *
 * 02/03/94 (seiwald) -	Changed trace output to read "setting" instead of 
 *			the awkward sounding "settings".
 * 04/12/94 (seiwald) - Combined build_depends() with build_includes().
 * 04/12/94 (seiwald) - actionlist() now just appends a single action.
 * 04/13/94 (seiwald) - added shorthand L0 for null list pointer
 * 05/13/94 (seiwald) - include files are now bound as targets, and thus
 *			can make use of $(SEARCH)
 * 06/01/94 (seiwald) - new 'actions existing' does existing sources
 * 08/23/94 (seiwald) - Support for '+=' (append to variable)
 * 12/20/94 (seiwald) - NOTIME renamed NOTFILE.
 * 01/22/95 (seiwald) - Exit rule.
 * 02/02/95 (seiwald) - Always rule; LEAVES rule.
 * 02/14/95 (seiwald) - NoUpdate rule.
 * 09/11/00 (seiwald) - new evaluate_rule() for headers().
 * 09/11/00 (seiwald) - compile_xxx() now return LIST *.
 *			New compile_append() and compile_list() in
 *			support of building lists here, rather than
 *			in jamgram.yy.
 * 01/10/00 (seiwald) - built-ins split out to builtin.c.
 */

static void debug_compile( int which, char *s );
int glob( char *s, char *c );



/*
 * compile_append() - append list results of two statements
 *
 *	parse->left	more compile_append() by left-recursion
 *	parse->right	single rule
 */

LIST *
compile_append(
	PARSE	*parse,
	LOL	*args )
{
	/* Append right to left. */

	return list_append( 
		(*parse->left->func)( parse->left, args ),
		(*parse->right->func)( parse->right, args ) );
}

/*
 * compile_eval() - evaluate if to determine which leg to compile
 *
 * Returns:
 *	list 	if expression true - compile 'then' clause
 *	L0	if expression false - compile 'else' clause
 */

static int
lcmp( LIST *t, LIST *s )
{
	int status = 0;

	while( !status && ( t || s ) )
	{
	    char *st = t ? t->string : "";
	    char *ss = s ? s->string : "";

	    status = strcmp( st, ss );

	    t = t ? list_next( t ) : t;
	    s = s ? list_next( s ) : s;
	}

	return status;
}

LIST *
compile_eval(
	PARSE	*parse,
	LOL	*args )
{
	LIST *ll, *lr, *s, *t;
	int status = 0;

	/* Short circuit lr eval for &&, ||, and 'in' */

	ll = (*parse->left->func)( parse->left, args );
	lr = 0;

	switch( parse->num )
	{
	case EXPR_AND: 
	case EXPR_IN: 	if( ll ) goto eval; break;
	case EXPR_OR: 	if( !ll ) goto eval; break;
	default: eval: 	lr = (*parse->right->func)( parse->right, args );
	}

	/* Now eval */

	switch( parse->num )
	{
	case EXPR_NOT:	
		if( !ll ) status = 1;
		break;

	case EXPR_AND:
		if( ll && lr ) status = 1;
		break;

	case EXPR_OR:
		if( ll || lr ) status = 1;
		break;

	case EXPR_IN:
		/* "a in b": make sure each of */
		/* ll is equal to something in lr. */

		for( t = ll; t; t = list_next( t ) )
		{
		    for( s = lr; s; s = list_next( s ) )
			if( !strcmp( t->string, s->string ) )
			    break;
		    if( !s ) break;
		}

		/* No more ll? Success */

		if( !t ) status = 1;

		break;

	case EXPR_EXISTS:       if( lcmp( ll, L0 ) != 0 ) status = 1; break;
	case EXPR_EQUALS:	if( lcmp( ll, lr ) == 0 ) status = 1; break;
	case EXPR_NOTEQ:	if( lcmp( ll, lr ) != 0 ) status = 1; break;
	case EXPR_LESS:		if( lcmp( ll, lr ) < 0  ) status = 1; break;
	case EXPR_LESSEQ:	if( lcmp( ll, lr ) <= 0 ) status = 1; break;
	case EXPR_MORE:		if( lcmp( ll, lr ) > 0  ) status = 1; break;
	case EXPR_MOREEQ:	if( lcmp( ll, lr ) >= 0 ) status = 1; break;

	}

	if( DEBUG_IF )
	{
	    debug_compile( 0, "if" );
	    list_print( ll );
	    printf( "(%d) ", status );
	    list_print( lr );
	    printf( "\n" );
	}

	/* Find something to return. */
	/* In odd circumstances (like "" = "") */
	/* we'll have to return a new string. */

	if( !status ) t = 0;
	else if( ll ) t = ll, ll = 0;
	else if( lr ) t = lr, lr = 0;
	else t = list_new( L0, newstr( "1" ) );

	if( ll ) list_free( ll );
	if( lr ) list_free( lr );
	return t;
}

/*
 * compile_foreach() - compile the "for x in y" statement
 *
 * Compile_foreach() resets the given variable name to each specified
 * value, executing the commands enclosed in braces for each iteration.
 *
 *	parse->string	index variable
 *	parse->left	variable values
 *	parse->right	rule to compile
 */

LIST *
compile_foreach(
	PARSE	*parse,
	LOL	*args )
{
	LIST	*nv = (*parse->left->func)( parse->left, args );
	LIST	*l;

	/* Call var_set to reset $(parse->string) for each val. */

	for( l = nv; l; l = list_next( l ) )
	{
	    LIST *val = list_new( L0, copystr( l->string ) );

	    var_set( parse->string, val, VAR_SET );

	    list_free( (*parse->right->func)( parse->right, args ) );
	}

	list_free( nv );

	return L0;
}

/*
 * compile_if() - compile 'if' rule
 *
 *	parse->left		condition tree
 *	parse->right		then tree
 *	parse->third		else tree
 */

LIST *
compile_if(
	PARSE	*p,
	LOL	*args )
{
	LIST *l = (*p->left->func)( p->left, args );

	if( l )
	{
	    list_free( l );
	    return (*p->right->func)( p->right, args );
	}
	else
	{
	    return (*p->third->func)( p->third, args );
	}
}

/*
 * compile_include() - support for 'include' - call include() on file
 *
 * 	parse->left	list of files to include (can only do 1)
 */

LIST *
compile_include(
	PARSE	*parse,
	LOL	*args )
{
	LIST	*nt = (*parse->left->func)( parse->left, args );

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "include" );
	    list_print( nt );
	    printf( "\n" );
	}

	if( nt )
	{
	    TARGET *t = bindtarget( nt->string );

	    /* Bind the include file under the influence of */
	    /* "on-target" variables.  Though they are targets, */
	    /* include files are not built with make(). */

	    pushsettings( t->settings );
	    t->boundname = search( t->name, &t->time );
	    popsettings( t->settings );

	    parse_file( t->boundname );
	}

	list_free( nt );

	return L0;
}

/*
 * compile_list() - expand and return a list 
 *
 * 	parse->string - character string to expand
 */

LIST *
compile_list(
	PARSE	*parse,
	LOL	*args )
{
	/* voodoo 1 means: s is a copyable string */
	char *s = parse->string;
	return var_expand( L0, s, s + strlen( s ), args, 1 );
}

/*
 * compile_local() - declare (and set) local variables
 *
 *	parse->left	list of variables
 *	parse->right	list of values
 *	parse->third	rules to execute
 */

LIST *
compile_local(
	PARSE	*parse,
	LOL	*args )
{
	LIST *l;
	SETTINGS *s = 0;
	LIST	*nt = (*parse->left->func)( parse->left, args );
	LIST	*ns = (*parse->right->func)( parse->right, args );
	LIST	*result;

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "local" );
	    list_print( nt );
	    printf( " = " );
	    list_print( ns );
	    printf( "\n" );
	}

	/* Initial value is ns */

	for( l = nt; l; l = list_next( l ) )
	    s = addsettings( s, 0, l->string, list_copy( (LIST*)0, ns ) );

	list_free( ns );
	list_free( nt );

	/* Note that callees of the current context get this "local" */
	/* variable, making it not so much local as layered. */

	pushsettings( s );
	result = (*parse->third->func)( parse->third, args );
	popsettings( s );

	freesettings( s );

	return result;
}

/*
 * compile_null() - do nothing -- a stub for parsing
 */

LIST *
compile_null(
	PARSE	*parse,
	LOL	*args )
{
	return L0;
}

/*
 * compile_on() - run rule under influence of on-target variables
 *
 * 	parse->left	list of files to include (can only do 1)
 *	parse->right	rule to run
 *
 * EXPERIMENTAL!
 */

LIST *
compile_on(
	PARSE	*parse,
	LOL	*args )
{
	LIST	*nt = (*parse->left->func)( parse->left, args );
	LIST	*result = 0;

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "on" );
	    list_print( nt );
	    printf( "\n" );
	}

	if( nt )
	{
	    TARGET *t = bindtarget( nt->string );

	    pushsettings( t->settings );
	    result = (*parse->right->func)( parse->right, args );
	    t->boundname = search( t->name, &t->time );
	    popsettings( t->settings );

	}

	list_free( nt );

	return result;
}


/*
 * compile_rule() - compile a single user defined rule
 *
 *	parse->left	list of rules to run
 *	parse->right	parameters (list of lists) to rule, recursing left
 *
 * Wrapped around evaluate_rule() so that headers() can share it.
 */

LIST *
compile_rule(
	PARSE	*parse,
	LOL	*args )
{
	LOL	nargs[1];
	LIST	*result = 0;
	LIST	*ll, *l;
	PARSE	*p;

	/* list of rules to run -- normally 1! */

	ll = (*parse->left->func)( parse->left, args );

	/* Build up the list of arg lists */

	lol_init( nargs );

	for( p = parse->right; p; p = p->left )
	    lol_add( nargs, (*p->right->func)( p->right, args ) );

	/* Run rules, appending results from each */

	for( l = ll; l; l = list_next( l ) )
	    result = evaluate_rule( l->string, nargs, result );

	list_free( ll );
	lol_free( nargs );

	return result;
}

/*
 * evaluate_rule() - execute a rule invocation
 */

LIST *
evaluate_rule(
	char	*rulename,
	LOL	*args, 
	LIST	*result )
{
	RULE	*rule = bindrule( rulename );

	if( DEBUG_COMPILE )
	{
	    debug_compile( 1, rulename );
	    lol_print( args );
	    printf( "\n" );
	}

	/* Check traditional targets $(<) and sources $(>) */

	if( !rule->actions && !rule->procedure )
	    printf( "warning: unknown rule %s\n", rule->name );

	/* If this rule will be executed for updating the targets */
	/* then construct the action for make(). */

	if( rule->actions )
	{
	    TARGETS	*t;
	    ACTION	*action;

	    /* The action is associated with this instance of this rule */

	    action = (ACTION *)malloc( sizeof( ACTION ) );
	    memset( (char *)action, '\0', sizeof( *action ) );

	    action->rule = rule;
	    action->targets = targetlist( (TARGETS *)0, lol_get( args, 0 ) );
	    action->sources = targetlist( (TARGETS *)0, lol_get( args, 1 ) );

	    /* Append this action to the actions of each target */

	    for( t = action->targets; t; t = t->next )
		t->target->actions = actionlist( t->target->actions, action );
	}

	/* Now recursively compile any parse tree associated with this rule */
	/* refer/free to ensure rule not freed during use */

	if( rule->procedure )
	{
	    PARSE *parse = rule->procedure;
	    parse_refer( parse );
	    result = list_append( result, (*parse->func)( parse, args ) );
	    parse_free( parse );
	}

	if( DEBUG_COMPILE )
	    debug_compile( -1, 0 );

	return result;
}

/*
 * compile_rules() - compile a chain of rules
 *
 *	parse->left	single rule
 *	parse->right	more compile_rules() by right-recursion
 */

LIST *
compile_rules(
	PARSE	*parse,
	LOL	*args )
{
	/* Ignore result from first statement; return the 2nd. */
	/* Optimize recursion on the right by looping. */

	do list_free( (*parse->left->func)( parse->left, args ) );
	while( (parse = parse->right)->func == compile_rules );

	return (*parse->func)( parse, args );
}

/*
 * compile_set() - compile the "set variable" statement
 *
 *	parse->left	variable names
 *	parse->right	variable values 
 *	parse->num	ASSIGN_SET/APPEND/DEFAULT
 */

LIST *
compile_set(
	PARSE	*parse,
	LOL	*args )
{
	LIST	*nt = (*parse->left->func)( parse->left, args );
	LIST	*ns = (*parse->right->func)( parse->right, args );
	LIST	*l;
	int	setflag;
	char	*trace;

	switch( parse->num )
	{
	case ASSIGN_SET:	setflag = VAR_SET; trace = "="; break;
	case ASSIGN_APPEND:	setflag = VAR_APPEND; trace = "+="; break;
	case ASSIGN_DEFAULT:	setflag = VAR_DEFAULT; trace = "?="; break;
	default:		setflag = VAR_SET; trace = ""; break;
	}

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "set" );
	    list_print( nt );
	    printf( " %s ", trace );
	    list_print( ns );
	    printf( "\n" );
	}

	/* Call var_set to set variable */
	/* var_set keeps ns, so need to copy it */

	for( l = nt; l; l = list_next( l ) )
	    var_set( l->string, list_copy( L0, ns ), setflag );

	list_free( nt );

	return ns;
}

/*
 * compile_setcomp() - support for `rule` - save parse tree 
 *
 *	parse->string	rule name
 *	parse->left	rules for rule
 */

LIST *
compile_setcomp(
	PARSE	*parse,
	LOL	*args )
{
	RULE	*rule = bindrule( parse->string );

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "rule" );
	    printf( " %s\n", parse->string );
	}

	/* Free old one, if present */

	if( rule->procedure )
	    parse_free( rule->procedure );

	rule->procedure = parse->left;

	/* we now own this parse tree */
	/* don't let parse_free() release it */

	parse_refer( parse->left );

	return L0;
}

/*
 * compile_setexec() - support for `actions` - save execution string 
 *
 *	parse->string	rule name
 *	parse->string1	OS command string
 *	parse->num	flags
 *	parse->left	`bind` variables
 *
 * Note that the parse flags (as defined in compile.h) are transfered
 * directly to the rule flags (as defined in rules.h).
 */

LIST *
compile_setexec(
	PARSE	*parse,
	LOL	*args )
{
	RULE	*rule = bindrule( parse->string );
	LIST	*bindlist = (*parse->left->func)( parse->left, args );
	
	/* Free old one, if present */

	if( rule->actions )
	{
	    freestr( rule->actions );
	    list_free( rule->bindlist );
	}

	rule->actions = copystr( parse->string1 );
	rule->bindlist = bindlist;
	rule->flags = parse->num;

	return L0;
}

/*
 * compile_settings() - compile the "on =" (set variable on exec) statement
 *
 *	parse->left	variable names
 *	parse->right	target name 
 *	parse->third	variable value 
 *	parse->num	ASSIGN_SET/APPEND	
 */

LIST *
compile_settings(
	PARSE	*parse,
	LOL	*args )
{
	LIST	*nt = (*parse->left->func)( parse->left, args );
	LIST	*ns = (*parse->third->func)( parse->third, args );
	LIST	*targets = (*parse->right->func)( parse->right, args );
	LIST	*ts;
	int	append = parse->num == ASSIGN_APPEND;

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "set" );
	    list_print( nt );
	    printf( "on " );
	    list_print( targets );
	    printf( " %s ", append ? "+=" : "=" );
	    list_print( ns );
	    printf( "\n" );
	}

	/* Call addsettings to save variable setting */
	/* addsettings keeps ns, so need to copy it */
	/* Pass append flag to addsettings() */

	for( ts = targets; ts; ts = list_next( ts ) )
	{
	    TARGET 	*t = bindtarget( ts->string );
	    LIST	*l;

	    for( l = nt; l; l = list_next( l ) )
		t->settings = addsettings( t->settings, append, 
				l->string, list_copy( (LIST*)0, ns ) );
	}

	list_free( nt );
	list_free( targets );

	return ns;
}

/*
 * compile_switch() - compile 'switch' rule
 *
 *	parse->left	switch value (only 1st used)
 *	parse->right	cases
 *
 *	cases->left	1st case
 *	cases->right	next cases
 *
 *	case->string	argument to match
 *	case->left	parse tree to execute
 */

LIST *
compile_switch(
	PARSE	*parse,
	LOL	*args )
{
	LIST	*nt = (*parse->left->func)( parse->left, args );
	LIST	*result = 0;

	if( DEBUG_COMPILE )
	{
	    debug_compile( 0, "switch" );
	    list_print( nt );
	    printf( "\n" );
	}

	/* Step through cases */

	for( parse = parse->right; parse; parse = parse->right )
	{
	    if( !glob( parse->left->string, nt ? nt->string : "" ) )
	    {
		/* Get & exec parse tree for this case */
		parse = parse->left->left;
		result = (*parse->func)( parse, args );
		break;
	    }
	}

	list_free( nt );

	return result;
}

/*
 * compile_while() - compile 'while' rule
 *
 *	parse->left		condition tree
 *	parse->right		execution tree
 */

LIST *
compile_while(
	PARSE	*p,
	LOL	*args )
{
	LIST *r = 0;
	LIST *l;

	/* Returns the value from the last execution of the block */

	while( l = (*p->left->func)( p->left, args ) )
	{
	    list_free( l );
	    if( r ) list_free( r );
	    r = (*p->right->func)( p->right, args );
	}

	return r;
}

/*
 * debug_compile() - printf with indent to show rule expansion.
 */

static void
debug_compile( int which, char *s )
{
	static int level = 0;
	static char indent[36] = ">>>>|>>>>|>>>>|>>>>|>>>>|>>>>|>>>>|";
	int i = ((1+level) * 2) % 35;

	if( which >= 0 )
	    printf( "%*.*s ", i, i, indent );

	if( s )
	    printf( "%s ", s );

	level += which;
}
