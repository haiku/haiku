/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef IMAGES_H
#define IMAGES_H

#include <runtime_loader.h>


enum {
	// the lower two bits are reserved for RTLD_NOW and RTLD_GLOBAL

	RFLAG_RW					= 0x0010,
	RFLAG_ANON					= 0x0020,

	RFLAG_TERMINATED			= 0x0200,
	RFLAG_INITIALIZED			= 0x0400,
	RFLAG_SYMBOLIC				= 0x0800,
	RFLAG_RELOCATED				= 0x1000,
	RFLAG_PROTECTED				= 0x2000,
	RFLAG_DEPENDENCIES_LOADED	= 0x4000,
	RFLAG_REMAPPED				= 0x8000,

	RFLAG_VISITED				= 0x10000,
	RFLAG_USE_FOR_RESOLVING		= 0x20000
		// temporarily set in the symbol resolution code
};


#define IMAGE_TYPE_TO_MASK(type)	(1 << ((type) - 1))
#define ALL_IMAGE_TYPES				(IMAGE_TYPE_TO_MASK(B_APP_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_LIBRARY_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_ADD_ON_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_SYSTEM_IMAGE))
#define APP_OR_LIBRARY_TYPE			(IMAGE_TYPE_TO_MASK(B_APP_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_LIBRARY_IMAGE))


extern bool gInvalidImageIDs;


image_t*	create_image(const char* name, const char* path, int regionCount);
void		delete_image_struct(image_t* image);
void		delete_image(image_t* image);
void		put_image(image_t* image);

status_t	map_image(int fd, char const* path, image_t* image, bool fixed);
void		unmap_image(image_t* image);
void		remap_images();

void		register_image(image_t* image, int fd, const char* path);
status_t	update_image_ids();

image_queue_t& get_loaded_images();
image_queue_t& get_disposable_images();
uint32		count_loaded_images();
void		enqueue_loaded_image(image_t* image);
void		dequeue_loaded_image(image_t* image);
void		dequeue_disposable_image(image_t* image);

image_t*	find_loaded_image_by_name(char const* name, uint32 typeMask);
image_t*	find_loaded_image_by_id(image_id id, bool ignoreDisposable);
image_t*	find_loaded_image_by_address(addr_t address);

void		set_image_flags_recursively(image_t* image, uint32 flags);
void		clear_image_flags_recursively(image_t* image, uint32 flags);
ssize_t		get_sorted_image_list(image_t* image, image_t*** _list,
				uint32 sortFlag);


#endif	// IMAGES_H
