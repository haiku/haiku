/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mahmoud Al Gammal
 */


#include <Beep.h>

#include <stdio.h>


int
main(int argc, char* argv[])
{
	// "beep" can only take a single optional event name
	if (argc > 2
		|| (argc == 2 && argv[1][0] == '-')) {
		fprintf(stdout, "usage: beep [ eventname ]\n");
		fprintf(stdout, "Event names are found in the "
			"Sounds preferences panel.\n");
		fflush(stdout);
		return B_OK;
	}

	// if no event name is specified, play the default "Beep" event
	if (argc == 1)
		return beep();
	else
		return system_beep(argv[1]);
}
