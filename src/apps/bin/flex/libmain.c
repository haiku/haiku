/* libmain - flex run-time support library "main" function */

/* $Header: /tmp/bonefish/open-beos/current/src/apps/bin/flex/libmain.c,v 1.1 2004/06/14 09:18:17 korli Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	while ( yylex() != 0 )
		;

	return 0;
	}
