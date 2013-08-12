/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextStyle.h"


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
