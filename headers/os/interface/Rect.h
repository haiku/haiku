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
//	File Name:		Rect.h
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BRect represents a rectangular area.
//------------------------------------------------------------------------------

#ifndef	_RECT_H
#define	_RECT_H

// Standard Includes -----------------------------------------------------------
#include <math.h>

// System Includes -------------------------------------------------------------
#include <SupportDefs.h>
#include <Point.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BRect class -----------------------------------------------------------------
class BRect {
public:
	float	left;
	float	top;
	float	right;
	float	bottom;

	BRect();
	BRect(const BRect &r);
	BRect(float l, float t, float r, float b);
	BRect(BPoint lt, BPoint rb);

	BRect	&operator=(const BRect &r);
	void	Set(float l, float t, float r, float b);

	void	PrintToStream() const;

	BPoint	LeftTop() const;
	BPoint	RightBottom() const;
	BPoint	LeftBottom() const;
	BPoint	RightTop() const;

	void	SetLeftTop(const BPoint p);
	void	SetRightBottom(const BPoint p);
	void	SetLeftBottom(const BPoint p);
	void	SetRightTop(const BPoint p);

	// transformation
	void	InsetBy(BPoint p);
	void	InsetBy(float dx, float dy);
	void	OffsetBy(BPoint p);
	void	OffsetBy(float dx, float dy);
	void	OffsetTo(BPoint p);
	void	OffsetTo(float x, float y);

	// expression transformations
	BRect &	InsetBySelf(BPoint);
	BRect &	InsetBySelf(float dx, float dy);
	BRect	InsetByCopy(BPoint);
	BRect	InsetByCopy(float dx, float dy);
	BRect &	OffsetBySelf(BPoint);
	BRect &	OffsetBySelf(float dx, float dy);
	BRect	OffsetByCopy(BPoint);
	BRect	OffsetByCopy(float dx, float dy);
	BRect &	OffsetToSelf(BPoint);
	BRect &	OffsetToSelf(float dx, float dy);
	BRect	OffsetToCopy(BPoint);
	BRect	OffsetToCopy(float dx, float dy);

	// comparison
	bool	operator==(BRect r) const;
	bool	operator!=(BRect r) const;

	// intersection and union
	BRect	operator&(BRect r) const;
	BRect	operator|(BRect r) const;

	bool	Intersects(BRect r) const;
	bool	IsValid() const;
	float	Width() const;
	int32	IntegerWidth() const;
	float	Height() const;
	int32	IntegerHeight() const;
	bool	Contains(BPoint p) const;
	bool	Contains(BRect r) const;

};
//------------------------------------------------------------------------------

// inline definitions ----------------------------------------------------------

inline BPoint BRect::LeftTop() const
{
	return(*((const BPoint*)&left));
}

inline BPoint BRect::RightBottom() const
{
	return(*((const BPoint*)&right));
}

inline BPoint BRect::LeftBottom() const
{
	return(BPoint(left, bottom));
}

inline BPoint BRect::RightTop() const
{
	return(BPoint(right, top));
}

inline BRect::BRect()
{
	top = left = 0;
	bottom = right = -1;
}

inline BRect::BRect(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

inline BRect::BRect(const BRect &r)
{
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
}

inline BRect::BRect(BPoint leftTop, BPoint rightBottom)
{
	left = leftTop.x;
	top = leftTop.y;
	right = rightBottom.x;
	bottom = rightBottom.y;
}

inline BRect &BRect::operator=(const BRect& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}

inline void BRect::Set(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

inline bool BRect::IsValid() const
{
	if (left <= right && top <= bottom)
		return true;
	else
		return false;
}

inline int32 BRect::IntegerWidth() const
{
	return((int32)ceil(right - left));
}

inline float BRect::Width() const
{
	return(right - left);
}

inline int32 BRect::IntegerHeight() const
{
	return((int32)ceil(bottom - top));
}

inline float BRect::Height() const
{
	return(bottom - top);
}

//------------------------------------------------------------------------------

#endif	// _RECT_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

