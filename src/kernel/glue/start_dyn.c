/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>
#include <syscalls.h>

#include <string.h>
#include <stdlib.h>


extern int main(int argc, char **argv);
extern void _thread_do_exit_notification(void);

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

	_thread_do_exit_notification();
		// ToDo: must also (only) be called in exit()!

	exit(retcode);
	return 0;
}

