/*
 * Copyright 2009-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_IMAGE_DEFS_H
#define _SYSTEM_IMAGE_DEFS_H


#include <SupportDefs.h>
#include <image.h>


#define B_SHARED_OBJECT_HAIKU_VERSION_VARIABLE		_gSharedObjectHaikuVersion
#define B_SHARED_OBJECT_HAIKU_VERSION_VARIABLE_NAME	"_gSharedObjectHaikuVersion"

#define B_SHARED_OBJECT_HAIKU_ABI_VARIABLE			_gSharedObjectHaikuABI
#define B_SHARED_OBJECT_HAIKU_ABI_VARIABLE_NAME		"_gSharedObjectHaikuABI"


typedef struct extended_image_info {
	image_info	basic_info;
	ssize_t		text_delta;
	void*		symbol_table;
	void*		symbol_hash;
	void*		string_table;
} extended_image_info;


#endif	/* _SYSTEM_IMAGE_DEFS_H */
