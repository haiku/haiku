/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <user_runtime.h>
#include <fork.h>
#include <image.h>

#include <string.h>


void initialize_before(image_id imageID);

struct rld_export *__gRuntimeLoader = NULL;
	// This little bugger is set to something meaningful by the runtime loader
	// Ugly, eh?

char *__progname = NULL;
int __libc_argc;
char **__libc_argv;

char _single_threaded = true;
	// determines if I/O locking needed; needed for BeOS compatibility

thread_id __main_thread_id;
char **argv_save;
	// needed for BeOS compatibility - they are set in the startup code
	// (have a look at the glue/ directory)

int _data_offset_main_;
	// this is obviously needed for R4.5 compatiblity


void
initialize_before(image_id imageID)
{
	char *programPath = __gRuntimeLoader->program_args->argv[0];
	if (programPath) {
		if ((__progname = strrchr(programPath, '/')) == NULL)
			__progname = programPath;
		else
			__progname++;
	}

	__libc_argc = __gRuntimeLoader->program_args->argc;
	__libc_argv = __gRuntimeLoader->program_args->argv;

	__init_time();
	__init_fork();
	__init_heap();
	__init_env(__gRuntimeLoader->program_args);
}


void _init_c_library_(void);
void
_init_c_library_(void)
{
	// This function is called from the BeOS start_dyn.o - so it's called once
	// for every application that was compiled under BeOS.
	// Our libroot functions are already initialized above, so we don't have to
	// do anything here.
}

