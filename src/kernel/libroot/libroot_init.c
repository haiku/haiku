/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <libroot_private.h>
#include <user_runtime.h>
#include <fork.h>

#include <string.h>


void initialize_before(image_id imageID, struct uspace_program_args const *args);

char *__progname = NULL;

char _single_threaded = true;
	// determines if I/O locking needed; needed for BeOS compatibility

int __libc_argc;
char **__libc_argv;
thread_id __main_thread_id;
char **argv_save;
	// needed for BeOS compatibility - they are set in the startup code
	// (have a look at the glue/ directory)


void
initialize_before(image_id imageID, struct uspace_program_args const *args)
{
	char *programPath = args->argv[0];
	if (programPath) {
		if ((__progname = strrchr(programPath, '/')) == NULL)
			__progname = programPath;
		else
			__progname++;
	}

	__init_image(args);
	__init_dlfcn(args);
	__init_fork();
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

