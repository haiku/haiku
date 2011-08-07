/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
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

#include <algorithm>

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
	if (image->flags & sortFlag)
		return slot;

	image->flags |= sortFlag; /* make sure we don't visit this one */
	for (uint32 i = 0; i < image->num_needed; i++)
		slot = topological_sort(image->needed[i], slot, initList, sortFlag);

	initList[slot] = image;
	return slot + 1;
}


/*!	Finds the load address and address specifier of the given image region.
*/
static void
get_image_region_load_address(image_t* image, uint32 index, int32 lastDelta,
	bool fixed, addr_t& loadAddress, uint32& addressSpecifier)
{
	if (image->dynamic_ptr != 0 && !fixed) {
		// relocatable image... we can afford to place wherever
		if (index == 0) {
			// but only the first segment gets a free ride
			loadAddress = RLD_PROGRAM_BASE;
			addressSpecifier = B_BASE_ADDRESS;
		} else {
			loadAddress = image->regions[index].vmstart + lastDelta;
			addressSpecifier = B_EXACT_ADDRESS;
		}
	} else {
		// not relocatable, put it where it asks or die trying
		loadAddress = image->regions[index].vmstart;
		addressSpecifier = B_EXACT_ADDRESS;
	}
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

	// determine how much space we need for all loaded segments

	addr_t reservedAddress = 0;
	addr_t loadAddress;
	size_t reservedSize = 0;
	size_t length = 0;
	uint32 addressSpecifier = B_ANY_ADDRESS;

	for (uint32 i = 0; i < image->num_regions; i++) {
		// for BeOS compatibility: if we load an old BeOS executable, we
		// have to relocate it, if possible - we recognize it because the
		// vmstart is set to 0 (hopefully always)
		if (fixed && image->regions[i].vmstart == 0)
			fixed = false;

		uint32 regionAddressSpecifier;
		get_image_region_load_address(image, i,
			i > 0 ? loadAddress - image->regions[i - 1].vmstart : 0,
			fixed, loadAddress, regionAddressSpecifier);
		if (i == 0) {
			reservedAddress = loadAddress;
			addressSpecifier = regionAddressSpecifier;
		}

		length += TO_PAGE_SIZE(image->regions[i].vmsize
			+ (loadAddress % B_PAGE_SIZE));

		size_t size = TO_PAGE_SIZE(loadAddress + image->regions[i].vmsize)
			- reservedAddress;
		if (size > reservedSize)
			reservedSize = size;
	}

	// Check whether the segments have an unreasonable amount of unused space
	// inbetween.
	if (reservedSize > length + 8 * 1024)
		return B_BAD_DATA;

	// reserve that space and allocate the areas from that one
	if (_kern_reserve_address_range(&reservedAddress, addressSpecifier,
			reservedSize) != B_OK)
		return B_NO_MEMORY;

	for (uint32 i = 0; i < image->num_regions; i++) {
		char regionName[B_OS_NAME_LENGTH];

		snprintf(regionName, sizeof(regionName), "%s_seg%lu%s",
			baseName, i, (image->regions[i].flags & RFLAG_RW) ? "rw" : "ro");

		get_image_region_load_address(image, i,
			i > 0 ? image->regions[i - 1].delta : 0, fixed, loadAddress,
			addressSpecifier);

		// If the image position is arbitrary, we must let it point to the start
		// of the reserved address range.
		if (addressSpecifier != B_EXACT_ADDRESS)
			loadAddress = reservedAddress;

		if ((image->regions[i].flags & RFLAG_ANON) != 0) {
			image->regions[i].id = _kern_create_area(regionName,
				(void**)&loadAddress, B_EXACT_ADDRESS,
				image->regions[i].vmsize, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA);

			if (image->regions[i].id < 0) {
				_kern_unreserve_address_range(reservedAddress, reservedSize);
				return image->regions[i].id;
			}
		} else {
			// Map all segments r/w first -- write access might be needed for
			// relocations. When we've done with those we change the protection
			// of read-only segments back to read-only. We map those segments
			// over-committing, since quite likely only a relatively small
			// number of pages needs to be touched and we want to avoid a lot
			// of memory to be committed for them temporarily, just because we
			// have to write map them.
			uint32 protection = B_READ_AREA | B_WRITE_AREA
				| ((image->regions[i].flags & RFLAG_RW) != 0
					? 0 : B_OVERCOMMITTING_AREA);
			image->regions[i].id = _kern_map_file(regionName,
				(void**)&loadAddress, B_EXACT_ADDRESS,
				image->regions[i].vmsize, protection, REGION_PRIVATE_MAP, false,
				fd, PAGE_BASE(image->regions[i].fdstart));

			if (image->regions[i].id < 0) {
				_kern_unreserve_address_range(reservedAddress, reservedSize);
				return image->regions[i].id;
			}

			TRACE(("\"%s\" at %p, 0x%lx bytes (%s)\n", path,
				(void *)loadAddress, image->regions[i].vmsize,
				image->regions[i].flags & RFLAG_RW ? "rw" : "read-only"));

			// handle trailer bits in data segment
			if (image->regions[i].flags & RFLAG_RW) {
				addr_t startClearing = loadAddress
					+ PAGE_OFFSET(image->regions[i].start)
					+ image->regions[i].size;
				addr_t toClear = image->regions[i].vmsize
					- PAGE_OFFSET(image->regions[i].start)
					- image->regions[i].size;

				TRACE(("cleared 0x%lx and the following 0x%lx bytes\n",
					startClearing, toClear));
				memset((void *)startClearing, 0, toClear);
			}
		}

		image->regions[i].delta = loadAddress - image->regions[i].vmstart;
		image->regions[i].vmstart = loadAddress;
	}

	if (image->dynamic_ptr != 0)
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

	// We may have split segments into separate regions. Compute the correct
	// segments for the image info.
	addr_t textBase = 0;
	addr_t textEnd = 0;
	addr_t dataBase = 0;
	addr_t dataEnd = 0;
	for (uint32 i= 0; i < image->num_regions; i++) {
		addr_t base = image->regions[i].vmstart;
		addr_t end = base + image->regions[i].vmsize;
		if (image->regions[i].flags & RFLAG_RW) {
			// data
			if (dataBase == 0) {
				dataBase = base;
				dataEnd = end;
			} else {
				dataBase = std::min(dataBase, base);
				dataEnd = std::max(dataEnd, end);
			}
		} else {
			// text
			if (textBase == 0) {
				textBase = base;
				textEnd = end;
			} else {
				textBase = std::min(textBase, base);
				textEnd = std::max(textEnd, end);
			}
		}
	}

	strlcpy(info.name, path, sizeof(info.name));
	info.text = (void*)textBase;
	info.text_size = textEnd - textBase;
	info.data = (void*)dataBase;
	info.data_size = dataEnd - dataBase;
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


image_t*
find_loaded_image_by_address(addr_t address)
{
	for (image_t* image = sLoadedImages.head; image; image = image->next) {
		for (uint32 i = 0; i < image->num_regions; i++) {
			elf_region_t& region = image->regions[i];
			if (region.vmstart <= address
				&& region.vmstart - 1 + region.vmsize >= address)
				return image;
		}
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


/*!	Returns a topologically sorted image list.

	If \a image is non-NULL, an array containing the image and all its
	transitive dependencies is returned. If \a image is NULL, all loaded images
	are returned. In either case dependencies are listed before images
	depending on them.

	\param image The image specifying the tree of images that shall be sorted.
		If NULL, all loaded images are sorted.
	\param _list On success it will be set to an array of the sorted images.
		The caller is responsible for free()ing it.
	\param sortFlags The image flag that shall be used for sorting. Images that
		already have this flag set are ignored (and their dependencies, unless
		they are also reachable via another path). The flag will be set on all
		returned images.
	\return The number of images in the returned array or an error code on
		failure.
*/
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

	if (image != NULL)
		return topological_sort(image, 0, list, sortFlag);

	// no image given -- sort all loaded images
	uint32 count = 0;
	image = sLoadedImages.head;
	while (image != NULL) {
		count = topological_sort(image, count, list, sortFlag);
		image = image->next;
	}

	return count;
}
