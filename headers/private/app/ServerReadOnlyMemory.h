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


static inline int32
color_which_to_index(color_which which)
{
	if (which <= B_TOOLTIP_TEXT_COLOR)
		return which - 1;
	if (which >= B_SUCCESS_COLOR && which <= B_FAILURE_COLOR)
		return which - B_SUCCESS_COLOR + B_TOOLTIP_TEXT_COLOR;

	return -1;
}

#endif	/* SERVER_READ_ONLY_MEMORY_H */
