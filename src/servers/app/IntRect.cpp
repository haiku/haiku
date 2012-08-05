/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IntRect.h"

#include <stdio.h>


void
IntRect::SetLeftTop(const IntPoint& p)
{
	left = p.x;
	top = p.y;
}


void
IntRect::SetRightBottom(const IntPoint& p)
{
	right = p.x;
	bottom = p.y;
}


void
IntRect::SetLeftBottom(const IntPoint& p)
{
	left = p.x;
	bottom = p.y;
}


void
IntRect::SetRightTop(const IntPoint& p)
{
	right = p.x;
	top = p.y;
}


void
IntRect::InsetBy(const IntPoint& point)
{
	 left += point.x;
	 right -= point.x;
	 top += point.y;
	 bottom -= point.y;
}


void
IntRect::InsetBy(int32 dx, int32 dy)
{
	 left += dx;
	 right -= dx;
	 top += dy;
	 bottom -= dy;
}


IntRect&
IntRect::InsetBySelf(const IntPoint& point)
{
	InsetBy(point);
	return *this;
}


IntRect&
IntRect::InsetBySelf(int32 dx, int32 dy)
{
	InsetBy(dx, dy);
	return *this;
}


IntRect
IntRect::InsetByCopy(const IntPoint& point)
{
	IntRect copy(*this);
	copy.InsetBy(point);
	return copy;
}


IntRect
IntRect::InsetByCopy(int32 dx, int32 dy)
{
	IntRect copy(*this);
	copy.InsetBy(dx, dy);
	return copy;
}


void
IntRect::OffsetBy(const IntPoint& point)
{
	 left += point.x;
	 right += point.x;
	 top += point.y;
	 bottom += point.y;
}


void
IntRect::OffsetBy(int32 dx, int32 dy)
{
	 left += dx;
	 right += dx;
	 top += dy;
	 bottom += dy;
}


IntRect&
IntRect::OffsetBySelf(const IntPoint& point)
{
	OffsetBy(point);
	return *this;
}


IntRect&
IntRect::OffsetBySelf(int32 dx, int32 dy)
{
	OffsetBy(dx, dy);
	return *this;
}


IntRect
IntRect::OffsetByCopy(const IntPoint& point)
{
	IntRect copy(*this);
	copy.OffsetBy(point);
	return copy;
}


IntRect
IntRect::OffsetByCopy(int32 dx, int32 dy)
{
	IntRect copy(*this);
	copy.OffsetBy(dx, dy);
	return copy;
}


void
IntRect::OffsetTo(const IntPoint& point)
{
	 right = (right - left) + point.x;
	 left = point.x;
	 bottom = (bottom - top) + point.y;
	 top = point.y;
}


void
IntRect::OffsetTo(int32 x, int32 y)
{
	 right = (right - left) + x;
	 left = x;
	 bottom = (bottom - top) + y;
	 top=y;
}


IntRect&
IntRect::OffsetToSelf(const IntPoint& point)
{
	OffsetTo(point);
	return *this;
}


IntRect&
IntRect::OffsetToSelf(int32 dx, int32 dy)
{
	OffsetTo(dx, dy);
	return *this;
}


IntRect
IntRect::OffsetToCopy(const IntPoint& point)
{
	IntRect copy(*this);
	copy.OffsetTo(point);
	return copy;
}


IntRect
IntRect::OffsetToCopy(int32 dx, int32 dy)
{
	IntRect copy(*this);
	copy.OffsetTo(dx, dy);
	return copy;
}


void
IntRect::PrintToStream() const
{
	printf("IntRect(l:%" B_PRId32 ", t:%" B_PRId32 ", r:%" B_PRId32 ", b:%"
		B_PRId32 ")\n", left, top, right, bottom);
}


bool
IntRect::operator==(const IntRect& rect) const
{
	 return left == rect.left && right == rect.right &&
	 		top == rect.top && bottom == rect.bottom;
}


bool
IntRect::operator!=(const IntRect& rect) const
{
	 return !(*this == rect);
}


IntRect
IntRect::operator&(const IntRect& rect) const
{
	 return IntRect(max_c(left, rect.left), max_c(top, rect.top),
	 				min_c(right, rect.right), min_c(bottom, rect.bottom));
}


IntRect
IntRect::operator|(const IntRect& rect) const
{
	 return IntRect(min_c(left, rect.left), min_c(top, rect.top),
	 				max_c(right, rect.right), max_c(bottom, rect.bottom));
}


bool
IntRect::Intersects(const IntRect& rect) const
{
	if (!IsValid() || !rect.IsValid())
		return false;

	return !(rect.left > right || rect.right < left
			|| rect.top > bottom || rect.bottom < top);	
}


bool
IntRect::Contains(const IntPoint& point) const
{
	return point.x >= left && point.x <= right
			&& point.y >= top && point.y <= bottom;
}


bool
IntRect::Contains(const IntRect& rect) const
{
	return rect.left >= left && rect.right <= right
			&& rect.top >= top && rect.bottom <= bottom;
}

