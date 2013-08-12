/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LINE_INFO_H
#define LINE_INFO_H

#include "List.h"


class LineInfo {
public:
	LineInfo()
		:
		textOffset(0),
		y(0.0f),
		height(0.0f),
		maxAscent(0.0f),
		maxDescent(0.0f)
	{
	}
	
	LineInfo(int textOffset, float y, float height, float maxAscent,
			float maxDescent)
		:
		textOffset(textOffset),
		y(y),
		height(height),
		maxAscent(maxAscent),
		maxDescent(maxDescent)
	{
	}
	
	LineInfo(const LineInfo& other)
		:
		textOffset(other.textOffset),
		y(other.y),
		height(other.height),
		maxAscent(other.maxAscent),
		maxDescent(other.maxDescent)
	{
	}

	LineInfo& operator=(const LineInfo& other)
	{
		textOffset = other.textOffset;
		y = other.y;
		height = other.height;
		maxAscent = other.maxAscent;
		maxDescent = other.maxDescent;
		return *this;
	}

	bool operator==(const LineInfo& other) const
	{
		return textOffset == other.textOffset
			&& y == other.y
			&& height == other.height
			&& maxAscent == other.maxAscent
			&& maxDescent == other.maxDescent;
	}

	bool operator!=(const LineInfo& other) const
	{
		return !(*this == other);
	}

public:
	int		textOffset;
	float	y;
	float	height;
	float	maxAscent;
	float	maxDescent;
};


typedef List<LineInfo, false> LineInfoList;


#endif // LINE_INFO_H
