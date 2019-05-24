/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERM_POS_H
#define TERM_POS_H

#include <SupportDefs.h>


class TermPos {
public:
	int32	x;
	int32	y;

	inline TermPos()						: x(0), y(0) { }
	inline TermPos(int32 x, int32 y)		: x(x), y(y) { }

	inline void SetTo(int32 x, int32 y)
	{
		this->x = x;
		this->y = y;
	}

	inline bool operator==(const TermPos& other) const
	{
		return x == other.x && y == other.y;
	}

	inline bool operator!=(const TermPos& other) const
	{
		return x != other.x || y != other.y;
	}

	inline bool operator<=(const TermPos& other) const
	{
		return y < other.y || (y == other.y && x <= other.x);
	}

	inline bool operator>=(const TermPos& other) const
	{
		return other <= *this;
	}

	inline bool operator<(const TermPos& other) const
	{
		return !(*this >= other);
	}

	inline bool operator>(const TermPos& other) const
	{
		return !(*this <= other);
	}
};


#endif	// TERM_POS_H
