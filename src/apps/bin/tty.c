// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        tty.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: prints the file name of the user's terminal
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <OS.h>
#include <stdio.h>
#include <unistd.h>


void
usage()
{
	printf("Usage: tty [-s]\n"
	       "Prints the file name for the terminal connected to standard input.\n"
	       "  -s   silent mode: no output -- only return an exit status\n");

	exit(2);
}


int
main(int argc, char *argv[])
{
	
	if (argc > 2)
		usage();
	
	else {
		bool silent = false;
		
		if (argc == 2) {
			if (!strcmp(argv[1], "-s"))
				silent = true;
			else
				usage();
		}
		
		if (!silent)
			printf("%s\n", ttyname(STDIN_FILENO));
	}
	
	return (isatty(STDIN_FILENO) ? 0 : 1);
}
