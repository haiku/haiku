/*
 * Copyright 2010-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Colors.h"

#include <Catalog.h>


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
