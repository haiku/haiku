/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <syscalls.h>
#include <user_runtime.h>
#include "rld_priv.h"



static struct rld_export_t exports=
{
	/*
	 * Unix like stuff
	 */
	export_dl_open,
	export_dl_close,
	export_dl_sym,


	/*
	 * BeOS like stuff
	 */
	export_load_addon,
	export_unload_addon,
	export_addon_symbol
};


int
rldmain(void *arg)
{
	int retval;
	unsigned long entry;
	struct uspace_prog_args_t *uspa= (struct uspace_prog_args_t *)arg;

	rldheap_init();

	entry= 0;

	uspa->rld_export= &exports;

	rldelf_init(uspa);

	load_program(uspa->prog_path, (void**)&entry);

	if(entry) {
		retval= ((int(*)(void*))(entry))(uspa);
	} else {
		return -1;
	}

	return retval;
}

