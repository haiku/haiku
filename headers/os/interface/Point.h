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
//	File Name:		Point.h
//	Author:			Frans van Nispen
//	Description:	BPoint represents a single x,y coordinate.
//------------------------------------------------------------------------------
#ifndef	_POINT_H
#define	_POINT_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


class BRect;

// BPoint class ----------------------------------------------------------------
class BPoint {
public:
	float x;
	float y;

	BPoint();
	BPoint(float X, float Y);
	BPoint(const BPoint &p);
		
	BPoint	&operator=(const BPoint &p);
	void	Set(float X, float Y);

	void	ConstrainTo(BRect r);
	void	PrintToStream() const;
			
	BPoint	operator+(const BPoint &p) const;
	BPoint	operator-(const BPoint &p) const;
	BPoint&	operator+=(const BPoint &p);
	BPoint&	operator-=(const BPoint &p);

	bool	operator!=(const BPoint &p) const;
	bool	operator==(const BPoint &p) const;
};
//------------------------------------------------------------------------------

extern _IMPEXP_BE const BPoint B_ORIGIN;	// returns (0,0)

//------------------------------------------------------------------------------
inline BPoint::BPoint()
{
	x = y = 0;
}
//------------------------------------------------------------------------------
inline BPoint::BPoint(float X, float Y)
{
	x = X;
	y = Y;
}
//------------------------------------------------------------------------------
inline BPoint::BPoint(const BPoint& pt)
{
	x = pt.x;
	y = pt.y;
}
//------------------------------------------------------------------------------
inline BPoint &BPoint::operator=(const BPoint& from)
{
	x = from.x;
	y = from.y;
	return *this;
}
//------------------------------------------------------------------------------
inline void BPoint::Set(float X, float Y)
{
	x = X;
	y = Y;
}
//------------------------------------------------------------------------------

#endif	// _POINT_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

