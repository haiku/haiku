/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "images.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syscalls.h>
#include <vm_defs.h>

#include "add_ons.h"
#include "runtime_loader_private.h"


#define RLD_PROGRAM_BASE 0x00200000
	/* keep in sync with app ldscript */


bool gInvalidImageIDs;

static image_queue_t sLoadedImages = {0, 0};
static image_queue_t sDisposableImages = {0, 0};
static uint32 sLoadedImageCount = 0;


//! Remaps the image ID of \a image after fork.
static status_t
update_image_id(image_t* image)
{
	int32 cookie = 0;
	image_info info;
	while (_kern_get_next_image_info(B_CURRENT_TEAM, &cookie, &info,
			sizeof(image_info)) == B_OK) {
		for (uint32 i = 0; i < image->num_regions; i++) {
			if (image->regions[i].vmstart == (addr_t)info.text) {
				image->id = info.id;
				return B_OK;
			}
		}
	}

	FATAL("Could not update image ID %ld after fork()!\n", image->id);
	return B_ENTRY_NOT_FOUND;
}


static void
enqueue_image(image_queue_t* queue, image_t* image)
{
	image->next = NULL;

	image->prev = queue->tail;
	if (queue->tail)
		queue->tail->next = image;

	queue->tail = image;
	if (!queue->head)
		queue->head = image;
}


static void
dequeue_image(image_queue_t* queue, image_t* image)
{
	if (image->next)
		image->next->prev = image->prev;
	else
		queue->tail = image->prev;

	if (image->prev)
		image->prev->next = image->next;
	else
		queue->head = image->next;

	image->prev = NULL;
	image->next = NULL;
}


static image_t*
find_image_in_queue(image_queue_t* queue, const char* name, bool isPath,
	uint32 typeMask)
{
	for (image_t* image = queue->head; image; image = image->next) {
		const char* imageName = isPath ? image->path : image->name;
		int length = isPath ? sizeof(image->path) : sizeof(image->name);

		if (!strncmp(imageName, name, length)
			&& (typeMask & IMAGE_TYPE_TO_MASK(image->type)) != 0) {
			return image;
		}
	}

	return NULL;
}


static void
update_image_flags_recursively(image_t* image, uint32 flagsToSet,
	uint32 flagsToClear)
{
	image_t* queue[sLoadedImageCount];
	uint32 count = 0;
	uint32 index = 0;
	queue[count++] = image;
	image->flags |= RFLAG_VISITED;

	while (index < count) {
		// pop next image
		image = queue[index++];

		// push dependencies
		for (uint32 i = 0; i < image->num_needed; i++) {
			image_t* needed = image->needed[i];
			if ((needed->flags & RFLAG_VISITED) == 0) {
				queue[count++] = needed;
				needed->flags |= RFLAG_VISITED;
			}
		}
	}

	// update flags
	for (uint32 i = 0; i < count; i++) {
		queue[i]->flags = (queue[i]->flags | flagsToSet)
			& ~(flagsToClear | RFLAG_VISITED);
	}
}


static uint32
topological_sort(image_t* image, uint32 slot, image_t** initList,
	uint32 sortFlag)
{
	uint32 i;

	if (image->flags & sortFlag)
		return slot;

	image->flags |= sortFlag; /* make sure we don't visit this one */
	for (i = 0; i < image->num_needed; i++)
		slot = topological_sort(image->needed[i], slot, initList, sortFlag);

	initList[slot] = image;
	return slot + 1;
}


// #pragma mark -


image_t*
create_image(const char* name, const char* path, int regionCount)
{
	size_t allocSize = sizeof(image_t)
		+ (regionCount - 1) * sizeof(elf_region_t);

	image_t* image = (image_t*)malloc(allocSize);
	if (image == NULL) {
		FATAL("no memory for image %s\n", path);
		return NULL;
	}

	memset(image, 0, allocSize);

	strlcpy(image->path, path, sizeof(image->path));

	// Make the last component of the supplied name the image name.
	// If present, DT_SONAME will replace this name.
	const char* lastSlash = strrchr(name, '/');
	if (lastSlash != NULL)
		strlcpy(image->name, lastSlash + 1, sizeof(image->name));
	else
		strlcpy(image->name, name, sizeof(image->name));

	image->ref_count = 1;
	image->num_regions = regionCount;

	return image;
}


void
delete_image_struct(image_t* image)
{
#ifdef DEBUG
	size_t size = sizeof(image_t)
		+ (image->num_regions - 1) * sizeof(elf_region_t);
	memset(image->needed, 0xa5, sizeof(image->needed[0]) * image->num_needed);
#endif
	free(image->needed);
	free(image->versions);

	while (RuntimeLoaderSymbolPatcher* patcher
			= image->defined_symbol_patchers) {
		image->defined_symbol_patchers = patcher->next;
		delete patcher;
	}
	while (RuntimeLoaderSymbolPatcher* patcher
			= image->undefined_symbol_patchers) {
		image->undefined_symbol_patchers = patcher->next;
		delete patcher;
	}

#ifdef DEBUG
	// overwrite images to make sure they aren't accidently reused anywhere
	memset(image, 0xa5, size);
#endif
	free(image);
}


void
delete_image(image_t* image)
{
	if (image == NULL)
		return;

	_kern_unregister_image(image->id);
		// registered in load_container()

	delete_image_struct(image);
}


void
put_image(image_t* image)
{
	// If all references to the image are gone, add it to the disposable list
	// and remove all dependencies

	if (atomic_add(&image->ref_count, -1) == 1) {
		size_t i;

		dequeue_image(&sLoadedImages, image);
		enqueue_image(&sDisposableImages, image);
		sLoadedImageCount--;

		for (i = 0; i < image->num_needed; i++)
			put_image(image->needed[i]);
	}
}


status_t
map_image(int fd, char const* path, image_t* image, bool fixed)
{
	// cut the file name from the path as base name for the created areas
	const char* baseName = strrchr(path, '/');
	if (baseName != NULL)
		baseName++;
	else
		baseName = path;

	for (uint32 i = 0; i < image->num_regions; i++) {
		char regionName[B_OS_NAME_LENGTH];
		addr_t loadAddress;
		uint32 addressSpecifier;

		// for BeOS compatibility: if we load an old BeOS executable, we
		// have to relocate it, if possible - we recognize it because the
		// vmstart is set to 0 (hopefully always)
		if (fixed && image->regions[i].vmstart == 0)
			fixed = false;

		snprintf(regionName, sizeof(regionName), "%s_seg%lu%s",
			baseName, i, (image->regions[i].flags & RFLAG_RW) ? "rw" : "ro");

		if (image->dynamic_ptr && !fixed) {
			// relocatable image... we can afford to place wherever
			if (i == 0) {
				// but only the first segment gets a free ride
				loadAddress = RLD_PROGRAM_BASE;
				addressSpecifier = B_BASE_ADDRESS;
			} else {
				loadAddress = image->regions[i].vmstart
					+ image->regions[i-1].delta;
				addressSpecifier = B_EXACT_ADDRESS;
			}
		} else {
			// not relocatable, put it where it asks or die trying
			loadAddress = image->regions[i].vmstart;
			addressSpecifier = B_EXACT_ADDRESS;
		}

		if (image->regions[i].flags & RFLAG_ANON) {
			image->regions[i].id = _kern_create_area(regionName,
				(void**)&loadAddress, addressSpecifier,
				image->regions[i].vmsize, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA);

			if (image->regions[i].id < 0)
				return image->regions[i].id;

			image->regions[i].delta = loadAddress - image->regions[i].vmstart;
			image->regions[i].vmstart = loadAddress;
		} else {
			image->regions[i].id = _kern_map_file(regionName,
				(void**)&loadAddress, addressSpecifier,
				image->regions[i].vmsize, B_READ_AREA | B_WRITE_AREA,
				REGION_PRIVATE_MAP, fd, PAGE_BASE(image->regions[i].fdstart));

			if (image->regions[i].id < 0)
				return image->regions[i].id;

			TRACE(("\"%s\" at %p, 0x%lx bytes (%s)\n", path,
				(void *)loadAddress, image->regions[i].vmsize,
				image->regions[i].flags & RFLAG_RW ? "rw" : "read-only"));

			image->regions[i].delta = loadAddress - image->regions[i].vmstart;
			image->regions[i].vmstart = loadAddress;

			// handle trailer bits in data segment
			if (image->regions[i].flags & RFLAG_RW) {
				addr_t startClearing;
				addr_t toClear;

				startClearing = image->regions[i].vmstart
					+ PAGE_OFFSET(image->regions[i].start)
					+ image->regions[i].size;
				toClear = image->regions[i].vmsize
					- PAGE_OFFSET(image->regions[i].start)
					- image->regions[i].size;

				TRACE(("cleared 0x%lx and the following 0x%lx bytes\n",
					startClearing, toClear));
				memset((void *)startClearing, 0, toClear);
			}
		}
	}

	if (image->dynamic_ptr)
		image->dynamic_ptr += image->regions[0].delta;

	return B_OK;
}


void
unmap_image(image_t* image)
{
	for (uint32 i = 0; i < image->num_regions; i++) {
		_kern_delete_area(image->regions[i].id);

		image->regions[i].id = -1;
	}
}


/*!	This function will change the protection of all read-only segments to really
	be read-only.
	The areas have to be read/write first, so that they can be relocated.
*/
void
remap_images()
{
	for (image_t* image = sLoadedImages.head; image != NULL;
			image = image->next) {
		for (uint32 i = 0; i < image->num_regions; i++) {
			if ((image->regions[i].flags & RFLAG_RW) == 0
				&& (image->regions[i].flags & RFLAG_REMAPPED) == 0) {
				// we only need to do this once, so we remember those we've already mapped
				if (_kern_set_area_protection(image->regions[i].id,
						B_READ_AREA | B_EXECUTE_AREA) == B_OK) {
					image->regions[i].flags |= RFLAG_REMAPPED;
				}
			}
		}
	}
}


void
register_image(image_t* image, int fd, const char* path)
{
	struct stat stat;
	image_info info;

	// TODO: set these correctly
	info.id = 0;
	info.type = image->type;
	info.sequence = 0;
	info.init_order = 0;
	info.init_routine = (void (*)())image->init_routine;
	info.term_routine = (void (*)())image->term_routine;

	if (_kern_read_stat(fd, NULL, false, &stat, sizeof(struct stat)) == B_OK) {
		info.device = stat.st_dev;
		info.node = stat.st_ino;
	} else {
		info.device = -1;
		info.node = -1;
	}

	strlcpy(info.name, path, sizeof(info.name));
	info.text = (void *)image->regions[0].vmstart;
	info.text_size = image->regions[0].vmsize;
	info.data = (void *)image->regions[1].vmstart;
	info.data_size = image->regions[1].vmsize;
	info.api_version = image->api_version;
	info.abi = image->abi;
	image->id = _kern_register_image(&info, sizeof(image_info));
}


//! After fork, we lazily rebuild the image IDs of all loaded images.
status_t
update_image_ids()
{
	for (image_t* image = sLoadedImages.head; image; image = image->next) {
		status_t status = update_image_id(image);
		if (status != B_OK)
			return status;
	}
	for (image_t* image = sDisposableImages.head; image; image = image->next) {
		status_t status = update_image_id(image);
		if (status != B_OK)
			return status;
	}

	gInvalidImageIDs = false;
	return B_OK;
}


image_queue_t&
get_loaded_images()
{
	return sLoadedImages;
}


image_queue_t&
get_disposable_images()
{
	return sDisposableImages;
}


uint32
count_loaded_images()
{
	return sLoadedImageCount;
}


void
enqueue_loaded_image(image_t* image)
{
	enqueue_image(&sLoadedImages, image);
	sLoadedImageCount++;
}


void
dequeue_loaded_image(image_t* image)
{
	dequeue_image(&sLoadedImages, image);
	sLoadedImageCount--;
}


void
dequeue_disposable_image(image_t* image)
{
	dequeue_image(&sDisposableImages, image);
}


image_t*
find_loaded_image_by_name(char const* name, uint32 typeMask)
{
	bool isPath = strchr(name, '/') != NULL;
	return find_image_in_queue(&sLoadedImages, name, isPath, typeMask);
}


image_t*
find_loaded_image_by_id(image_id id, bool ignoreDisposable)
{
	if (gInvalidImageIDs) {
		// After fork, we lazily rebuild the image IDs of all loaded images
		update_image_ids();
	}

	for (image_t* image = sLoadedImages.head; image; image = image->next) {
		if (image->id == id)
			return image;
	}

	if (ignoreDisposable)
		return NULL;

	for (image_t* image = sDisposableImages.head; image; image = image->next) {
		if (image->id == id)
			return image;
	}

	return NULL;
}


void
set_image_flags_recursively(image_t* image, uint32 flags)
{
	update_image_flags_recursively(image, flags, 0);
}


void
clear_image_flags_recursively(image_t* image, uint32 flags)
{
	update_image_flags_recursively(image, 0, flags);
}


ssize_t
get_sorted_image_list(image_t* image, image_t*** _list, uint32 sortFlag)
{
	image_t** list;

	list = (image_t**)malloc(sLoadedImageCount * sizeof(image_t*));
	if (list == NULL) {
		FATAL("memory shortage in get_sorted_image_list()");
		*_list = NULL;
		return B_NO_MEMORY;
	}

	memset(list, 0, sLoadedImageCount * sizeof(image_t*));

	*_list = list;
	return topological_sort(image, 0, list, sortFlag);
}
