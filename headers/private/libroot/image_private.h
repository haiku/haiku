/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_IMAGE_PRIVATE_H
#define _LIBROOT_IMAGE_PRIVATE_H

#include <sys/cdefs.h>

#include <image.h>


__BEGIN_DECLS

status_t get_image_symbol_etc(image_id id, char const* symbolName,
	int32 symbolType, bool recursive, image_id* _inImage, void** _location);

__END_DECLS


#endif	// _LIBROOT_IMAGE_PRIVATE_H
