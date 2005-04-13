/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <libroot_private.h>
#include <user_runtime.h>

#include <dlfcn.h>
#include <string.h>


static struct rld_export const *sRuntimeLinker;
static status_t sStatus;
	// Note, this is not thread-safe


void *
dlopen(char const *name, int mode)
{
	status_t status;

	if (name == NULL)
		name = MAGIC_APP_NAME;

	status = sRuntimeLinker->load_add_on(name, mode);
	sStatus = status;

	if (status < B_OK)
		return NULL;

	return (void *)status;
}


void *
dlsym(void *handle, char const *name)
{
	status_t status;
	void *location;

	status = sRuntimeLinker->get_image_symbol((image_id)handle, name, B_SYMBOL_TYPE_ANY, &location);
	sStatus = status;

	if (status < B_OK)
		return NULL;

	return location;
}


int
dlclose(void *handle)
{
	return sRuntimeLinker->unload_add_on((image_id)handle);
}


char *
dlerror(void)
{
	if (sStatus < B_OK)
		return strerror(sStatus);

	return NULL;
}


void
__init_dlfcn(const struct uspace_program_args *args)
{
	sRuntimeLinker = args->rld_export;
}
