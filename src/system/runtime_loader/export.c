/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
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
	return load_library(name, flags, true);
}


static status_t
export_unload_add_on(image_id id)
{
	return unload_library(id, true);
}


struct rld_export gRuntimeLoader = {
	// dynamic loading support API
	export_load_add_on,
	export_unload_add_on,
	get_symbol,
	get_nth_symbol,
	test_executable,
	get_next_image_dependency
};


void
rldexport_init(void)
{
	gRuntimeLoader.program_args = gProgramArgs;
}
