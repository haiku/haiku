/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * compile.h - compile parsed jam statements
 */

void compile_builtins();

LIST *compile_append( PARSE *parse, LOL *args );
LIST *compile_foreach( PARSE *parse, LOL *args );
LIST *compile_if( PARSE *parse, LOL *args );
LIST *compile_eval( PARSE *parse, LOL *args );
LIST *compile_include( PARSE *parse, LOL *args );
LIST *compile_list( PARSE *parse, LOL *args );
LIST *compile_local( PARSE *parse, LOL *args );
LIST *compile_null( PARSE *parse, LOL *args );
LIST *compile_on( PARSE *parse, LOL *args );
LIST *compile_rule( PARSE *parse, LOL *args );
LIST *compile_rules( PARSE *parse, LOL *args );
LIST *compile_set( PARSE *parse, LOL *args );
LIST *compile_setcomp( PARSE *parse, LOL *args );
LIST *compile_setexec( PARSE *parse, LOL *args );
LIST *compile_settings( PARSE *parse, LOL *args );
LIST *compile_switch( PARSE *parse, LOL *args );
LIST *compile_while( PARSE *parse, LOL *args );

LIST *evaluate_rule( char *rulename, LOL *args, LIST *result );

/* Flags for compile_set(), etc */

# define ASSIGN_SET	0x00	/* = assign variable */
# define ASSIGN_APPEND	0x01	/* += append variable */
# define ASSIGN_DEFAULT	0x02	/* set only if unset */

/* Conditions for compile_if() */

# define EXPR_NOT	0	/* ! cond */
# define EXPR_AND	1	/* cond && cond */
# define EXPR_OR	2	/* cond || cond */

# define EXPR_EXISTS	3	/* arg */
# define EXPR_EQUALS	4	/* arg = arg */
# define EXPR_NOTEQ	5	/* arg != arg */
# define EXPR_LESS	6	/* arg < arg  */
# define EXPR_LESSEQ	7	/* arg <= arg */
# define EXPR_MORE	8	/* arg > arg  */
# define EXPR_MOREEQ	9	/* arg >= arg */
# define EXPR_IN	10	/* arg in arg */
