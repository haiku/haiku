/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "runtime_loader_private.h"

#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <syscalls.h>
#include <util/kernel_cpp.h>

#include <locks.h>

#include "add_ons.h"
#include "elf_load_image.h"
#include "elf_symbol_lookup.h"
#include "elf_tls.h"
#include "elf_versioning.h"
#include "errors.h"
#include "images.h"


// TODO: implement better locking strategy
// TODO: implement lazy binding

// a handle returned by load_library() (dlopen())
#define RLD_GLOBAL_SCOPE	((void*)-2l)

static const char* const kLockName = "runtime loader";


typedef void (*init_term_function)(image_id);
typedef void (*initfini_array_function)();

bool gProgramLoaded = false;
image_t* gProgramImage;

static image_t** sPreloadedImages = NULL;
static uint32 sPreloadedImageCount = 0;

static recursive_lock sLock = RECURSIVE_LOCK_INITIALIZER(kLockName);


static const char *
find_dt_rpath(image_t *image)
{
	int i;
	elf_dyn *d = (elf_dyn *)image->dynamic_ptr;

	for (i = 0; d[i].d_tag != DT_NULL; i++) {
		if (d[i].d_tag == DT_RPATH)
			return STRING(image, d[i].d_un.d_val);
	}

	return NULL;
}


static status_t
load_immediate_dependencies(image_t *image)
{
	elf_dyn *d = (elf_dyn *)image->dynamic_ptr;
	bool reportErrors = report_errors();
	status_t status = B_OK;
	uint32 i, j;
	const char *rpath;

	if (!d || (image->flags & RFLAG_DEPENDENCIES_LOADED))
		return B_OK;

	image->flags |= RFLAG_DEPENDENCIES_LOADED;

	if (image->num_needed == 0)
		return B_OK;

	KTRACE("rld: load_dependencies(\"%s\", id: %" B_PRId32 ")", image->name,
		image->id);

	image->needed = (image_t**)malloc(image->num_needed * sizeof(image_t *));
	if (image->needed == NULL) {
		FATAL("%s: Failed to allocate needed struct\n", image->path);
		KTRACE("rld: load_dependencies(\"%s\", id: %" B_PRId32
			") failed: no memory", image->name, image->id);
		return B_NO_MEMORY;
	}

	memset(image->needed, 0, image->num_needed * sizeof(image_t *));
	rpath = find_dt_rpath(image);

	for (i = 0, j = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
			{
				int32 neededOffset = d[i].d_un.d_val;
				const char *name = STRING(image, neededOffset);

				status_t loadStatus = load_image(name, B_LIBRARY_IMAGE,
					rpath, image->path, &image->needed[j]);
				if (loadStatus < B_OK) {
					status = loadStatus;
					// correct error code in case the file could not been found
					if (status == B_ENTRY_NOT_FOUND) {
						status = B_MISSING_LIBRARY;

						if (reportErrors)
							gErrorMessage.AddString("missing library", name);
					}

					// Collect all missing libraries in case we report back
					if (!reportErrors) {
						KTRACE("rld: load_dependencies(\"%s\", id: %" B_PRId32
							") failed: %s", image->name, image->id,
							strerror(status));
						return status;
					}
				}

				j += 1;
				break;
			}

			default:
				// ignore any other tag
				continue;
		}
	}

	if (status < B_OK) {
		KTRACE("rld: load_dependencies(\"%s\", id: %" B_PRId32 ") "
			"failed: %s", image->name, image->id,
			strerror(status));
		return status;
	}

	if (j != image->num_needed) {
		FATAL("Internal error at load_dependencies()");
		KTRACE("rld: load_dependencies(\"%s\", id: %" B_PRId32 ") "
			"failed: internal error", image->name, image->id);
		return B_ERROR;
	}

	KTRACE("rld: load_dependencies(\"%s\", id: %" B_PRId32 ") done",
		image->name, image->id);

	return B_OK;
}


static status_t
load_dependencies(image_t* image)
{
	// load dependencies (breadth-first)
	for (image_t* otherImage = image; otherImage != NULL;
			otherImage = otherImage->next) {
		status_t status = load_immediate_dependencies(otherImage);
		if (status != B_OK)
			return status;
	}

	// Check the needed versions for the given image and all newly loaded
	// dependencies.
	for (image_t* otherImage = image; otherImage != NULL;
			otherImage = otherImage->next) {
		status_t status = check_needed_image_versions(otherImage);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


static status_t
relocate_image(image_t *rootImage, image_t *image)
{
	SymbolLookupCache cache(image);

	status_t status = arch_relocate_image(rootImage, image, &cache);
	if (status < B_OK) {
		FATAL("%s: Troubles relocating: %s\n", image->path, strerror(status));
		return status;
	}

	_kern_image_relocated(image->id);
	image_event(image, IMAGE_EVENT_RELOCATED);
	return B_OK;
}


static status_t
relocate_dependencies(image_t *image)
{
	// get the images that still have to be relocated
	image_t **list;
	ssize_t count = get_sorted_image_list(image, &list, RFLAG_RELOCATED);
	if (count < B_OK)
		return count;

	// relocate
	for (ssize_t i = 0; i < count; i++) {
		status_t status = relocate_image(image, list[i]);
		if (status < B_OK) {
			free(list);
			return status;
		}
	}

	free(list);
	return B_OK;
}


static void
init_dependencies(image_t *image, bool initHead)
{
	image_t **initList;
	ssize_t count, i;

	count = get_sorted_image_list(image, &initList, RFLAG_INITIALIZED);
	if (count <= 0)
		return;

	if (!initHead) {
		// this removes the "calling" image
		image->flags &= ~RFLAG_INITIALIZED;
		initList[--count] = NULL;
	}

	TRACE(("%ld: init dependencies\n", find_thread(NULL)));
	for (i = 0; i < count; i++) {
		image = initList[i];

		TRACE(("%ld:  init: %s\n", find_thread(NULL), image->name));

		if (image->preinit_array) {
			uint count_preinit = image->preinit_array_len / sizeof(addr_t);
			for (uint j = 0; j < count_preinit; j++)
				((initfini_array_function)image->preinit_array[j])();
		}

		init_term_function before;
		if (find_symbol(image,
				SymbolLookupInfo(B_INIT_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT),
				(void**)&before) == B_OK) {
			before(image->id);
		}

		if (image->init_routine != 0)
			((init_term_function)image->init_routine)(image->id);

		if (image->init_array) {
			uint count_init = image->init_array_len / sizeof(addr_t);
			for (uint j = 0; j < count_init; j++)
				((initfini_array_function)image->init_array[j])();
		}

		init_term_function after;
		if (find_symbol(image,
				SymbolLookupInfo(B_INIT_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT),
				(void**)&after) == B_OK) {
			after(image->id);
		}

		image_event(image, IMAGE_EVENT_INITIALIZED);
	}
	TRACE(("%ld: init done.\n", find_thread(NULL)));

	free(initList);
}


static void
inject_runtime_loader_api(image_t* rootImage)
{
	// We patch any exported __gRuntimeLoader symbols to point to our private
	// API.
	image_t* image;
	void* _export;
	if (find_symbol_breadth_first(rootImage,
			SymbolLookupInfo("__gRuntimeLoader", B_SYMBOL_TYPE_DATA), &image,
			&_export) == B_OK) {
		*(void**)_export = &gRuntimeLoader;
	}
}


static status_t
add_preloaded_image(image_t* image)
{
	// We realloc() everytime -- not particularly efficient, but good enough for
	// small number of preloaded images.
	image_t** newArray = (image_t**)realloc(sPreloadedImages,
		sizeof(image_t*) * (sPreloadedImageCount + 1));
	if (newArray == NULL)
		return B_NO_MEMORY;

	sPreloadedImages = newArray;
	newArray[sPreloadedImageCount++] = image;

	return B_OK;
}


image_id
preload_image(char const* path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	KTRACE("rld: preload_image(\"%s\")", path);

	image_t *image = NULL;
	status_t status = load_image(path, B_LIBRARY_IMAGE, NULL, NULL, &image);
	if (status < B_OK) {
		KTRACE("rld: preload_image(\"%s\") failed to load container: %s", path,
			strerror(status));
		return status;
	}

	if (image->find_undefined_symbol == NULL)
		image->find_undefined_symbol = find_undefined_symbol_global;

	status = load_dependencies(image);
	if (status < B_OK)
		goto err;

	set_image_flags_recursively(image, RTLD_GLOBAL);

	status = relocate_dependencies(image);
	if (status < B_OK)
		goto err;

	status = add_preloaded_image(image);
	if (status < B_OK)
		goto err;

	inject_runtime_loader_api(image);

	remap_images();
	init_dependencies(image, true);

	// if the image contains an add-on, register it
	runtime_loader_add_on* addOnStruct;
	if (find_symbol(image,
			SymbolLookupInfo("__gRuntimeLoaderAddOn", B_SYMBOL_TYPE_DATA),
			(void**)&addOnStruct) == B_OK) {
		add_add_on(image, addOnStruct);
	}

	KTRACE("rld: preload_image(\"%s\") done: id: %" B_PRId32, path, image->id);

	return image->id;

err:
	KTRACE("rld: preload_image(\"%s\") failed: %s", path, strerror(status));

	dequeue_loaded_image(image);
	delete_image(image);
	return status;
}


static void
preload_images()
{
	const char* imagePaths = getenv("LD_PRELOAD");
	if (imagePaths == NULL)
		return;

	while (*imagePaths != '\0') {
		// find begin of image path
		while (*imagePaths != '\0' && isspace(*imagePaths))
			imagePaths++;

		if (*imagePaths == '\0')
			break;

		// find end of image path
		const char* imagePath = imagePaths;
		while (*imagePaths != '\0' && !isspace(*imagePaths))
			imagePaths++;

		// extract the path
		char path[B_PATH_NAME_LENGTH];
		size_t pathLen = imagePaths - imagePath;
		if (pathLen > sizeof(path) - 1)
			continue;
		memcpy(path, imagePath, pathLen);
		path[pathLen] = '\0';

		// load the image
		preload_image(path);
	}
}


//	#pragma mark - libroot.so exported functions


image_id
load_program(char const *path, void **_entry)
{
	status_t status;
	image_t *image;

	KTRACE("rld: load_program(\"%s\")", path);

	RecursiveLocker _(sLock);
		// for now, just do stupid simple global locking

	preload_images();

	TRACE(("rld: load %s\n", path));

	status = load_image(path, B_APP_IMAGE, NULL, NULL, &gProgramImage);
	if (status < B_OK)
		goto err;

	if (gProgramImage->find_undefined_symbol == NULL)
		gProgramImage->find_undefined_symbol = find_undefined_symbol_global;

	status = load_dependencies(gProgramImage);
	if (status < B_OK)
		goto err;

	// Set RTLD_GLOBAL on all libraries including the program.
	// This results in the desired symbol resolution for dlopen()ed libraries.
	set_image_flags_recursively(gProgramImage, RTLD_GLOBAL);

	status = relocate_dependencies(gProgramImage);
	if (status < B_OK)
		goto err;

	inject_runtime_loader_api(gProgramImage);

	remap_images();
	init_dependencies(gProgramImage, true);

	// Since the images are initialized now, we no longer should use our
	// getenv(), but use the one from libroot.so
	find_symbol_breadth_first(gProgramImage,
		SymbolLookupInfo("getenv", B_SYMBOL_TYPE_TEXT), &image,
		(void**)&gGetEnv);

	if (gProgramImage->entry_point == 0) {
		status = B_NOT_AN_EXECUTABLE;
		goto err;
	}

	*_entry = (void *)(gProgramImage->entry_point);

	gProgramLoaded = true;

	KTRACE("rld: load_program(\"%s\") done: entry: %p, id: %" B_PRId32 , path,
		*_entry, gProgramImage->id);

	return gProgramImage->id;

err:
	KTRACE("rld: load_program(\"%s\") failed: %s", path, strerror(status));

	delete_image(gProgramImage);

	if (report_errors()) {
		// send error message
		gErrorMessage.AddInt32("error", status);
		gErrorMessage.SetDeliveryInfo(gProgramArgs->error_token,
			-1, 0, find_thread(NULL));

		_kern_write_port_etc(gProgramArgs->error_port, 'KMSG',
			gErrorMessage.Buffer(), gErrorMessage.ContentSize(), 0, 0);
	}
	_kern_loading_app_failed(status);

	return status;
}


image_id
load_library(char const *path, uint32 flags, bool addOn, void** _handle)
{
	image_t *image = NULL;
	image_type type = (addOn ? B_ADD_ON_IMAGE : B_LIBRARY_IMAGE);
	status_t status;

	if (path == NULL && addOn)
		return B_BAD_VALUE;

	KTRACE("rld: load_library(\"%s\", %#" B_PRIx32 ", %d)", path, flags, addOn);

	RecursiveLocker _(sLock);
		// for now, just do stupid simple global locking

	// have we already loaded this library?
	// Checking it at this stage saves loading its dependencies again
	if (!addOn) {
		// a NULL path is fine -- it means the global scope shall be opened
		if (path == NULL) {
			*_handle = RLD_GLOBAL_SCOPE;
			return 0;
		}

		image = find_loaded_image_by_name(path, APP_OR_LIBRARY_TYPE);
		if (image != NULL && (flags & RTLD_GLOBAL) != 0)
			set_image_flags_recursively(image, RTLD_GLOBAL);

		if (image) {
			atomic_add(&image->ref_count, 1);
			KTRACE("rld: load_library(\"%s\"): already loaded: %" B_PRId32,
				path, image->id);
			*_handle = image;
			return image->id;
		}
	}

	status = load_image(path, type, NULL, NULL, &image);
	if (status < B_OK) {
		KTRACE("rld: load_library(\"%s\") failed to load container: %s", path,
			strerror(status));
		return status;
	}

	if (image->find_undefined_symbol == NULL) {
		if (addOn)
			image->find_undefined_symbol = find_undefined_symbol_add_on;
		else
			image->find_undefined_symbol = find_undefined_symbol_global;
	}

	status = load_dependencies(image);
	if (status < B_OK)
		goto err;

	// If specified, set the RTLD_GLOBAL flag recursively on this image and all
	// dependencies. If not specified, we temporarily set
	// RFLAG_USE_FOR_RESOLVING so that the dependencies will correctly be used
	// for undefined symbol resolution.
	if ((flags & RTLD_GLOBAL) != 0)
		set_image_flags_recursively(image, RTLD_GLOBAL);
	else
		set_image_flags_recursively(image, RFLAG_USE_FOR_RESOLVING);

	status = relocate_dependencies(image);
	if (status < B_OK)
		goto err;

	if ((flags & RTLD_GLOBAL) == 0)
		clear_image_flags_recursively(image, RFLAG_USE_FOR_RESOLVING);

	remap_images();
	init_dependencies(image, true);

	KTRACE("rld: load_library(\"%s\") done: id: %" B_PRId32, path, image->id);

	*_handle = image;
	return image->id;

err:
	KTRACE("rld: load_library(\"%s\") failed: %s", path, strerror(status));

	dequeue_loaded_image(image);
	delete_image(image);
	return status;
}


status_t
unload_library(void* handle, image_id imageID, bool addOn)
{
	image_t *image;
	image_type type = addOn ? B_ADD_ON_IMAGE : B_LIBRARY_IMAGE;

	if (handle == NULL && imageID < 0)
		return B_BAD_IMAGE_ID;

	if (handle == RLD_GLOBAL_SCOPE)
		return B_OK;

	RecursiveLocker _(sLock);
		// for now, just do stupid simple global locking

	if (gInvalidImageIDs) {
		// After fork, we lazily rebuild the image IDs of all loaded images
		update_image_ids();
	}

	// we only check images that have been already initialized

	status_t status = B_BAD_IMAGE_ID;

	if (handle != NULL) {
		image = (image_t*)handle;
		put_image(image);
		status = B_OK;
	} else {
		image = find_loaded_image_by_id(imageID, true);
		if (image != NULL) {
			// unload image
			if (type == image->type) {
				put_image(image);
				status = B_OK;
			} else
				status = B_BAD_VALUE;
		}
	}

	if (status == B_OK) {
		while ((image = get_disposable_images().head) != NULL) {
			// Call the exit hooks that live in this image.
			// Note: With the Itanium ABI this shouldn't really be done this
			// way anymore, since global destructors are registered via
			// __cxa_atexit() (the ones that are registered dynamically) and the
			// termination routine should call __cxa_finalize() for the image.
			// The reason why we still do it is that hooks registered with
			// atexit() aren't associated with the image. We could find out
			// there which image the hooks lives in and register it
			// respectively, but since that would be done always, that's
			// probably more expensive than calling
			// call_atexit_hooks_for_range() only here, which happens only when
			// libraries are unloaded dynamically.
			if (gRuntimeLoader.call_atexit_hooks_for_range) {
				gRuntimeLoader.call_atexit_hooks_for_range(
					image->regions[0].vmstart, image->regions[0].vmsize);
			}

			image_event(image, IMAGE_EVENT_UNINITIALIZING);

			init_term_function before;
			if (find_symbol(image,
					SymbolLookupInfo(B_TERM_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT),
					(void**)&before) == B_OK) {
				before(image->id);
			}

			if (image->term_array) {
				uint count_term = image->term_array_len / sizeof(addr_t);
				for (uint i = count_term; i-- > 0;)
					((initfini_array_function)image->term_array[i])();
			}

			if (image->term_routine)
				((init_term_function)image->term_routine)(image->id);

			init_term_function after;
			if (find_symbol(image,
					SymbolLookupInfo(B_TERM_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT),
					(void**)&after) == B_OK) {
				after(image->id);
			}

			TLSBlockTemplates::Get().Unregister(image->dso_tls_id);

			dequeue_disposable_image(image);
			unmap_image(image);

			image_event(image, IMAGE_EVENT_UNLOADING);

			delete_image(image);
		}
	}

	return status;
}


status_t
get_nth_symbol(image_id imageID, int32 num, char *nameBuffer,
	int32 *_nameLength, int32 *_type, void **_location)
{
	int32 count = 0, j;
	uint32 i;
	image_t *image;

	RecursiveLocker _(sLock);

	// get the image from those who have been already initialized
	image = find_loaded_image_by_id(imageID, false);
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	// iterate through all the hash buckets until we've found the one
	for (i = 0; i < HASHTABSIZE(image); i++) {
		for (j = HASHBUCKETS(image)[i]; j != STN_UNDEF; j = HASHCHAINS(image)[j]) {
			elf_sym *symbol = &image->syms[j];

			if (count == num) {
				const char* symbolName = SYMNAME(image, symbol);
				strlcpy(nameBuffer, symbolName, *_nameLength);
				*_nameLength = strlen(symbolName);

				void* location = (void*)(symbol->st_value
					+ image->regions[0].delta);
				int32 type;
				if (symbol->Type() == STT_FUNC)
					type = B_SYMBOL_TYPE_TEXT;
				else if (symbol->Type() == STT_OBJECT)
					type = B_SYMBOL_TYPE_DATA;
				else
					type = B_SYMBOL_TYPE_ANY;
					// TODO: check with the return types of that BeOS function

				patch_defined_symbol(image, symbolName, &location, &type);

				if (_type != NULL)
					*_type = type;
				if (_location != NULL)
					*_location = location;
				goto out;
			}
			count++;
		}
	}
out:
	if (num != count)
		return B_BAD_INDEX;

	return B_OK;
}


status_t
get_nearest_symbol_at_address(void* address, image_id* _imageID,
	char** _imagePath, char** _imageName, char** _symbolName, int32* _type,
	void** _location, bool* _exactMatch)
{
	RecursiveLocker _(sLock);

	image_t* image = find_loaded_image_by_address((addr_t)address);
	if (image == NULL)
		return B_BAD_VALUE;

	bool exactMatch = false;
	elf_sym* foundSymbol = NULL;
	addr_t foundLocation = (addr_t)NULL;

	for (uint32 i = 0; i < HASHTABSIZE(image) && !exactMatch; i++) {
		for (int32 j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
				j = HASHCHAINS(image)[j]) {
			elf_sym *symbol = &image->syms[j];
			addr_t location = symbol->st_value + image->regions[0].delta;

			if (location <= (addr_t)address	&& location >= foundLocation) {
				foundSymbol = symbol;
				foundLocation = location;

				// jump out if we have an exact match
				if (location + symbol->st_size > (addr_t)address) {
					exactMatch = true;
					break;
				}
			}
		}
	}

	if (_imageID != NULL)
		*_imageID = image->id;
	if (_imagePath != NULL)
		*_imagePath = image->path;
	if (_imageName != NULL)
		*_imageName = image->name;
	if (_exactMatch != NULL)
		*_exactMatch = exactMatch;

	if (foundSymbol != NULL) {
		*_symbolName = SYMNAME(image, foundSymbol);

		if (_type != NULL) {
			if (foundSymbol->Type() == STT_FUNC)
				*_type = B_SYMBOL_TYPE_TEXT;
			else if (foundSymbol->Type() == STT_OBJECT)
				*_type = B_SYMBOL_TYPE_DATA;
			else
				*_type = B_SYMBOL_TYPE_ANY;
			// TODO: check with the return types of that BeOS function
		}

		if (_location != NULL)
			*_location = (void*)foundLocation;
	} else {
		*_symbolName = NULL;
		if (_location != NULL)
			*_location = NULL;
	}

	return B_OK;
}


status_t
get_symbol(image_id imageID, char const *symbolName, int32 symbolType,
	bool recursive, image_id *_inImage, void **_location)
{
	status_t status = B_OK;
	image_t *image;

	if (imageID < B_OK)
		return B_BAD_IMAGE_ID;
	if (symbolName == NULL)
		return B_BAD_VALUE;

	// Previously, these functions were called in __haiku_init_before
	// and __haiku_init_after. Now we call them inside runtime_loader,
	// so we prevent applications from fetching them.
	if (strcmp(symbolName, B_INIT_BEFORE_FUNCTION_NAME) == 0
		|| strcmp(symbolName, B_INIT_AFTER_FUNCTION_NAME) == 0
		|| strcmp(symbolName, B_TERM_BEFORE_FUNCTION_NAME) == 0
		|| strcmp(symbolName, B_TERM_AFTER_FUNCTION_NAME) == 0)
		return B_BAD_VALUE;

	RecursiveLocker _(sLock);
		// for now, just do stupid simple global locking

	// get the image from those who have been already initialized
	image = find_loaded_image_by_id(imageID, false);
	if (image != NULL) {
		if (recursive) {
			// breadth-first search in the given image and its dependencies
			status = find_symbol_breadth_first(image,
				SymbolLookupInfo(symbolName, symbolType, NULL,
					LOOKUP_FLAG_DEFAULT_VERSION),
				&image, _location);
		} else {
			status = find_symbol(image,
				SymbolLookupInfo(symbolName, symbolType, NULL,
					LOOKUP_FLAG_DEFAULT_VERSION),
				_location);
		}

		if (status == B_OK && _inImage != NULL)
			*_inImage = image->id;
	} else
		status = B_BAD_IMAGE_ID;

	return status;
}


status_t
get_library_symbol(void* handle, void* caller, const char* symbolName,
	void **_location)
{
	status_t status = B_ENTRY_NOT_FOUND;

	if (symbolName == NULL)
		return B_BAD_VALUE;

	RecursiveLocker _(sLock);
		// for now, just do stupid simple global locking

	if (handle == RTLD_DEFAULT || handle == RLD_GLOBAL_SCOPE) {
		// look in the default scope
		image_t* image;
		elf_sym* symbol = find_undefined_symbol_global(gProgramImage,
			gProgramImage,
			SymbolLookupInfo(symbolName, B_SYMBOL_TYPE_ANY, NULL,
				LOOKUP_FLAG_DEFAULT_VERSION),
			&image);
		if (symbol != NULL) {
			*_location = (void*)(symbol->st_value + image->regions[0].delta);
			int32 symbolType = symbol->Type() == STT_FUNC
				? B_SYMBOL_TYPE_TEXT : B_SYMBOL_TYPE_DATA;
			patch_defined_symbol(image, symbolName, _location, &symbolType);
			status = B_OK;
		}
	} else if (handle == RTLD_NEXT) {
		// Look in the default scope, but also in the dependencies of the
		// calling image. Return the next after the caller symbol.

		// First of all, find the caller image.
		image_t* callerImage = get_loaded_images().head;
		for (; callerImage != NULL; callerImage = callerImage->next) {
			elf_region_t& text = callerImage->regions[0];
			if ((addr_t)caller >= text.vmstart
				&& (addr_t)caller < text.vmstart + text.vmsize) {
				// found the image
				break;
			}
		}

		if (callerImage != NULL) {
			// found the caller -- now search the global scope until we find
			// the next symbol
			bool hitCallerImage = false;
			set_image_flags_recursively(callerImage, RFLAG_USE_FOR_RESOLVING);

			elf_sym* candidateSymbol = NULL;
			image_t* candidateImage = NULL;

			image_t* image = get_loaded_images().head;
			for (; image != NULL; image = image->next) {
				// skip the caller image
				if (image == callerImage) {
					hitCallerImage = true;
					continue;
				}

				// skip all images up to the caller image; also skip add-on
				// images and those not marked above for resolution
				if (!hitCallerImage || image->type == B_ADD_ON_IMAGE
					|| (image->flags
						& (RTLD_GLOBAL | RFLAG_USE_FOR_RESOLVING)) == 0) {
					continue;
				}

				elf_sym *symbol = find_symbol(image,
					SymbolLookupInfo(symbolName, B_SYMBOL_TYPE_TEXT, NULL,
						LOOKUP_FLAG_DEFAULT_VERSION));
				if (symbol == NULL)
					continue;

				// found a symbol
				bool isWeak = symbol->Bind() == STB_WEAK;
				if (candidateImage == NULL || !isWeak) {
					candidateSymbol = symbol;
					candidateImage = image;

					if (!isWeak)
						break;
				}

				// symbol is weak, so we need to continue
			}

			if (candidateSymbol != NULL) {
				// found the symbol
				*_location = (void*)(candidateSymbol->st_value
					+ candidateImage->regions[0].delta);
				int32 symbolType = B_SYMBOL_TYPE_TEXT;
				patch_defined_symbol(candidateImage, symbolName, _location,
					&symbolType);
				status = B_OK;
			}

			clear_image_flags_recursively(callerImage, RFLAG_USE_FOR_RESOLVING);
		}
	} else {
		// breadth-first search in the given image and its dependencies
		image_t* inImage;
		status = find_symbol_breadth_first((image_t*)handle,
			SymbolLookupInfo(symbolName, B_SYMBOL_TYPE_ANY, NULL,
				LOOKUP_FLAG_DEFAULT_VERSION),
			&inImage, _location);
	}

	return status;
}


status_t
get_next_image_dependency(image_id id, uint32 *cookie, const char **_name)
{
	uint32 i, j, searchIndex = *cookie;
	elf_dyn *dynamicSection;
	image_t *image;

	if (_name == NULL)
		return B_BAD_VALUE;

	RecursiveLocker _(sLock);

	image = find_loaded_image_by_id(id, false);
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	dynamicSection = (elf_dyn *)image->dynamic_ptr;
	if (dynamicSection == NULL || image->num_needed <= searchIndex)
		return B_ENTRY_NOT_FOUND;

	for (i = 0, j = 0; dynamicSection[i].d_tag != DT_NULL; i++) {
		if (dynamicSection[i].d_tag != DT_NEEDED)
			continue;

		if (j++ == searchIndex) {
			int32 neededOffset = dynamicSection[i].d_un.d_val;

			*_name = STRING(image, neededOffset);
			*cookie = searchIndex + 1;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


//	#pragma mark - runtime_loader private exports


/*! Read and verify the ELF header */
status_t
elf_verify_header(void *header, size_t length)
{
	int32 programSize, sectionSize;

	if (length < sizeof(elf_ehdr))
		return B_NOT_AN_EXECUTABLE;

	return parse_elf_header((elf_ehdr *)header, &programSize, &sectionSize);
}


#ifdef _COMPAT_MODE
#ifdef __x86_64__
status_t
elf32_verify_header(void *header, size_t length)
{
	int32 programSize, sectionSize;

	if (length < sizeof(Elf32_Ehdr))
		return B_NOT_AN_EXECUTABLE;

	return parse_elf32_header((Elf32_Ehdr *)header, &programSize, &sectionSize);
}
#else
status_t
elf64_verify_header(void *header, size_t length)
{
	int32 programSize, sectionSize;

	if (length < sizeof(Elf64_Ehdr))
		return B_NOT_AN_EXECUTABLE;

	return parse_elf64_header((Elf64_Ehdr *)header, &programSize, &sectionSize);
}
#endif	// __x86_64__
#endif	// _COMPAT_MODE


void
terminate_program(void)
{
	image_t **termList;
	ssize_t count, i;

	count = get_sorted_image_list(NULL, &termList, RFLAG_TERMINATED);
	if (count < B_OK)
		return;

	if (gInvalidImageIDs) {
		// After fork, we lazily rebuild the image IDs of all loaded images
		update_image_ids();
	}

	TRACE(("%ld: terminate dependencies\n", find_thread(NULL)));
	for (i = count; i-- > 0;) {
		image_t *image = termList[i];

		TRACE(("%ld:  term: %s\n", find_thread(NULL), image->name));

		image_event(image, IMAGE_EVENT_UNINITIALIZING);

		if (image->term_array) {
			uint count_term = image->term_array_len / sizeof(addr_t);
			for (uint j = count_term; j-- > 0;)
				((init_term_function)image->term_array[j])(image->id);
		}

		if (image->term_routine)
			((init_term_function)image->term_routine)(image->id);

		image_event(image, IMAGE_EVENT_UNLOADING);
	}
	TRACE(("%ld:  term done.\n", find_thread(NULL)));

	free(termList);
}


void
rldelf_init(void)
{
	init_add_ons();

	// create the debug area
	{
		size_t size = TO_PAGE_SIZE(sizeof(runtime_loader_debug_area));

		runtime_loader_debug_area *area;
		area_id areaID = _kern_create_area(RUNTIME_LOADER_DEBUG_AREA_NAME,
			(void **)&area, B_RANDOMIZED_ANY_ADDRESS, size, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
		if (areaID < B_OK) {
			FATAL("Failed to create debug area.\n");
			_kern_loading_app_failed(areaID);
		}

		area->loaded_images = &get_loaded_images();
	}

	// initialize error message if needed
	if (report_errors()) {
		void *buffer = malloc(1024);
		if (buffer == NULL)
			return;

		gErrorMessage.SetTo(buffer, 1024, 'Rler');
	}
}


status_t
elf_reinit_after_fork(void)
{
	recursive_lock_init(&sLock, kLockName);

	// We also need to update the IDs of our images. We are the child and
	// and have cloned images with different IDs. Since in most cases (fork()
	// + exec*()) this would just increase the fork() overhead with no one
	// caring, we do that lazily, when first doing something different.
	gInvalidImageIDs = true;

	return B_OK;
}
