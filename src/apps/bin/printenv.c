// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        printenv.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: prints environment variables
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char **environ;

void print_env(char *);


void
usage()
{
	printf("usage: printenv [VARIABLE]\n"
	       "If no environment VARIABLE is specified, print them all.\n");
}


int
main(int argc, char *argv[])
{
	if (argc > 2)
		usage();
	
	else {
		char *arg = (argc == 2 ? argv[1] : NULL);
		
		if (arg && !strcmp(arg, "--help"))
			usage();
		
		print_env(arg);
	}
	
	return 0;
}


void
print_env(char *arg)
{
	char **env = environ;
	
	if (arg) {
		// print only the value of the specified variable
		char *s;
		int   len = strlen(arg);
		
	    while ((s = *env++) != NULL)
	    	if (!strncmp(s, arg, len)) {
	    		char *p = strchr(s, '=');
	    		if (p) {
					printf("%s\n", p+1);
					break;
				}
			}
	}
	else {
		// print all environment 'key=value' pairs (one per line)
	    while (*env)
			printf("%s\n", *env++);
	}
}
