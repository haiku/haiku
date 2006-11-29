/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef	INT_POINT_H
#define	INT_POINT_H

#include <Point.h>

class IntRect;

class IntPoint {
 public:
			int32				x;
			int32				y;

								IntPoint();
								IntPoint(int32 X, int32 Y);
								IntPoint(const IntPoint& p);
								IntPoint(const BPoint& p);
		
			IntPoint&			operator=(const IntPoint& p);
			void				Set(int32 x, int32 y);

			void				ConstrainTo(const IntRect& r);
			void				PrintToStream() const;
			
			IntPoint			operator+(const IntPoint& p) const;
			IntPoint			operator-(const IntPoint& p) const;
			IntPoint&			operator+=(const IntPoint& p);
			IntPoint&			operator-=(const IntPoint& p);

			bool				operator!=(const IntPoint& p) const;
			bool				operator==(const IntPoint& p) const;

			// conversion to BPoint
								operator BPoint() const
									{ return BPoint((float)x, (float)y); }
};


inline
IntPoint::IntPoint()
	: x(0),
	  y(0)
{
}


inline
IntPoint::IntPoint(int32 x, int32 y)
	: x(x),
	  y(y)
{
}


inline
IntPoint::IntPoint(const IntPoint& p)
	: x(p.x),
	  y(p.y)
{
}


inline
IntPoint::IntPoint(const BPoint& p)
	: x((int32)p.x),
	  y((int32)p.y)
{
}


inline IntPoint&
IntPoint::operator=(const IntPoint& from)
{
	x = from.x;
	y = from.y;
	return *this;
}


inline void
IntPoint::Set(int32 x, int32 y)
{
	this->x = x;
	this->y = y;
}

#endif	// INT_POINT_H
