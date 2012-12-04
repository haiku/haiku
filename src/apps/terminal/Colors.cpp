/*
 * Copyright 2010-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Colors.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal colors schema"


// Standard colors
const rgb_color kBlack= { 0, 0, 0, 255 };
const rgb_color kGreen = { 0, 255, 0, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };
const rgb_color kYellow = { 255, 255, 0, 255 };


const struct color_schema kColorDefault = {
	B_TRANSLATE("Default"),
	kBlack,
	kWhite,
	kWhite,
	kBlack,
	kWhite,
	kBlack
};

const struct color_schema kColorBlue = {
	B_TRANSLATE("Blue"),
	kYellow,
	{ 0, 0, 139, 255 },
	kBlack,
	kWhite,
	kBlack,
	{ 0, 139, 139, 255 },
};

const struct color_schema kColorMidnight = {
	B_TRANSLATE("Midnight"),
	kWhite,
	kBlack,
	kBlack,
	kWhite,
	kBlack,
	kWhite
};

const struct color_schema kColorProfessional = {
	B_TRANSLATE("Professional"),
	kWhite,
	{ 8, 8, 8, 255 },
	{ 50, 50, 50, 255 },
	kWhite,
	kWhite,
	{ 50, 50, 50, 255 },
};

const struct color_schema kColorRetroTerminal = {
	B_TRANSLATE("Retro Terminal"),
	kGreen,
	kBlack,
	kBlack,
	kGreen,
	kBlack,
	kGreen
};

const struct color_schema kColorSlate = {
	B_TRANSLATE("Slate"),
	kWhite,
	{ 20, 20, 28, 255 },
	{ 70, 70, 70, 255 },
	{ 255, 200, 0, 255 },
	kWhite,
	{ 70, 70, 70, 255 },
};

struct color_schema gCustomSchema = {
	B_TRANSLATE("Custom")
};

const color_schema* gPredefinedSchemas[] = {
		&kColorDefault,
		&kColorBlue,
		&kColorMidnight,
		&kColorProfessional,
		&kColorRetroTerminal,
		&kColorSlate,
		&gCustomSchema,
		NULL
};


bool
color_schema::operator==(const color_schema& schema)
{
	if (text_fore_color == schema.text_fore_color
		&& text_back_color == schema.text_back_color
		&& cursor_fore_color == schema.cursor_fore_color
		&& cursor_back_color == schema.cursor_back_color
		&& select_fore_color == schema.select_fore_color
		&& select_back_color == schema.select_back_color)
		return true;

	return false;
}
