/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <user_runtime.h>
#include <string.h>


extern void __init__image(struct uspace_program_args const *args);
extern void __init__dlfcn(struct uspace_program_args const *args);

void initialize_before(image_id imageID, struct uspace_program_args const *args);

char *__progname = NULL;

thread_id __main_thread_id;
char **argv_save;
	// needed for BeOS compatibility - they get set in the original
	// BeOS startup code, but they won't be initialized when the
	// OpenBeOS startup code is used.


// ToDo: these functions are defined in libgcc - this one is included in
//		libroot.so in BeOS, but we probably don't want to do that.
//		Since we don't yet have libgcc at all, it's defined here for now
//		so that BeOS executables will find them.
//		Remove them later!


void __deregister_frame_info(void);
void
__deregister_frame_info(void)
{
}


void __register_frame_info(void);
void
__register_frame_info(void)
{
}


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

	__init__image(args);
	__init__dlfcn(args);
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

