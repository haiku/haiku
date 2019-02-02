/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include <string.h>

#include <image.h>
#include <OS.h>

#include <r5_compatibility.h>


static void
find_own_image(image_id id)
{
	image_info info;
	if (get_image_info(id, &info) == B_OK) {
		__gNetAPIStart = (addr_t)min_c(info.text, info.data);
		__gNetAPIEnd = min_c((addr_t)info.text + info.text_size,
			(addr_t)info.data + info.data_size);
	}
}


extern "C" void
initialize_before(image_id id)
{
	// If in compatibility mode get our code address range.
	if (__gR5Compatibility)
		find_own_image(id);
}
