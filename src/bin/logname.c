/*
 * Haiku Command line apps
 * logname.c
 * Larry Cow <larrycow@free.fr>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_USER "baron"

#define HELP_TIP "Try '%s --help' for more information.\n"

#define HELP_MESSAGE "Usage: /bin/logname [OPTION]... 
Print the name of the current    

  --help\t display this help and exit
  --version\t output version information and exit
  
Reports bugs to <larrycow@free.fr>."

#define VERSION_MESSAGE "logname (OBOS) 1.0
Written by Larry Cow

Coded by Larry Cow 2002
Released under the MIT license with Haiku."

void dispatch_args(char* av0, char* av1)
{
	if (!strcmp(av1, "--help"))
	{
		puts(HELP_MESSAGE);
		return;
	}
	if (!strcmp(av1, "--version"))
	{
		puts(VERSION_MESSAGE);
		return;
	}
	printf(HELP_TIP, av0);
}

int main(int argc, char* argv[])
{
	if (argc > 1)
		dispatch_args(argv[0], argv[1]);
	else
	{
		char* user = getenv("USER");
		if (user == NULL)
			puts(DEFAULT_USER);
		else
			puts(user);
	}
	return 0;
}
