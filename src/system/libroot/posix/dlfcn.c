/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <libroot_private.h>

#include <dlfcn.h>
#include <string.h>

#include <runtime_loader.h>
#include <user_runtime.h>


static status_t sStatus;
	// Note, this is not thread-safe


void *
dlopen(char const *name, int mode)
{
	void* handle;
	image_id imageID = __gRuntimeLoader->load_library(name, mode, &handle);

	sStatus = imageID >= 0 ? B_OK : imageID;

	return imageID >= 0 ? handle : NULL;
}


void *
dlsym(void *handle, char const *name)
{
	void* location;
	status_t status;
	void* caller = NULL;

	if (handle == RTLD_NEXT)
		caller = __arch_get_caller();

	status = __gRuntimeLoader->get_library_symbol(handle, caller, name,
		&location);
	sStatus = status;

	if (status < B_OK)
		return NULL;

	return location;
}


int
dlclose(void *handle)
{
	return sStatus = __gRuntimeLoader->unload_library(handle);
}


char *
dlerror(void)
{
	if (sStatus < B_OK)
		return strerror(sStatus);

	return NULL;
}


int
dladdr(void *address, Dl_info *info)
{
	static char sImageName[MAXPATHLEN];
	static char sSymbolName[NAME_MAX];

	image_id image;
	int32 nameLength = sizeof(sSymbolName);
	void* location;
	image_info imageInfo;
	sStatus = __gRuntimeLoader->get_symbol_at_address(address, &image,
		sSymbolName, &nameLength, NULL, &location);
	if (sStatus != B_OK)
		return 0;

	sStatus = get_image_info(image, &imageInfo);
	if (sStatus != B_OK)
		return 0;

	strlcpy(sImageName, imageInfo.name, MAXPATHLEN);
	info->dli_fname = sImageName;
	info->dli_fbase = imageInfo.text;
	info->dli_sname = sSymbolName;
	info->dli_saddr = location;

	return 1;
}


// __libc_dl*** wrappers
// We use a mixed glibc / bsd libc, and glibc wants these
void *__libc_dlopen(const char *name);
void *__libc_dlsym(void *handle, const char *name);
void __libc_dlclose(void *handle);

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
