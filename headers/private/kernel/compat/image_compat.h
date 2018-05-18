/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_IMAGE_H
#define _KERNEL_COMPAT_IMAGE_H


#include <image.h>


#define compat_uintptr_t uint32
typedef struct {
	image_id	id;
	image_type	type;
	int32		sequence;
	int32		init_order;
	compat_uintptr_t	init_routine;
	compat_uintptr_t	term_routine;
	dev_t		device;
	ino_t		node;
	char		name[MAXPATHLEN];
	compat_uintptr_t	text;
	compat_uintptr_t	data;
	int32		text_size;
	int32		data_size;

	/* Haiku R1 extensions */
	int32		api_version;	/* the Haiku API version used by the image */
	int32		abi;			/* the Haiku ABI used by the image */
} _PACKED compat_image_info;


typedef struct {
	compat_image_info	basic_info;
	int32				text_delta;
	compat_uintptr_t	symbol_table;
	compat_uintptr_t	symbol_hash;
	compat_uintptr_t	string_table;
} _PACKED compat_extended_image_info;


static_assert(sizeof(compat_image_info) == 1084,
	"size of compat_image_info mismatch");
static_assert(sizeof(compat_extended_image_info) == 1100,
	"size of compat_extended_image_info mismatch");


inline status_t
copy_ref_var_to_user(image_info &info, image_info* userInfo, size_t size)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		if (size > sizeof(compat_image_info))
			return B_BAD_VALUE;
		compat_image_info compat_info;
		compat_info.id = info.id;
		compat_info.type = info.type;
		compat_info.sequence = info.sequence;
		compat_info.init_order = info.init_order;
		compat_info.init_routine = (uint32)(addr_t)info.init_routine;
		compat_info.term_routine = (uint32)(addr_t)info.term_routine;
		compat_info.device = info.device;
		compat_info.node = info.node;
		strlcpy(compat_info.name, info.name, MAXPATHLEN);
		compat_info.text = (uint32)(addr_t)info.text;
		compat_info.data = (uint32)(addr_t)info.data;
		compat_info.text_size = info.text_size;
		compat_info.data_size = info.data_size;
		if (user_memcpy(userInfo, &compat_info, size) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (size > sizeof(image_info))
			return B_BAD_VALUE;

		if (user_memcpy(userInfo, &info, size) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(extended_image_info* userInfo,
	extended_image_info &info, size_t size)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_extended_image_info compat_info;
		if (size != sizeof(compat_info))
			return B_BAD_VALUE;
		if (user_memcpy(&compat_info, userInfo, size) < B_OK)
			return B_BAD_ADDRESS;
		info.basic_info.id = compat_info.basic_info.id;
		info.basic_info.type = compat_info.basic_info.type;
		info.basic_info.sequence = compat_info.basic_info.sequence;
		info.basic_info.init_order = compat_info.basic_info.init_order;
		info.basic_info.init_routine = (void(*)())(addr_t)compat_info.basic_info.init_routine;
		info.basic_info.term_routine = (void(*)())(addr_t)compat_info.basic_info.term_routine;
		info.basic_info.device = compat_info.basic_info.device;
		info.basic_info.node = compat_info.basic_info.node;
		strlcpy(info.basic_info.name, compat_info.basic_info.name, MAXPATHLEN);
		info.basic_info.text = (void*)(addr_t)compat_info.basic_info.text;
		info.basic_info.data = (void*)(addr_t)compat_info.basic_info.data;
		info.basic_info.text_size = compat_info.basic_info.text_size;
		info.basic_info.data_size = compat_info.basic_info.data_size;
		info.text_delta = compat_info.text_delta;
		info.symbol_table = (void*)(addr_t)compat_info.symbol_table;
		info.symbol_hash = (void*)(addr_t)compat_info.symbol_hash;
		info.string_table = (void*)(addr_t)compat_info.string_table;
	} else {
		if (size != sizeof(info))
			return B_BAD_VALUE;

		if (user_memcpy(&info, userInfo, size) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_IMAGE_H
