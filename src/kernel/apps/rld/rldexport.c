/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include "rld_priv.h"


// exported via the rld_export structure in user space program arguments


static image_id
export_load_add_on(char const *name, uint32 flags)
{
	// ToDo: use load_add_on() here when it's implemented, or better, unify
	//		load_library() and load_add_on().
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


void 
rldexport_init(struct uspace_program_args *args)
{
	static struct rld_export exports = {
		// dynamic loading support API
		export_load_add_on,
		export_unload_add_on,
		export_get_image_symbol,
		export_get_nth_image_symbol
	};

	args->rld_export = &exports;
}
