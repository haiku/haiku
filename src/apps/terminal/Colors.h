/*
 * Copyright 2010-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COLORS_H
#define _COLORS_H


#include <InterfaceDefs.h>


struct color_scheme {
	const char* name;
	rgb_color text_fore_color;
	rgb_color text_back_color;
	rgb_color cursor_fore_color;
	rgb_color cursor_back_color;
	rgb_color select_fore_color;
	rgb_color select_back_color;
	bool operator==(const color_scheme& color);
};

extern color_scheme gCustomColorScheme;
extern const color_scheme* gPredefinedColorSchemes[];


#endif // _COLORS_H
