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

	if (handle == RTLD_NEXT) {
		caller = __builtin_return_address(0);
	}

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
dladdr(const void *address, Dl_info *info)
{
	image_id image;
	char* imagePath;
	char* symbolName;
	void* location;
	image_info imageInfo;

	sStatus = __gRuntimeLoader->get_nearest_symbol_at_address(address, &image,
		&imagePath, NULL, &symbolName, NULL, &location, NULL);
	if (sStatus != B_OK)
		return 0;

	sStatus = get_image_info(image, &imageInfo);
	if (sStatus != B_OK)
		return 0;

	info->dli_fname = imagePath;
	info->dli_fbase = imageInfo.text;
	info->dli_sname = symbolName;
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
