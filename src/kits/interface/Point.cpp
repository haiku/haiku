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
//	File Name:		Point.cpp
//	Author:			Frans van Nispen
//	Description:	BPoint represents a single x,y coordinate.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <math.h>
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <SupportDefs.h>
#include <Point.h>
#include <Rect.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------
void BPoint::ConstrainTo(BRect r)
{
	x = max_c(min_c(x, r.right), r.left);
	y = max_c(min_c(y, r.bottom), r.top);
}
//------------------------------------------------------------------------------
void BPoint::PrintToStream() const
{
	printf("BPoint(x:%.0f, y:%.0f)\n", x, y);
}
//------------------------------------------------------------------------------
BPoint BPoint::operator+(const BPoint& p) const
{
	return BPoint(x + p.x, y + p.y);
}
//------------------------------------------------------------------------------
BPoint BPoint::operator-(const BPoint& p) const
{
	return BPoint(x - p.x, y - p.y);
}
//------------------------------------------------------------------------------
BPoint&	BPoint::operator+=(const BPoint& p)
{
	x += p.x;
	y += p.y;

	return *this;
}
//------------------------------------------------------------------------------
BPoint&	BPoint::operator-=(const BPoint& p)
{
	x -= p.x;
	y -= p.y;

	return *this;
}
//------------------------------------------------------------------------------
bool BPoint::operator!=(const BPoint& p) const
{
	return x != p.x || y != p.y;
}
//------------------------------------------------------------------------------
bool BPoint::operator==(const BPoint& p) const
{
	return x == p.x && y == p.y;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

