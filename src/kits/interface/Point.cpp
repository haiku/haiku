/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 */

#include <Point.h>

#include <stdio.h>

#include <SupportDefs.h>
#include <Rect.h>


const BPoint B_ORIGIN(0, 0);


void
BPoint::ConstrainTo(BRect r)
{
	x = max_c(min_c(x, r.right), r.left);
	y = max_c(min_c(y, r.bottom), r.top);
}


void
BPoint::PrintToStream() const
{
	printf("BPoint(x:%.0f, y:%.0f)\n", x, y);
}


BPoint
BPoint::operator-() const
{
	return BPoint(-x, -y);
}


BPoint
BPoint::operator+(const BPoint& p) const
{
	return BPoint(x + p.x, y + p.y);
}


BPoint
BPoint::operator-(const BPoint& p) const
{
	return BPoint(x - p.x, y - p.y);
}


BPoint &
BPoint::operator+=(const BPoint& p)
{
	x += p.x;
	y += p.y;

	return *this;
}


BPoint &
BPoint::operator-=(const BPoint& p)
{
	x -= p.x;
	y -= p.y;

	return *this;
}


bool
BPoint::operator!=(const BPoint& p) const
{
	return x != p.x || y != p.y;
}


bool
BPoint::operator==(const BPoint& p) const
{
	return x == p.x && y == p.y;
}

