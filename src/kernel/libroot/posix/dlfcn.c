/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <dlfcn.h>
#include <string.h>
#include "user_runtime.h"


void __init__dlfcn(struct uspace_program_args const *);

static struct rld_export const *gRuntimeLinker;
static status_t gStatus;
	// Note, this is not thread-safe


void *
dlopen(char const *name, int mode)
{
	if (name == NULL)
		name = MAGIC_APP_NAME;

	gStatus = gRuntimeLinker->load_add_on(name, mode);
	if (gStatus < B_OK)
		return NULL;

	return (void *)gStatus;
}


void *
dlsym(void *handle, char const *name)
{
	void *location;

	gStatus = gRuntimeLinker->get_image_symbol((image_id)handle, name, B_SYMBOL_TYPE_ANY, &location);
	if (gStatus < B_OK)
		return NULL;

	return location;
}


int
dlclose(void *handle)
{
	return gRuntimeLinker->unload_add_on((image_id)handle);
}


char *
dlerror(void)
{
	if (gStatus < B_OK)
		return strerror(gStatus);

	return NULL;
}


void
__init__dlfcn(struct uspace_program_args const *args)
{
	gRuntimeLinker = args->rld_export;
}
