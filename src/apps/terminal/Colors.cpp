/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include "Colors.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal colors schema"


const rgb_color kBlack= { 0, 0, 0, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };
const rgb_color kGreen = { 0, 255, 0, 255 };

const struct color_schema kBlackOnWhite = {
	B_TRANSLATE("Black on White"),
	kBlack,
	kWhite,
	kWhite,
	kBlack,
	kWhite,
	kBlack
};


const struct color_schema kWhiteOnBlack = {
	B_TRANSLATE("White on Black"),
	kWhite,
	kBlack,
	kBlack,
	kWhite,
	kBlack,
	kWhite
};

const struct color_schema kGreenOnBlack = {
	B_TRANSLATE("Green on Black"),
	kGreen,
	kBlack,
	kBlack,
	kGreen,
	kBlack,
	kGreen
};

struct color_schema gCustomSchema = {
	B_TRANSLATE("Custom")
};

const color_schema* gPredefinedSchemas[] = {
		&kBlackOnWhite,
		&kWhiteOnBlack,
		&kGreenOnBlack,
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
