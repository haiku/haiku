/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 */
#ifndef	_RECT_H
#define	_RECT_H


#include <SupportDefs.h>
#include <Point.h>
#include <Size.h>

#include <math.h>


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
		BRect(BPoint leftTop, BSize size);

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
		BRect&	InsetBySelf(BPoint);
		BRect&	InsetBySelf(float dx, float dy);
		BRect	InsetByCopy(BPoint);
		BRect	InsetByCopy(float dx, float dy);
		BRect&	OffsetBySelf(BPoint);
		BRect&	OffsetBySelf(float dx, float dy);
		BRect	OffsetByCopy(BPoint);
		BRect	OffsetByCopy(float dx, float dy);
		BRect&	OffsetToSelf(BPoint);
		BRect&	OffsetToSelf(float dx, float dy);
		BRect	OffsetToCopy(BPoint);
		BRect	OffsetToCopy(float dx, float dy);

		// comparison
		bool	operator==(BRect r) const;
		bool	operator!=(BRect r) const;

		// intersection and union
		BRect	operator&(BRect r) const;
		BRect	operator|(BRect r) const;

		bool	IsValid() const;
		float	Width() const;
		int32	IntegerWidth() const;
		float	Height() const;
		int32	IntegerHeight() const;
		BSize	Size() const;

		bool	Intersects(BRect r) const;
		bool	Contains(BPoint p) const;
		bool	Contains(BRect r) const;
};


// inline definitions ----------------------------------------------------------

inline BPoint
BRect::LeftTop() const
{
	return *(const BPoint *)&left;
}


inline BPoint
BRect::RightBottom() const
{
	return *(const BPoint *)&right;
}


inline BPoint
BRect::LeftBottom() const
{
	return BPoint(left, bottom);
}


inline BPoint
BRect::RightTop() const
{
	return BPoint(right, top);
}


inline
BRect::BRect()
{
	top = left = 0;
	bottom = right = -1;
}


inline
BRect::BRect(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}


inline
BRect::BRect(const BRect &r)
{
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
}


inline
BRect::BRect(BPoint leftTop, BPoint rightBottom)
{
	left = leftTop.x;
	top = leftTop.y;
	right = rightBottom.x;
	bottom = rightBottom.y;
}


inline
BRect::BRect(BPoint leftTop, BSize size)
	: left(leftTop.x),
	  top(leftTop.y),
	  right(leftTop.x + size.width),
	  bottom(leftTop.y + size.height)
{
}


inline BRect &
BRect::operator=(const BRect& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}


inline void
BRect::Set(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}


inline bool
BRect::IsValid() const
{
	return left <= right && top <= bottom;
}


inline int32
BRect::IntegerWidth() const
{
	return (int32)ceil(right - left);
}


inline float
BRect::Width() const
{
	return right - left;
}


inline int32
BRect::IntegerHeight() const
{
	return (int32)ceil(bottom - top);
}


inline float
BRect::Height() const
{
	return bottom - top;
}

inline BSize
BRect::Size() const
{
	return BSize(right - left, bottom - top);
}


#endif	// _RECT_H
