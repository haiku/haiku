/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <user_runtime.h>

#include <string.h>
#include <stdlib.h>


extern int main(int argc, char **argv, char **env);

int _start(int argc, char **argv, char **env, struct uspace_program_args *args);

// these are part of libroot.so, and initialized here
extern char **argv_save;
extern thread_id __main_thread_id;
extern char **environ;


/* The argument list is redundant, but that is for keeping BeOS compatibility.
 * BeOS doesn't have the last pointer, though.
 */

int
_start(int argc, char **argv, char **_environ, struct uspace_program_args *args)
{
	int returnCode;

	argv_save = args->argv;
	__main_thread_id = find_thread(NULL);

	returnCode = main(args->argc, args->argv, args->envp);

	exit(returnCode);
	return 0;
}

