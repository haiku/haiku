/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>
#include <syscalls.h>

#include <string.h>
#include <stdlib.h>


extern int main(int argc, char **argv);

int _start(int argc, char **argv, char **, struct uspace_program_args *);

//static char empty[1];
//char *__progname = empty;

extern char **environ;


/* The argument list is redundant, but that is for keeping BeOS compatibility.
 * BeOS doesn't have the last pointer, though.
 */

int
_start(int argc, char **argv, char **_environ, struct uspace_program_args *args)
{
	int retcode;

	environ = args->envp;

	retcode = main(args->argc, args->argv);

	exit(retcode);
	return 0;
}

