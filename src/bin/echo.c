// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        echo.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: standard Unix echo command
//
//  Notes:
//  the only command line option is "-n" which, if present, must appear
//  before all the other command line args, and specifies to leave off
//  the normal end-of-line character
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <string.h>


int
main (int argc, char **argv)
{
	int eol = 1;
	
	if (argc > 1) {
		char *first = *++argv;
		
		if (strcmp (first, "-n") == 0)
			eol = 0;
		
		if (--argc) {
			if (eol)
				fprintf (stdout, "%s ", first);
			while (--argc)
				fprintf (stdout, "%s ", *++argv);
		}
	}
	
	if (eol)
		fputc ('\n', stdout);
	
	fflush (stdout);
	return 0;
}
