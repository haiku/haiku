/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * parse.h - make and destroy parse trees as driven by the parser
 */

/*
 * parse tree node
 */

typedef struct _PARSE PARSE;

struct _PARSE {
	LIST	*(*func)( PARSE *p, LOL *args );
	PARSE	*left;
	PARSE	*right;
	PARSE	*third;
	char	*string;
	char	*string1;
	int	num;
	int	refs;
} ;

void 	parse_file( char *f );
void 	parse_save( PARSE *p );

PARSE * parse_make( 
	LIST 	*(*func)( PARSE *p, LOL *args ),
	PARSE	*left,
	PARSE	*right,
	PARSE	*third,
	char	*string,
	char	*string1,
	int	num );

void 	parse_refer( PARSE *p );
void 	parse_free( PARSE *p );
