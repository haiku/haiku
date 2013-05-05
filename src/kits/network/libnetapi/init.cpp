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
find_own_image()
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (((addr_t)info.text <= (addr_t)find_own_image
			&& (addr_t)info.text + (size_t)info.text_size > (addr_t)find_own_image)) {
			// found us
			__gNetAPIStart = (addr_t)min_c(info.text, info.data);
			__gNetAPIEnd = min_c((addr_t)info.text + info.text_size,
				(addr_t)info.data + info.data_size);
			break;
		}
	}
}


extern "C" void
initialize_before()
{
	// If in compatibility mode get our code address range.
	if (__gR5Compatibility)
		find_own_image();
}
