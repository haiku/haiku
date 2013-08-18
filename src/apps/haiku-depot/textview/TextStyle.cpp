/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextStyle.h"


TextStyle::TextStyle()
	:
	font(),

	ascent(0.0f),
	descent(0.0f),
	width(-1.0f),
	glyphSpacing(0.0f),

	fgColor((rgb_color){ 0, 0, 0, 255 }),
	bgColor((rgb_color){ 255, 255, 255, 255 }),

	strikeOut(false),
	strikeOutColor((rgb_color){ 0, 0, 0, 255 }),

	underline(false),
	underlineStyle(0),
	underlineColor((rgb_color){ 0, 0, 0, 255 })
{
}


TextStyle::TextStyle(const BFont& font, float ascent, float descent,
		float width, float glyphSpacing, rgb_color fgColor, rgb_color bgColor,
		bool strikeOut, rgb_color strikeOutColor, bool underline,
		uint32 underlineStyle, rgb_color underlineColor)
	:
	font(font),

	ascent(ascent),
	descent(descent),
	width(width),
	glyphSpacing(glyphSpacing),

	fgColor(fgColor),
	bgColor(bgColor),

	strikeOut(strikeOut),
	strikeOutColor(strikeOutColor),

	underline(underline),
	underlineStyle(underlineStyle),
	underlineColor(underlineColor)
{
}


TextStyle::TextStyle(const TextStyle& other)
	:
	font(other.font),

	ascent(other.ascent),
	descent(other.descent),
	width(other.width),
	glyphSpacing(other.glyphSpacing),

	fgColor(other.fgColor),
	bgColor(other.bgColor),

	strikeOut(other.strikeOut),
	strikeOutColor(other.strikeOutColor),

	underline(other.underline),
	underlineStyle(other.underlineStyle),
	underlineColor(other.underlineColor)
{
}


bool
TextStyle::operator==(const TextStyle& other) const
{
	return font == other.font

		&& ascent == other.ascent
		&& descent == other.descent
		&& width == other.width
		&& glyphSpacing == other.glyphSpacing

		&& fgColor == other.fgColor
		&& bgColor == other.bgColor
		
		&& strikeOut == other.strikeOut
		&& strikeOutColor == other.strikeOutColor
		
		&& underline == other.underline
		&& underlineStyle == other.underlineStyle
		&& underlineColor == other.underlineColor;
}
