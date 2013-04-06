/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_READ_ONLY_MEMORY_H
#define SERVER_READ_ONLY_MEMORY_H


#include <GraphicsDefs.h>
#include <InterfaceDefs.h>


struct server_read_only_memory {
	rgb_color	colors[B_COLOR_WHICH_COUNT];
};


static inline int32
color_which_to_index(color_which which)
{
	if (which <= B_COLOR_WHICH_COUNT - 3)
		return which - 1;
	if (which >= B_SUCCESS_COLOR && which <= B_FAILURE_COLOR)
		return which - B_SUCCESS_COLOR + B_COLOR_WHICH_COUNT - 3;

	return -1;
}


static inline color_which
index_to_color_which(int32 index)
{
	if (index >= 0 && index < B_COLOR_WHICH_COUNT) {
		if ((color_which)index < B_COLOR_WHICH_COUNT - 3)
			return (color_which)(index + 1);
		else {
			return (color_which)(index + B_SUCCESS_COLOR
				- B_COLOR_WHICH_COUNT - 3);
		}
	}

	return (color_which)-1;
}

#endif	/* SERVER_READ_ONLY_MEMORY_H */
