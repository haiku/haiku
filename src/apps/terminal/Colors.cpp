/*
 * Copyright 2010-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Siarzhuk Zharski, zharik@gmx.li
 */


#include "Colors.h"

#include <ctype.h>
#include <stdio.h>
#include <strings.h>

#include <Application.h>
#include <Catalog.h>
#include <Resources.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal colors scheme"


// Standard colors
const rgb_color kBlack = { 0, 0, 0, 255 };
const rgb_color kGreen = { 0, 255, 0, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };
const rgb_color kYellow = { 255, 255, 0, 255 };

const struct ansi_color_scheme kAnsiColorSchemeDefault = {
	{ 40, 40, 40, 255 },
	{ 204, 0, 0, 255 },
	{ 78, 154, 6, 255 },
	{ 218, 168, 0, 255 },
	{ 51, 102, 152, 255 },
	{ 115, 68, 123, 255 },
	{ 6, 152, 154, 255 },
	{ 245, 245, 245, 255 }
};

/* The 'bright' colors palette, relates to the existing
   PREF_ANSI_<color>_HCOLOR macros used elsewhere */
const struct ansi_color_scheme kAnsiColorHSchemeDefault = {
	{ 128, 128, 128, 255 },
	{ 255, 0, 0, 255 },
	{ 0, 255, 0, 255 },
	{ 255, 255, 0, 255 },
	{ 0, 0, 255, 255 },
	{ 255, 0, 255, 255 },
	{ 0, 255, 255, 255 },
	{ 255, 255, 255, 255 }
};

const struct color_scheme kColorSchemeDefault = {
	B_TRANSLATE("Default"),
	kBlack,
	kWhite,
	kWhite,
	kBlack,
	kWhite,
	kBlack,
	kAnsiColorSchemeDefault,
	kAnsiColorHSchemeDefault
};

const struct color_scheme kColorSchemeBlue = {
	B_TRANSLATE("Blue"),
	kYellow,
	{ 0, 0, 139, 255 },
	kBlack,
	kYellow,
	kBlack,
	{ 0, 139, 139, 255 },
	kAnsiColorSchemeDefault,
	kAnsiColorHSchemeDefault
};

const struct color_scheme kColorSchemeMidnight = {
	B_TRANSLATE("Midnight"),
	kWhite,
	kBlack,
	kBlack,
	kWhite,
	kBlack,
	kWhite,
	kAnsiColorSchemeDefault,
	kAnsiColorHSchemeDefault
};

const struct color_scheme kColorSchemeProfessional = {
	B_TRANSLATE("Professional"),
	kWhite,
	{ 8, 8, 8, 255 },
	{ 50, 50, 50, 255 },
	kWhite,
	kWhite,
	{ 50, 50, 50, 255 },
	kAnsiColorSchemeDefault,
	kAnsiColorHSchemeDefault
};

const struct color_scheme kColorSchemeRetro = {
	B_TRANSLATE("Retro"),
	kGreen,
	kBlack,
	kBlack,
	kGreen,
	kBlack,
	kGreen,
	kAnsiColorSchemeDefault,
	kAnsiColorHSchemeDefault
};

const struct color_scheme kColorSchemeSlate = {
	B_TRANSLATE("Slate"),
	kWhite,
	{ 20, 20, 28, 255 },
	{ 70, 70, 70, 255 },
	{ 255, 200, 0, 255 },
	kWhite,
	{ 70, 70, 70, 255 },
	kAnsiColorSchemeDefault,
	kAnsiColorHSchemeDefault
};

const struct color_scheme kColorSchemeSolarizedDark = {
	B_TRANSLATE("Solarized Dark"),
	{ 131, 148, 150, 255 },
	{ 0, 43, 54, 255 },
	{ 0, 46, 57, 255 },
	{ 120, 137, 139, 255 },
	{ 238, 232, 213, 255 },
	{ 88, 110, 117, 255 },
	{
		{ 7, 54, 66, 255 },
		{ 220, 50, 47, 255 },
		{ 133, 153, 0, 255 },
		{ 181, 137, 0, 255 },
		{ 38, 139, 210, 255 },
		{ 211, 54, 130, 255 },
		{ 42, 161, 152, 255 },
		{ 238, 232, 213, 255 }
	},
	{
		{ 0, 43, 54, 255 },
		{ 203, 75, 22, 255 },
		{ 88, 110, 117, 255 },
		{ 101, 123, 131, 255 },
		{ 131, 148, 150, 255 },
		{ 108, 113, 196, 255 },
		{ 147, 161, 161, 255 },
		{ 253, 246, 227, 255 }
	}
};

const struct color_scheme kColorSchemeSolarizedLight = {
	B_TRANSLATE("Solarized Light"),
	{ 101, 123, 131, 255 },
	{ 253, 246, 227, 255 },
	{ 236, 228, 207, 255 },
	{ 90, 112, 120, 255 },
	{ 147, 161, 161, 255 },
	{ 7, 54, 66, 255 },
	{
		{ 7, 54, 66, 255 },
		{ 220, 50, 47, 255 },
		{ 133, 153, 0, 255 },
		{ 181, 137, 0, 255 },
		{ 38, 139, 210, 255 },
		{ 211, 54, 130, 255 },
		{ 42, 161, 152, 255 },
		{ 238, 232, 213, 255 }
	},
	{
		{ 0, 43, 54, 255 },
		{ 203, 75, 22, 255 },
		{ 88, 110, 117, 255 },
		{ 101, 123, 131, 255 },
		{ 131, 148, 150, 255 },
		{ 108, 113, 196, 255 },
		{ 147, 161, 161, 255 },
		{ 253, 246, 227, 255 }
	}
};

const struct color_scheme kColorSchemeRelaxed = {
	B_TRANSLATE("Relaxed"),
	{ 217, 217, 217, 255 },
	{ 53, 58, 68, 255 },
	{ 27, 27, 27, 255 },
	{ 255, 203, 0, 255 },
	{ 53, 58, 68, 255 },
	{ 217, 217, 217, 255 },
	{
		{ 21, 21, 21, 255 },
		{ 188, 86, 83, 255 },
		{ 144, 157, 99, 255 },
		{ 235, 193, 122, 255 },
		{ 106, 135, 153, 255 },
		{ 176, 102, 152, 255 },
		{ 201, 223, 255, 255 },
		{ 217, 217, 217, 255 }
	},
	{
		{ 99, 99, 99, 255 },
		{ 188, 86, 83, 255 },
		{ 160, 172, 119, 255 },
		{ 235, 192, 122, 255 },
		{ 126, 170, 199, 255 },
		{ 176, 102, 152, 255 },
		{ 172, 187, 208, 255 },
		{ 247, 247, 247, 255 }
	}
};

struct color_scheme gCustomColorScheme = {
	B_TRANSLATE("Custom")
};

const color_scheme* gPredefinedColorSchemes[] = {
	&kColorSchemeDefault,
	&kColorSchemeBlue,
	&kColorSchemeMidnight,
	&kColorSchemeProfessional,
	&kColorSchemeRetro,
	&kColorSchemeSlate,
	&kColorSchemeSolarizedDark,
	&kColorSchemeSolarizedLight,
	&kColorSchemeRelaxed,
	&gCustomColorScheme,
	NULL
};


bool
ansi_color_scheme::operator==(const ansi_color_scheme& scheme)
{
	return black == scheme.black
		&& red == scheme.red
		&& green == scheme.green
		&& yellow == scheme.yellow
		&& blue == scheme.blue
		&& magenta == scheme.magenta
		&& cyan == scheme.cyan
		&& white == scheme.white;
}


bool
color_scheme::operator==(const color_scheme& scheme)
{
	return text_fore_color == scheme.text_fore_color
		&& text_back_color == scheme.text_back_color
		&& cursor_fore_color == scheme.cursor_fore_color
		&& cursor_back_color == scheme.cursor_back_color
		&& select_fore_color == scheme.select_fore_color
		&& select_back_color == scheme.select_back_color
		&& ansi_colors == scheme.ansi_colors
		&& ansi_colors_h == scheme.ansi_colors_h;
}

// #pragma mark XColorsTable implementation

XColorsTable gXColorsTable;


XColorsTable::XColorsTable()
		:
		fTable(NULL),
		fCount(0)
{
}


XColorsTable::~XColorsTable()
{
	// fTable as result of LoadResource call and owned
	// by BApplication so no need to free it explicitly
	fTable = NULL;
	fCount = 0;
}


status_t
XColorsTable::LookUpColor(const char* name, rgb_color* color)
{
	if (name == NULL || color == NULL)
		return B_BAD_DATA;

	// first check for 'rgb:xxx/xxx/xxx'-encoded color names
	const char magic[5] = "rgb:";
	if (strncasecmp(name, magic, sizeof(magic) - 1) == 0) {
		int r = 0, g = 0, b = 0;
		if (sscanf(&name[4], "%x/%x/%x", &r, &g, &b) == 3) {
			color->set_to(0xFF & r, 0xFF & g, 0xFF & b);
			return B_OK;
		}
		// else - let the chance lookup in rgb.txt table. Is it reasonable??
	}

	// for non-'rgb:xxx/xxx/xxx'-encoded - search the X11 rgb table
	if (fTable == NULL) {
		status_t result = _LoadXColorsTable();
		if (result != B_OK)
			return result;
	}

	// use binary search to lookup color name hash in table
	int left  = -1;
	int right = fCount;
	uint32 hash = _HashName(name);
	while ((right - left) > 1) {
		int i = (left + right) / 2;
		((fTable[i].hash < hash) ? left : right) = i;
	}

	if (fTable[right].hash == hash) {
		*color = fTable[right].color;
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}


status_t
XColorsTable::_LoadXColorsTable()
{
	BResources* res = BApplication::AppResources();
	if (res == NULL)
		return B_ERROR;

	size_t size = 0;
	fTable = (_XColorEntry*)res->LoadResource(B_RAW_TYPE, "XColorsTable", &size);
	if (fTable == NULL || size < sizeof(_XColorEntry)) {
		fTable = NULL;
		return B_ENTRY_NOT_FOUND;
	}

	fCount = size / sizeof(_XColorEntry);
	return B_OK;
}


uint32
XColorsTable::_HashName(const char* name)
{
	uint32 hash = 0;
	uint32 g = 0;

	// Using modified P.J.Weinberger hash algorithm
	// Spaces are purged out, characters are upper-cased
	// 'E' replaced with 'A' (to join 'grey' and 'gray' variations)
	for (const char* p = name; *p; p++) {
		int ch = toupper(*p);
		switch (ch) {
		case ' ':
			break;
		case 'E':
			ch = 'A';
		default:
			hash = (hash << 4) + (ch & 0xFF);
			g = hash & 0xf0000000;
			if (g != 0) {
				hash ^= g >> 24;
				hash ^= g;
			}
			break;
		}
	}

	return hash;
}
