//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		IPoint.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Based on BPoint code by Frans Van Nispen
//	Description:	Integer BPoint class
//------------------------------------------------------------------------------
#include "IPoint.h"
#include <math.h>
#include <stdio.h>
#include <Rect.h>

void IPoint::ConstrainTo(BRect r)
{
	x = (int32)max_c(min_c(x, r.right), r.left);
	y = (int32)max_c(min_c(y, r.bottom), r.top);
}

void IPoint::PrintToStream() const
{
	printf("IPoint(x:%ld, y:%ld)\n", x, y);
}

IPoint IPoint::operator+(const IPoint &p) const
{
	return IPoint(x + p.x, y + p.y);
}

IPoint IPoint::operator+(const BPoint &p) const
{
	return IPoint(x + (int32)p.x, y + (int32)p.y);
}

IPoint IPoint::operator-(const IPoint &p) const
{
	return IPoint(x - p.x, y - p.y);
}

IPoint IPoint::operator-(const BPoint &p) const
{
	return IPoint(x - (int32)p.x, y - (int32)p.y);
}

IPoint &IPoint::operator+=(const IPoint &p)
{
	x += p.x;
	y += p.y;

	return *this;
}

IPoint &IPoint::operator+=(const BPoint &p)
{
	x += (int32)p.x;
	y += (int32)p.y;

	return *this;
}

IPoint &IPoint::operator-=(const IPoint &p)
{
	x -= p.x;
	y -= p.y;

	return *this;
}

IPoint &IPoint::operator-=(const BPoint &p)
{
	x -= (int32)p.x;
	y -= (int32)p.y;

	return *this;
}

bool IPoint::operator!=(const IPoint &p) const
{
	return x != p.x || y != p.y;
}

bool IPoint::operator!=(const BPoint &p) const
{
	return x != (int32)p.x || y != (int32)p.y;
}

bool IPoint::operator==(const IPoint &p) const
{
	return x == p.x && y == p.y;
}

bool IPoint::operator==(const BPoint &p) const
{
	return x == (int32)p.x && y == (int32)p.y;
}


