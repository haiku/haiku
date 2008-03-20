/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <libroot_private.h>
#include <user_runtime.h>

#include <dlfcn.h>
#include <string.h>


static status_t sStatus;
	// Note, this is not thread-safe


void *
dlopen(char const *name, int mode)
{
// TODO: According to the standard multiple dlopen() invocations for the same
// file will cause the object to be loaded once only. That is we should load
// the object as a library, not an add-on.
	status_t status;

	if (name == NULL)
		name = MAGIC_APP_NAME;

	status = __gRuntimeLoader->load_add_on(name, mode);
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

	status = get_image_symbol((image_id)handle, name, B_SYMBOL_TYPE_ANY, &location);
	sStatus = status;

	if (status < B_OK)
		return NULL;

	return location;
}


int
dlclose(void *handle)
{
	return unload_add_on((image_id)handle);
}


char *
dlerror(void)
{
	if (sStatus < B_OK)
		return strerror(sStatus);

	return NULL;
}


// __libc_dl*** wrappers
// We use a mixed glibc / bsd libc, and glibc wants these
void *
__libc_dlopen(const char *name)
{
	return dlopen(name, 0);
}
 
 
void *
__libc_dlsym(void *handle, const char *name)
{
	return dlsym(handle, name);
}


void
__libc_dlclose(void *handle)
{
	dlclose(handle);
}
