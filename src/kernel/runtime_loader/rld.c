/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <string.h>
#include <syscalls.h>
#include <user_runtime.h>
#include "rld_priv.h"


/** This is the main entry point of the runtime loader as
 *	specified by its ld-script.
 */

int
runtime_loader(void *_args)
{
	struct uspace_program_args *args = (struct uspace_program_args *)_args;
	void *entry = 0;

#if DEBUG_RLD
	close(0); open("/dev/console", 0); /* stdin   */
	close(1); open("/dev/console", 0); /* stdout  */
	close(2); open("/dev/console", 0); /* stderr  */
#endif

	rldheap_init();
	rldexport_init(args);
	rldelf_init(args);

	load_program(args->program_path, &entry);

	if (entry == NULL)
		return -1;

	// call the program entry point (usually _start())
	return ((int (*)(int, void *, void *, void *))entry)(args->argc, args->argv, args->envp, args);
}
