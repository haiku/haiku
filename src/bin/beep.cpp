// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        beep.cpp
//  Author:      Mahmoud Al Gammal 
//  Description: BeOS' command line "beep" command
//  Created : Monday, September 23, 2002
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Beep.h>
#include <stdio.h>

int
main( int argc, char* argv[] )
{

	// "beep" can only take a single optional event name
	if (argc > 2) {
		fprintf(stdout,"usage: beep [ eventname ]\n");
		fprintf(stdout,"Event names are found in the Sounds preferences panel.\n");
		fflush(stdout);		
		return B_OK;
	}
	
	// if no event name is specified, play the default "Beep" event
	if (argc == 1) {
		return beep();
	}	else {
		return system_beep(argv[1]);
	}
}

// beep.c
