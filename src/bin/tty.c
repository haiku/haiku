/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *				Daniel Reinhold, danielre@users.sf.net
 */


#include <stdio.h>
#include <unistd.h>

#include <OS.h>


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
