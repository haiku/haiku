/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GLYPH_INFO_H
#define GLYPH_INFO_H

#include "TextStyle.h"


class GlyphInfo {
public:
	GlyphInfo()
		:
		charCode(0),
		x(0.0f),
		y(0.0f),
		advanceX(0.0f),
		maxAscent(0.0f),
		maxDescent(0.0f),
		lineIndex(0),
		style()
	{
	}

	GlyphInfo(uint32 charCode, float x, float y, float advanceX,
			float maxAscent, float maxDescent, int lineIndex,
			const TextStyleRef& style)
		:
		charCode(charCode),
		x(x),
		y(y),
		advanceX(advanceX),
		maxAscent(maxAscent),
		maxDescent(maxDescent),
		lineIndex(lineIndex),
		style(style)
	{
	}
	
	GlyphInfo(const GlyphInfo& other)
		:
		charCode(other.charCode),
		x(other.x),
		y(other.y),
		advanceX(other.advanceX),
		maxAscent(other.maxAscent),
		maxDescent(other.maxDescent),
		lineIndex(other.lineIndex),
		style(other.style)
	{
	}

	GlyphInfo& operator=(const GlyphInfo& other)
	{
		charCode = other.charCode;
		x = other.x;
		y = other.y;
		advanceX = other.advanceX;
		maxAscent = other.maxAscent;
		maxDescent = other.maxDescent;
		lineIndex = other.lineIndex;
		style = other.style;
		return *this;
	}

	bool operator==(const GlyphInfo& other) const
	{
		return charCode == other.charCode
			&& x == other.x
			&& y == other.y
			&& advanceX == other.advanceX
			&& maxAscent == other.maxAscent
			&& maxDescent == other.maxDescent
			&& lineIndex == other.lineIndex
			&& style == other.style;
	}

	bool operator!=(const GlyphInfo& other) const
	{
		return !(*this == other);
	}

public:
	uint32					charCode;

	float					x;
	float					y;
	float					advanceX;

	float					maxAscent;
	float					maxDescent;

	int						lineIndex;

	TextStyleRef			style;
};


#endif // GLYPH_INFO_H
