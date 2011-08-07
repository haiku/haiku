/*
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"


// exported via the rld_export structure in user space program arguments


static image_id
export_load_add_on(char const *name, uint32 flags)
{
	void* handle;
	return load_library(name, flags, true, &handle);
}


static status_t
export_unload_add_on(image_id id)
{
	return unload_library(NULL, id, true);
}


static image_id
export_load_library(char const *name, uint32 flags, void **_handle)
{
	return load_library(name, flags, false, _handle);
}


static status_t
export_unload_library(void* handle)
{
	return unload_library(handle, -1, false);
}


struct rld_export gRuntimeLoader = {
	// dynamic loading support API
	export_load_add_on,
	export_unload_add_on,
	export_load_library,
	export_unload_library,
	get_symbol,
	get_library_symbol,
	get_nth_symbol,
	get_symbol_at_address,
	test_executable,
	get_next_image_dependency,

	elf_reinit_after_fork,
	NULL, // call_atexit_hooks_for_range
	terminate_program
};


void
rldexport_init(void)
{
	gRuntimeLoader.program_args = gProgramArgs;
}
