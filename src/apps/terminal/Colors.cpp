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


const struct color_scheme kColorSchemeDefault = {
	B_TRANSLATE("Default"),
	kBlack,
	kWhite,
	kWhite,
	kBlack,
	kWhite,
	kBlack
};

const struct color_scheme kColorSchemeBlue = {
	B_TRANSLATE("Blue"),
	kYellow,
	{ 0, 0, 139, 255 },
	kBlack,
	kYellow,
	kBlack,
	{ 0, 139, 139, 255 },
};

const struct color_scheme kColorSchemeMidnight = {
	B_TRANSLATE("Midnight"),
	kWhite,
	kBlack,
	kBlack,
	kWhite,
	kBlack,
	kWhite
};

const struct color_scheme kColorSchemeProfessional = {
	B_TRANSLATE("Professional"),
	kWhite,
	{ 8, 8, 8, 255 },
	{ 50, 50, 50, 255 },
	kWhite,
	kWhite,
	{ 50, 50, 50, 255 },
};

const struct color_scheme kColorSchemeRetro = {
	B_TRANSLATE("Retro"),
	kGreen,
	kBlack,
	kBlack,
	kGreen,
	kBlack,
	kGreen
};

const struct color_scheme kColorSchemeSlate = {
	B_TRANSLATE("Slate"),
	kWhite,
	{ 20, 20, 28, 255 },
	{ 70, 70, 70, 255 },
	{ 255, 200, 0, 255 },
	kWhite,
	{ 70, 70, 70, 255 },
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
	&gCustomColorScheme,
	NULL
};


bool
color_scheme::operator==(const color_scheme& scheme)
{
	return text_fore_color == scheme.text_fore_color
		&& text_back_color == scheme.text_back_color
		&& cursor_fore_color == scheme.cursor_fore_color
		&& cursor_back_color == scheme.cursor_back_color
		&& select_fore_color == scheme.select_fore_color
		&& select_back_color == scheme.select_back_color;
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
		memcpy(color, &fTable[right].color, sizeof(rgb_color));
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
