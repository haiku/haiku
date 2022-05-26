/*
 * Copyright 2010-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef _COLORS_H
#define _COLORS_H


#include <InterfaceDefs.h>


struct ansi_color_scheme {
	rgb_color black;
	rgb_color red;
	rgb_color green;
	rgb_color yellow;
	rgb_color blue;
	rgb_color magenta;
	rgb_color cyan;
	rgb_color white;
	bool operator==(const ansi_color_scheme& color);
};

struct color_scheme {
	const char* name;
	rgb_color text_fore_color;
	rgb_color text_back_color;
	rgb_color cursor_fore_color;
	rgb_color cursor_back_color;
	rgb_color select_fore_color;
	rgb_color select_back_color;
	ansi_color_scheme ansi_colors;
	ansi_color_scheme ansi_colors_h;
	bool operator==(const color_scheme& color);
};

extern color_scheme gCustomColorScheme;
extern const color_scheme* gPredefinedColorSchemes[];

const uint kANSIColorCount = 16;
const uint kTermColorCount = 256;


// Helper class handling XColorName/rgb:xxx/xxx/xxx -> rgb_color conversions.
// The source of XColorNames is wide available rgb.txt for X11 System.
// It is stored in "XColorsTable" application resource as array of
// "hash <-> rgb_color" pairs. The table is loaded only on demand.
// Name hashes must be sorted to let lookup procedure work properly
class XColorsTable {

	struct _XColorEntry {
		uint32		hash;
		rgb_color	color;
	};

public:
							XColorsTable();
							~XColorsTable();

	status_t				LookUpColor(const char* name, rgb_color* color);
private:
	status_t				_LoadXColorsTable();
	uint32					_HashName(const char* name);

	const _XColorEntry*		fTable;
	size_t					fCount;
};

extern XColorsTable gXColorsTable;

#endif // _COLORS_H
