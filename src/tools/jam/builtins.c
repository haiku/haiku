/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

# include "jam.h"

# include "lists.h"
# include "parse.h"
# include "builtins.h"
# include "rules.h"
# include "filesys.h"
# include "newstr.h"
# include "regexp.h"
# include "pathsys.h"

/*
 * builtins.c - builtin jam rules
 *
 * External routines:
 *
 * 	load_builtin() - define builtin rules
 *
 * Internal routines:
 *
 *	builtin_depends() - DEPENDS/INCLUDES rule
 *	builtin_echo() - ECHO rule
 *	builtin_exit() - EXIT rule
 *	builtin_flags() - NOCARE, NOTFILE, TEMPORARY rule
 *	builtin_glob() - GLOB rule
 *	builtin_match() - MATCH rule
 *
 * 01/10/01 (seiwald) - split from compile.c
 */

/*
 * compile_builtin() - define builtin rules
 */

# define P0 (PARSE *)0
# define C0 (char *)0

LIST *builtin_depends( PARSE *parse, LOL *args );
LIST *builtin_echo( PARSE *parse, LOL *args );
LIST *builtin_exit( PARSE *parse, LOL *args );
LIST *builtin_flags( PARSE *parse, LOL *args );
LIST *builtin_glob( PARSE *parse, LOL *args );
LIST *builtin_match( PARSE *parse, LOL *args );

int glob( char *s, char *c );

void
load_builtins()
{
    bindrule( "Always" )->procedure = 
    bindrule( "ALWAYS" )->procedure = 
	parse_make( builtin_flags, P0, P0, P0, C0, C0, T_FLAG_TOUCHED );

    bindrule( "Depends" )->procedure = 
    bindrule( "DEPENDS" )->procedure = 
	parse_make( builtin_depends, P0, P0, P0, C0, C0, T_DEPS_DEPENDS );

    bindrule( "echo" )->procedure = 
    bindrule( "Echo" )->procedure = 
    bindrule( "ECHO" )->procedure = 
	parse_make( builtin_echo, P0, P0, P0, C0, C0, 0 );

    bindrule( "exit" )->procedure = 
    bindrule( "Exit" )->procedure = 
    bindrule( "EXIT" )->procedure = 
	parse_make( builtin_exit, P0, P0, P0, C0, C0, 0 );

    bindrule( "Glob" )->procedure = 
    bindrule( "GLOB" )->procedure = 
	parse_make( builtin_glob, P0, P0, P0, C0, C0, 0 );

    bindrule( "Includes" )->procedure = 
    bindrule( "INCLUDES" )->procedure = 
	parse_make( builtin_depends, P0, P0, P0, C0, C0, T_DEPS_INCLUDES );

    bindrule( "Leaves" )->procedure = 
    bindrule( "LEAVES" )->procedure = 
	parse_make( builtin_flags, P0, P0, P0, C0, C0, T_FLAG_LEAVES );

    bindrule( "Match" )->procedure = 
    bindrule( "MATCH" )->procedure = 
	parse_make( builtin_match, P0, P0, P0, C0, C0, 0 );

    bindrule( "NoCare" )->procedure = 
    bindrule( "NOCARE" )->procedure = 
	parse_make( builtin_flags, P0, P0, P0, C0, C0, T_FLAG_NOCARE );

    bindrule( "NOTIME" )->procedure = 
    bindrule( "NotFile" )->procedure = 
    bindrule( "NOTFILE" )->procedure = 
	parse_make( builtin_flags, P0, P0, P0, C0, C0, T_FLAG_NOTFILE );

    bindrule( "NoUpdate" )->procedure = 
    bindrule( "NOUPDATE" )->procedure = 
	parse_make( builtin_flags, P0, P0, P0, C0, C0, T_FLAG_NOUPDATE );

    bindrule( "Temporary" )->procedure = 
    bindrule( "TEMPORARY" )->procedure = 
	parse_make( builtin_flags, P0, P0, P0, C0, C0, T_FLAG_TEMP );
}

/*
 * builtin_depends() - DEPENDS/INCLUDES rule
 *
 * The DEPENDS builtin rule appends each of the listed sources on the 
 * dependency list of each of the listed targets.  It binds both the 
 * targets and sources as TARGETs.
 */

LIST *
builtin_depends(
	PARSE	*parse,
	LOL	*args )
{
	LIST *targets = lol_get( args, 0 );
	LIST *sources = lol_get( args, 1 );
	int which = parse->num;
	LIST *l;

	for( l = targets; l; l = list_next( l ) )
	{
	    TARGET *t = bindtarget( l->string );
	    t->deps[ which ] = targetlist( t->deps[ which ], sources );
	}

	return L0;
}

/*
 * builtin_echo() - ECHO rule
 *
 * The ECHO builtin rule echoes the targets to the user.  No other 
 * actions are taken.
 */

LIST *
builtin_echo(
	PARSE	*parse,
	LOL	*args )
{
	list_print( lol_get( args, 0 ) );
	printf( "\n" );
	return L0;
}

/*
 * builtin_exit() - EXIT rule
 *
 * The EXIT builtin rule echoes the targets to the user and exits
 * the program with a failure status.
 */

LIST *
builtin_exit(
	PARSE	*parse,
	LOL	*args )
{
	list_print( lol_get( args, 0 ) );
	printf( "\n" );
	exit( EXITBAD ); /* yeech */
	return L0;
}

/*
 * builtin_flags() - NOCARE, NOTFILE, TEMPORARY rule
 *
 * Builtin_flags() marks the target with the appropriate flag, for use
 * by make0().  It binds each target as a TARGET.
 */

LIST *
builtin_flags(
	PARSE	*parse,
	LOL	*args )
{
	LIST *l = lol_get( args, 0 );

	for( ; l; l = list_next( l ) )
	    bindtarget( l->string )->flags |= parse->num;

	return L0;
}

/*
 * builtin_globbing() - GLOB rule
 */

struct globbing {
	LIST	*patterns;
	LIST	*results;
} ;

static void
builtin_glob_back(
	void	*closure,
	char	*file,
	int	status,
	time_t	time )
{
	struct globbing *globbing = (struct globbing *)closure;
	LIST		*l;
	PATHNAME	f;
	char		buf[ MAXJPATH ];

	/* Null out directory for matching. */
	/* We wish we had file_dirscan() pass up a PATHNAME. */

	path_parse( file, &f );
	f.f_dir.len = 0;
	path_build( &f, buf, 0 );

	for( l = globbing->patterns; l; l = l->next )
	    if( !glob( l->string, buf ) )
	{
	    globbing->results = list_new( globbing->results, newstr( file ) );
	    break;
	}
}

LIST *
builtin_glob(
	PARSE	*parse,
	LOL	*args )
{
	LIST *l = lol_get( args, 0 );
	LIST *r = lol_get( args, 1 );

	struct globbing globbing;

	globbing.results = L0;
	globbing.patterns = r;

	for( ; l; l = list_next( l ) )
	    file_dirscan( l->string, builtin_glob_back, &globbing );

	return globbing.results;
}

/*
 * builtin_match() - MATCH rule, regexp matching
 */

LIST *
builtin_match(
	PARSE	*parse,
	LOL	*args )
{
	LIST *l, *r;
	LIST *result = 0;

	/* For each pattern */

	for( l = lol_get( args, 0 ); l; l = l->next )
	{
	    regexp *re = regcomp( l->string );

	    /* For each string to match against */

	    for( r = lol_get( args, 1 ); r; r = r->next )
		if( regexec( re, r->string ) )
	    {
		int i, top;

		/* Find highest parameter */

		for( top = NSUBEXP; top-- > 1; )
		    if( re->startp[top] )
			break;

		/* And add all parameters up to highest onto list. */
		/* Must have parameters to have results! */

		for( i = 1; i <= top; i++ )
		{
		    char buf[ MAXSYM ];
		    int l = re->endp[i] - re->startp[i];
		    memcpy( buf, re->startp[i], l );
		    buf[ l ] = 0;
		    result = list_new( result, newstr( buf ) );
		}
	    }

	    free( (char *)re );
	}

	return result;
}
