/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
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


int
dladdr(void *addr, Dl_info *info)
{
	char curSymName[NAME_MAX];
	static char symName[NAME_MAX];
	static char imageName[MAXPATHLEN];
	void *symLocation;
	int32 cookie;
	int32 symType, symNameLength;
	uint32 symIndex;
	image_info imageInfo;

	if (info == NULL)
		return 0;

	imageName[0] = '\0';
	symName[0] = '\0';
	info->dli_fname = imageName;
	info->dli_saddr = NULL;
	info->dli_sname = symName;

	cookie = 0;
	while (get_next_image_info(0, &cookie, &imageInfo) == B_OK) {
		// check if the image holds the symbol
		if ((addr_t)addr >= (addr_t)imageInfo.text
			&& (addr_t)addr < (addr_t)imageInfo.text + imageInfo.text_size)  {
			strlcpy(imageName, imageInfo.name, MAXPATHLEN);
			info->dli_fbase = imageInfo.text;
			symIndex = 0;
			symNameLength = NAME_MAX;

			while (get_nth_image_symbol(imageInfo.id, symIndex, curSymName,
					&symNameLength, &symType, &symLocation) == B_OK) {
				// check if symbol is the nearest until now
				if (symType == B_SYMBOL_TYPE_TEXT
					&& symLocation <= addr
					&& symLocation >= info->dli_saddr) {
					strlcpy(symName, curSymName, NAME_MAX);
					info->dli_saddr = symLocation;

					// stop here if exact match
					if (info->dli_saddr == addr)
						return 1;
				}
				symIndex++;
				symNameLength = NAME_MAX;
			}
			break;
		}
	}

	if (info->dli_saddr != NULL)
		return 1;

	return 0;
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
