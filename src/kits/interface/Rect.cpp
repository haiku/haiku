/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 */

#include <stdio.h>

#include <Rect.h>


void
BRect::SetLeftTop(const BPoint p)
{
	left = p.x;
	top = p.y;
}


void
BRect::SetRightBottom(const BPoint p)
{
	right = p.x;
	bottom = p.y;
}


void
BRect::SetLeftBottom(const BPoint p)
{
	left = p.x;
	bottom = p.y;
}


void
BRect::SetRightTop(const BPoint p)
{
	right = p.x;
	top = p.y;
}


void
BRect::InsetBy(BPoint point)
{
	 left += point.x;
	 right -= point.x;
	 top += point.y;
	 bottom -= point.y;
}


void
BRect::InsetBy(float dx, float dy)
{
	 left += dx;
	 right -= dx;
	 top += dy;
	 bottom -= dy;
}


BRect&
BRect::InsetBySelf(BPoint point)
{
	InsetBy(point);
	return *this;
}


BRect&
BRect::InsetBySelf(float dx, float dy)
{
	InsetBy(dx, dy);
	return *this;
}


BRect
BRect::InsetByCopy(BPoint point)
{
	BRect copy(*this);
	copy.InsetBy(point);
	return copy;
}


BRect
BRect::InsetByCopy(float dx, float dy)
{
	BRect copy(*this);
	copy.InsetBy(dx, dy);
	return copy;
}


void
BRect::OffsetBy(BPoint point)
{
	 left += point.x;
	 right += point.x;
	 top += point.y;
	 bottom += point.y;
}


void
BRect::OffsetBy(float dx, float dy)
{
	 left += dx;
	 right += dx;
	 top += dy;
	 bottom += dy;
}


BRect&
BRect::OffsetBySelf(BPoint point)
{
	OffsetBy(point);
	return *this;
}


BRect&
BRect::OffsetBySelf(float dx, float dy)
{
	OffsetBy(dx, dy);
	return *this;
}


BRect
BRect::OffsetByCopy(BPoint point)
{
	BRect copy(*this);
	copy.OffsetBy(point);
	return copy;
}


BRect
BRect::OffsetByCopy(float dx, float dy)
{
	BRect copy(*this);
	copy.OffsetBy(dx, dy);
	return copy;
}


void
BRect::OffsetTo(BPoint point)
{
	 right = (right - left) + point.x;
	 left = point.x;
	 bottom = (bottom - top) + point.y;
	 top = point.y;
}


void
BRect::OffsetTo(float x, float y)
{
	 right = (right - left) + x;
	 left = x;
	 bottom = (bottom - top) + y;
	 top=y;
}


BRect&
BRect::OffsetToSelf(BPoint point)
{
	OffsetTo(point);
	return *this;
}


BRect&
BRect::OffsetToSelf(float dx, float dy)
{
	OffsetTo(dx, dy);
	return *this;
}


BRect
BRect::OffsetToCopy(BPoint point)
{
	BRect copy(*this);
	copy.OffsetTo(point);
	return copy;
}


BRect
BRect::OffsetToCopy(float dx, float dy)
{
	BRect copy(*this);
	copy.OffsetTo(dx, dy);
	return copy;
}


void
BRect::PrintToStream() const
{
	printf("BRect(l:%.1f, t:%.1f, r:%.1f, b:%.1f)\n", left, top, right, bottom);
}


bool
BRect::operator==(BRect rect) const
{
	 return left == rect.left && right == rect.right &&
	 		top == rect.top && bottom == rect.bottom;
}


bool
BRect::operator!=(BRect rect) const
{
	 return !(*this == rect);
}


BRect
BRect::operator&(BRect rect) const
{
	 return BRect(max_c(left, rect.left), max_c(top, rect.top),
	 			  min_c(right, rect.right), min_c(bottom, rect.bottom));
}


BRect
BRect::operator|(BRect rect) const
{
	 return BRect(min_c(left, rect.left), min_c(top, rect.top),
	 			  max_c(right, rect.right), max_c(bottom, rect.bottom));
}


bool
BRect::Intersects(BRect rect) const
{
	if (!IsValid() || !rect.IsValid())
		return false;

	return !(rect.left > right || rect.right < left
			|| rect.top > bottom || rect.bottom < top);	
}


bool
BRect::Contains(BPoint point) const
{
	return point.x >= left && point.x <= right
			&& point.y >= top && point.y <= bottom;
}


bool
BRect::Contains(BRect rect) const
{
	return rect.left >= left && rect.right <= right
			&& rect.top >= top && rect.bottom <= bottom;
}
