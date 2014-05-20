/*
 * Copyright 2001-2014 Haiku, Inc. All rights reserved.
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
BPoint::ConstrainTo(BRect rect)
{
	x = max_c(min_c(x, rect.right), rect.left);
	y = max_c(min_c(y, rect.bottom), rect.top);
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
BPoint::operator+(const BPoint& other) const
{
	return BPoint(x + other.x, y + other.y);
}


BPoint
BPoint::operator-(const BPoint& other) const
{
	return BPoint(x - other.x, y - other.y);
}


BPoint&
BPoint::operator+=(const BPoint& other)
{
	x += other.x;
	y += other.y;

	return *this;
}


BPoint&
BPoint::operator-=(const BPoint& other)
{
	x -= other.x;
	y -= other.y;

	return *this;
}


bool
BPoint::operator!=(const BPoint& other) const
{
	return x != other.x || y != other.y;
}


bool
BPoint::operator==(const BPoint& other) const
{
	return x == other.x && y == other.y;
}
