/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <user_runtime.h>

#include <string.h>
#include <stdlib.h>


extern int main(int argc, char **argv, char **env);
extern void _init_c_library_(void);
extern void _call_init_routines_(void);

int _start(int argc, char **argv, char **env);

// these are part of libroot.so, and initialized here
extern char **argv_save;
extern thread_id __main_thread_id;
extern char **environ;


int
_start(int argc, char **argv, char **environment)
{
	int returnCode;

	argv_save = argv;
	__main_thread_id = find_thread(NULL);

	// These two are called to make our glue code usable under BeOS R5
	// - in Haiku, they are both empty.
	_init_c_library_();
	_call_init_routines_();

	returnCode = main(argc, argv, environment);

	exit(returnCode);
	return 0;
}

