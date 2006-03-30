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


static status_t
export_get_image_symbol(image_id id, char const *symbolName, int32 symbolType, void **_location)
{
	return get_symbol(id, symbolName, symbolType, _location);
}


static status_t
export_get_nth_image_symbol(image_id id, int32 num, char *nameBuffer, int32 *_nameLength,
	int32 *_symbolType, void **_location)
{
	return get_nth_symbol(id, num, nameBuffer, _nameLength, _symbolType, _location);
}


static status_t
export_test_executable(const char *path, uid_t user, gid_t group, char *starter)
{
	return test_executable(path, user, group, starter);
}


struct rld_export gRuntimeLoader = {
	// dynamic loading support API
	export_load_add_on,
	export_unload_add_on,
	export_get_image_symbol,
	export_get_nth_image_symbol,
	export_test_executable
};


void
rldexport_init(void)
{
	gRuntimeLoader.program_args = gProgramArgs;
}
