/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <string.h>
#include <syscalls.h>
#include <user_runtime.h>
#include "rld_priv.h"


int
rldmain(void *_args)
{
	struct uspace_program_args *args = (struct uspace_program_args *)_args;
	void *entry = 0;

	rldheap_init();

	rldexport_init(args);
	rldelf_init(args);

	load_program(args->program_path, &entry);

	if (entry == NULL)
		return -1;

	// call the program entry point (usually _start())
	return ((int (*)(int, void *, void *, void *))entry)(args->argc, args->argv, args->envp, args);
}
