// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        error.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: displays error message text for OS error codes
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>


void print_error(char *);


int
main(int argc, char *argv[])
{
	if (argc != 2)
		printf("usage: error number\n"
		       "Displays the message text for OS error codes. "
		       "The error number can be in decimal, hex or octal.\n");
	else
		print_error(argv[1]);
	
	return 0;
}


void
print_error(char *str)
{
	uint32 err;
	int    base = 10;
	char  *end;
	
	if (str[0] == '0') {
		if (tolower(str[1]) == 'x')
			base = 16;
		else
			base = 8;
	}
	
	errno = 0;
	err = strtoul(str, &end, base);
	
	if (errno) {
		fprintf(stderr, "%s: %s\n", str, strerror(errno));
		exit(1);
	}
	
	if (*end) {
		fprintf(stderr, "%s: invalid number\n", str);
		exit(1);
	}
	
	printf("0x%x: %s\n", err, strerror(err));
}
