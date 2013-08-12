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
		maxAscend(0.0f),
		maxDescend(0.0f),
		lineIndex(0),
		style()
	{
	}

	GlyphInfo(uint32 charCode, float x, float y, float advanceX,
			float maxAscend, float maxDescend, int lineIndex,
			const TextStyleRef& style)
		:
		charCode(charCode),
		x(x),
		y(y),
		advanceX(advanceX),
		maxAscend(maxAscend),
		maxDescend(maxDescend),
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
		maxAscend(other.maxAscend),
		maxDescend(other.maxDescend),
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
		maxAscend = other.maxAscend;
		maxDescend = other.maxDescend;
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
			&& maxAscend == other.maxAscend
			&& maxDescend == other.maxDescend
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

	float					maxAscend;
	float					maxDescend;

	int						lineIndex;

	TextStyleRef			style;
};


#endif // GLYPH_INFO_H
