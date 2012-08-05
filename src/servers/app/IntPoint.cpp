/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "IntPoint.h"

#include <stdio.h>

#include "IntRect.h"


void
IntPoint::ConstrainTo(const IntRect& r)
{
	x = max_c(min_c(x, r.right), r.left);
	y = max_c(min_c(y, r.bottom), r.top);
}


void
IntPoint::PrintToStream() const
{
	printf("IntPoint(x:%" B_PRId32 ", y:%" B_PRId32 ")\n", x, y);
}


IntPoint
IntPoint::operator+(const IntPoint& p) const
{
	return IntPoint(x + p.x, y + p.y);
}


IntPoint
IntPoint::operator-(const IntPoint& p) const
{
	return IntPoint(x - p.x, y - p.y);
}


IntPoint &
IntPoint::operator+=(const IntPoint& p)
{
	x += p.x;
	y += p.y;

	return *this;
}


IntPoint &
IntPoint::operator-=(const IntPoint& p)
{
	x -= p.x;
	y -= p.y;

	return *this;
}


bool
IntPoint::operator!=(const IntPoint& p) const
{
	return x != p.x || y != p.y;
}


bool
IntPoint::operator==(const IntPoint& p) const
{
	return x == p.x && y == p.y;
}

