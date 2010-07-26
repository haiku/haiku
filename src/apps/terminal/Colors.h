/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COLORS_H
#define _COLORS_H

#include <InterfaceDefs.h>

struct color_schema {
	const char* name;
	rgb_color text_fore_color;
	rgb_color text_back_color;
	rgb_color cursor_fore_color;
	rgb_color cursor_back_color;
	rgb_color select_fore_color;
	rgb_color select_back_color;
	bool operator==(const color_schema& color);
};


extern color_schema gCustomSchema;
extern const color_schema* gPredefinedSchemas[];


#endif // _COLORS_H
