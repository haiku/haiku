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
//	File Name:		Rect.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BRect represents a rectangular area.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Rect.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

bool TestLineIntersect(const BRect& r, float x1, float y1, float x2, float y2,
					   bool vertical = true);

//------------------------------------------------------------------------------
void BRect::InsetBy(BPoint p)
{
	 left += p.x;
	 right -= p.x;
	 top += p.y;
	 bottom -= p.y;
}
//------------------------------------------------------------------------------
void BRect::InsetBy(float dx, float dy)
{
	 left += dx;
	 right -= dx;
	 top += dy;
	 bottom -= dy;
}
//------------------------------------------------------------------------------
BRect& BRect::InsetBySelf(BPoint p)
{
	this->InsetBy(p);
	return *this;
}
//------------------------------------------------------------------------------
BRect& BRect::InsetBySelf(float dx, float dy)
{
	this->InsetBy(dx, dy);
	return *this;
}
//------------------------------------------------------------------------------
BRect BRect::InsetByCopy(BPoint p)
{
	BRect copy(*this);
	copy.InsetBy(p);
	return copy;
}
//------------------------------------------------------------------------------
BRect BRect::InsetByCopy(float dx, float dy)
{
	BRect copy(*this);
	copy.InsetBy(dx, dy);
	return copy;
}
//------------------------------------------------------------------------------
void BRect::OffsetBy(BPoint p)
{
	 left += p.x;
	 right += p.x;
	 top += p.y;
	 bottom += p.y;
}
//------------------------------------------------------------------------------
void BRect::OffsetBy(float dx, float dy)
{
	 left += dx;
	 right += dx;
	 top += dy;
	 bottom += dy;
}
//------------------------------------------------------------------------------
BRect& BRect::OffsetBySelf(BPoint p)
{
	this->OffsetBy(p);
	return *this;
}
//------------------------------------------------------------------------------
BRect& BRect::OffsetBySelf(float dx, float dy)
{
	this->OffsetBy(dx, dy);
	return *this;
}
//------------------------------------------------------------------------------
BRect BRect::OffsetByCopy(BPoint p)
{
	BRect copy(*this);
	copy.OffsetBy(p);
	return copy;
}
//------------------------------------------------------------------------------
BRect BRect::OffsetByCopy(float dx, float dy)
{
	BRect copy(*this);
	copy.OffsetBy(dx, dy);
	return copy;
}
//------------------------------------------------------------------------------
void BRect::OffsetTo(BPoint p)
{
	 right = (right - left) + p.x;
	 left = p.x;
	 bottom = (bottom - top) + p.y;
	 top = p.y;
}
//------------------------------------------------------------------------------
void BRect::OffsetTo(float x, float y)
{
	 right = (right - left) + x;
	 left = x;
	 bottom = (bottom - top) + y;
	 top=y;
}
//------------------------------------------------------------------------------
BRect& BRect::OffsetToSelf(BPoint p)
{
	this->OffsetTo(p);
	return *this;
}
//------------------------------------------------------------------------------
BRect& BRect::OffsetToSelf(float dx, float dy)
{
	this->OffsetTo(dx, dy);
	return *this;
}
//------------------------------------------------------------------------------
BRect BRect::OffsetToCopy(BPoint p)
{
	BRect copy(*this);
	copy.OffsetTo(p);
	return copy;
}
//------------------------------------------------------------------------------
BRect BRect::OffsetToCopy(float dx, float dy)
{
	BRect copy(*this);
	copy.OffsetTo(dx, dy);
	return copy;
}
//------------------------------------------------------------------------------
void BRect::PrintToStream() const
{
	printf("(l:%.1f t:%.1f r:%.1f b:%.1f)\n", left, top, right, bottom);
}
//------------------------------------------------------------------------------
bool BRect::operator==(BRect r) const
{
	 return left == r.left && right == r.right &&
	 		top == r.top && bottom == r.bottom;
}
//------------------------------------------------------------------------------
bool BRect::operator!=(BRect r) const
{
	 return !(*this == r);
}
//------------------------------------------------------------------------------
BRect BRect::operator&(BRect r) const
{
	 return BRect(max_c(left, r.left), max_c(top, r.top),
	 			  min_c(right, r.right), min_c(bottom, r.bottom));
}
//------------------------------------------------------------------------------
BRect BRect::operator|(BRect r) const
{
	 return BRect(min_c(left, r.left), min_c(top, r.top),
	 			  max_c(right, r.right), max_c(bottom, r.bottom));
}
//------------------------------------------------------------------------------
bool BRect::Intersects(BRect r) const
{
	return	TestLineIntersect(*this, r.left, r.top, r.left, r.bottom)		||
			TestLineIntersect(*this, r.left, r.top, r.right, r.top, false)	||
			TestLineIntersect(*this, r.right, r.top, r.right, r.bottom)		||
			TestLineIntersect(*this, r.left, r.bottom, r.right, r.bottom, false);
}
//------------------------------------------------------------------------------
bool BRect::Contains(BPoint p) const
{
	return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
}
//------------------------------------------------------------------------------
bool BRect::Contains(BRect r) const
{
	return r.left >= left && r.right <= right &&
		   r.top >= top && r.bottom <= bottom;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
bool TestLineIntersect(const BRect& r, float x1, float y1, float x2, float y2,
					   bool vertical)
{
	if (vertical)
	{
		return	(x1 >= r.left && x1 <= r.right) &&
				((y1 >= r.top && y1 <= r.bottom) ||
				 (y2 >= r.top && y2 <= r.bottom));
	}
	else
	{
		return	(y1 >= r.top && y1 <= r.bottom) &&
				((x1 >= r.left && x1 <= r.right) ||
				 (x2 >= r.left && x2 <= r.right));
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

