/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * variable.h - handle jam multi-element variables
 */

void 	var_defines( char **e );
int 	var_string( char *in, char *out, int outsize, LOL *lol );
LIST * 	var_get( char *symbol );
void 	var_set( char *symbol, LIST *value, int flag );
LIST * 	var_swap( char *symbol, LIST *value );
void 	var_done();

/*
 * Defines for var_set().
 */

# define VAR_SET	0	/* override previous value */
# define VAR_APPEND	1	/* append to previous value */
# define VAR_DEFAULT	2	/* set only if no previous value */

