/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef	INT_RECT_H
#define	INT_RECT_H


#include <Region.h>

#include "IntPoint.h"

class IntRect {
 public:
			int32				left;
			int32				top;
			int32				right;
			int32				bottom;
	
								IntRect();
								IntRect(const IntRect& r);
								IntRect(const BRect& r);
								IntRect(int32 l, int32 t, int32 r, int32 b);
								IntRect(const IntPoint& lt,
										const IntPoint& rb);
	
			IntRect&			operator=(const IntRect &r);
			void				Set(int32 l, int32 t, int32 r, int32 b);
	
			void				PrintToStream() const;
	
			IntPoint			LeftTop() const;
			IntPoint			RightBottom() const;
			IntPoint			LeftBottom() const;
			IntPoint			RightTop() const;
	
			void				SetLeftTop(const IntPoint& p);
			void				SetRightBottom(const IntPoint& p);
			void				SetLeftBottom(const IntPoint& p);
			void				SetRightTop(const IntPoint& p);
	
			// transformation
			void				InsetBy(const IntPoint& p);
			void				InsetBy(int32 dx, int32 dy);
			void				OffsetBy(const IntPoint& p);
			void				OffsetBy(int32 dx, int32 dy);
			void				OffsetTo(const IntPoint& p);
			void				OffsetTo(int32 x, int32 y);
	
			// expression transformations
			IntRect&			InsetBySelf(const IntPoint& p);
			IntRect&			InsetBySelf(int32 dx, int32 dy);
			IntRect				InsetByCopy(const IntPoint& p);
			IntRect				InsetByCopy(int32 dx, int32 dy);
			IntRect&			OffsetBySelf(const IntPoint& p);
			IntRect&			OffsetBySelf(int32 dx, int32 dy);
			IntRect				OffsetByCopy(const IntPoint& p);
			IntRect				OffsetByCopy(int32 dx, int32 dy);
			IntRect&			OffsetToSelf(const IntPoint& p);
			IntRect&			OffsetToSelf(int32 dx, int32 dy);
			IntRect				OffsetToCopy(const IntPoint& p);
			IntRect				OffsetToCopy(int32 dx, int32 dy);
	
			// comparison
			bool				operator==(const IntRect& r) const;
			bool				operator!=(const IntRect& r) const;
	
			// intersection and union
			IntRect				operator&(const IntRect& r) const;
			IntRect				operator|(const IntRect& r) const;

			// conversion to BRect and clipping_rect
								operator clipping_rect() const;
								operator BRect() const
									{ return BRect(left, top,
												   right, bottom); }
	
			bool				Intersects(const IntRect& r) const;
			bool				IsValid() const;
			int32				Width() const;
			int32				IntegerWidth() const;
			int32				Height() const;
			int32				IntegerHeight() const;
			bool				Contains(const IntPoint& p) const;
			bool				Contains(const IntRect& r) const;
};


// inline definitions ----------------------------------------------------------

inline IntPoint
IntRect::LeftTop() const
{
	return *(const IntPoint*)&left;
}


inline IntPoint
IntRect::RightBottom() const
{
	return *(const IntPoint*)&right;
}


inline IntPoint
IntRect::LeftBottom() const
{
	return IntPoint(left, bottom);
}


inline IntPoint
IntRect::RightTop() const
{
	return IntPoint(right, top);
}


inline
IntRect::IntRect()
{
	top = left = 0;
	bottom = right = -1;
}


inline
IntRect::IntRect(int32 l, int32 t, int32 r, int32 b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}


inline
IntRect::IntRect(const IntRect &r)
{
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
}


inline
IntRect::IntRect(const BRect &r)
{
	left = (int32)r.left;
	top = (int32)r.top;
	right = (int32)r.right;
	bottom = (int32)r.bottom;
}


inline
IntRect::IntRect(const IntPoint& leftTop, const IntPoint& rightBottom)
{
	left = leftTop.x;
	top = leftTop.y;
	right = rightBottom.x;
	bottom = rightBottom.y;
}


inline IntRect &
IntRect::operator=(const IntRect& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}


inline void
IntRect::Set(int32 l, int32 t, int32 r, int32 b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}


inline bool
IntRect::IsValid() const
{
	return left <= right && top <= bottom;
}


inline int32
IntRect::IntegerWidth() const
{
	return right - left;
}


inline int32
IntRect::Width() const
{
	return right - left;
}


inline int32
IntRect::IntegerHeight() const
{
	return bottom - top;
}


inline int32
IntRect::Height() const
{
	return bottom - top;
}

inline
IntRect::operator clipping_rect() const
{
	clipping_rect r;
	r.left = left;
	r.top = top;
	r.right = right;
	r.bottom = bottom;
	return r;
}


#endif	// INT_RECT_H
