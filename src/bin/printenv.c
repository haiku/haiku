// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//
//  File:        printenv.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: prints environment variables
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char **environ;

int print_env(char *);


int
main(int argc, char *argv[])
{
	char *arg = (argc == 2 ? argv[1] : NULL);
	
	if ((argc > 2) || (arg && !strcmp(arg, "--help"))) {
		printf("Usage: printenv [VARIABLE]\n"
		       "If no environment VARIABLE is specified, print them all.\n");
		return 1;
	}
	
	return print_env(arg);
}


int
print_env(char *arg)
{
	char **env = environ;
	
	if (arg == NULL) {
		// print all environment 'key=value' pairs (one per line)
	    while (*env)
			printf("%s\n", *env++);
		
		return 0;
	} else {
		// print only the value of the specified variable
		char *s;
		int   len   = strlen(arg);
		bool  found = false;
		
	    while ((s = *env++) != NULL)
	    	if (!strncmp(s, arg, len)) {
	    		char *p = strchr(s, '=');
	    		if (p) {
					printf("%s\n", p+1);
					found = true;
				}
			}
		
		return (found ? 0 : 1);
	}
}
