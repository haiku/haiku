/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include "Colors.h"

const rgb_color kBlack= { 0, 0, 0, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };

const struct color_schema kBlackOnWhite = {
	"Black on White",
	kBlack,
	kWhite,
	kWhite,
	kBlack,
	kWhite,
	kBlack
};


const struct color_schema kWhiteOnBlack = {
	"White on Black",
	kWhite,
	kBlack,
	kBlack,
	kWhite,
	kBlack,
	kWhite
};


struct color_schema gCustomSchema = {
	"Custom"
};

const color_schema *gPredefinedSchemas[] = {
		&kBlackOnWhite,
		&kWhiteOnBlack,
		&gCustomSchema,
};


bool
color_schema::operator==(const color_schema &schema)
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
