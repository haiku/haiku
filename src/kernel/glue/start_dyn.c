/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>
#include <syscalls.h>

#include <string.h>
#include <stdlib.h>


extern int main(int argc, char **argv);

int _start(int argc, char **argv, char **, struct uspace_program_args *);

// these are part of libroot.so, and initialized here
extern int __libc_argc;
extern char **__libc_argv;
extern char **argv_save;
extern thread_id __main_thread_id;

extern char **environ;


/* The argument list is redundant, but that is for keeping BeOS compatibility.
 * BeOS doesn't have the last pointer, though.
 */

int
_start(int argc, char **argv, char **_environ, struct uspace_program_args *args)
{
	int retcode;

	__libc_argc = args->argc;
	__libc_argv = argv_save = args->argv;
	__main_thread_id = find_thread(NULL);
	environ = args->envp;

	retcode = main(__libc_argc, __libc_argv);

	exit(retcode);
	return 0;
}

