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


static const int32 kNumColors = 32;

struct server_read_only_memory {
	rgb_color	colors[kNumColors];
};


// NOTE: these functions must be kept in sync with InterfaceDefs.h color_which!

static inline int32
color_which_to_index(color_which which)
{
	// NOTE: this must be kept in sync with InterfaceDefs.h color_which!
	if (which <= B_WINDOW_INACTIVE_BORDER_COLOR)
		return which - 1;
	if (which >= B_SUCCESS_COLOR && which <= B_FAILURE_COLOR)
		return which - B_SUCCESS_COLOR + B_WINDOW_INACTIVE_BORDER_COLOR;

	return -1;
}

static inline color_which
index_to_color_which(int32 index)
{
	if (index >= 0 && index < kNumColors) {
		if ((color_which)index < B_WINDOW_INACTIVE_BORDER_COLOR)
			return (color_which)(index + 1);
		else 
			return (color_which)(index + B_SUCCESS_COLOR - B_WINDOW_INACTIVE_BORDER_COLOR);
	}

	return (color_which)-1;
}

#endif	/* SERVER_READ_ONLY_MEMORY_H */
