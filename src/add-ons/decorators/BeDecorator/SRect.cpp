/*
	SPoint.cpp:
		Point class which utilizes Frans van Nispen's original SPoint sources
		and extends them slightly for app_server use.
*/

#include <stdio.h>
#include "SRect.h"

bool TestLineIntersect(const SRect& r, float x1, float y1, float x2, float y2,
					   bool vertical = true);


void SRect::InsetBy(SPoint p)
{
	 left += p.x;
	 right -= p.x;
	 top += p.y;
	 bottom -= p.y;
}

void SRect::InsetBy(float dx, float dy)
{
	 left += dx;
	 right -= dx;
	 top += dy;
	 bottom -= dy;
}

SRect& SRect::InsetBySelf(SPoint p)
{
	this->InsetBy(p);
	return *this;
}

SRect& SRect::InsetBySelf(float dx, float dy)
{
	this->InsetBy(dx, dy);
	return *this;
}

SRect SRect::InsetByCopy(SPoint p)
{
	SRect copy(*this);
	copy.InsetBy(p);
	return copy;
}

SRect SRect::InsetByCopy(float dx, float dy)
{
	SRect copy(*this);
	copy.InsetBy(dx, dy);
	return copy;
}

void SRect::OffsetBy(SPoint p)
{
	 left += p.x;
	 right += p.x;
	 top += p.y;
	 bottom += p.y;
}

void SRect::OffsetBy(float dx, float dy)
{
	 left += dx;
	 right += dx;
	 top += dy;
	 bottom += dy;
}

SRect& SRect::OffsetBySelf(SPoint p)
{
	this->OffsetBy(p);
	return *this;
}

SRect& SRect::OffsetBySelf(float dx, float dy)
{
	this->OffsetBy(dx, dy);
	return *this;
}

SRect SRect::OffsetByCopy(SPoint p)
{
	SRect copy(*this);
	copy.OffsetBy(p);
	return copy;
}

SRect SRect::OffsetByCopy(float dx, float dy)
{
	SRect copy(*this);
	copy.OffsetBy(dx, dy);
	return copy;
}

void SRect::OffsetTo(SPoint p)
{
	 right = (right - left) + p.x;
	 left = p.x;
	 bottom = (bottom - top) + p.y;
	 top = p.y;
}

void SRect::OffsetTo(float x, float y)
{
	 right = (right - left) + x;
	 left = x;
	 bottom = (bottom - top) + y;
	 top=y;
}

SRect& SRect::OffsetToSelf(SPoint p)
{
	this->OffsetTo(p);
	return *this;
}

SRect& SRect::OffsetToSelf(float dx, float dy)
{
	this->OffsetTo(dx, dy);
	return *this;
}

SRect SRect::OffsetToCopy(SPoint p)
{
	SRect copy(*this);
	copy.OffsetTo(p);
	return copy;
}

SRect SRect::OffsetToCopy(float dx, float dy)
{
	SRect copy(*this);
	copy.OffsetTo(dx, dy);
	return copy;
}

void SRect::PrintToStream() const
{
	printf("(l:%.1f t:%.1f r:%.1f b:%.1f)\n", left, top, right, bottom);
}

bool SRect::operator==(SRect r) const
{
	 return left == r.left && right == r.right &&
	 		top == r.top && bottom == r.bottom;
}

bool SRect::operator!=(SRect r) const
{
	 return !(*this == r);
}

SRect SRect::operator&(SRect r) const
{
	 return SRect(max_c(left, r.left), max_c(top, r.top),
	 			  min_c(right, r.right), min_c(bottom, r.bottom));
}

SRect SRect::operator|(SRect r) const
{
	 return SRect(min_c(left, r.left), min_c(top, r.top),
	 			  max_c(right, r.right), max_c(bottom, r.bottom));
}

bool SRect::Intersects(SRect r) const
{
	return	TestLineIntersect(*this, r.left, r.top, r.left, r.bottom)		||
			TestLineIntersect(*this, r.left, r.top, r.right, r.top, false)	||
			TestLineIntersect(*this, r.right, r.top, r.right, r.bottom)		||
			TestLineIntersect(*this, r.left, r.bottom, r.right, r.bottom, false);
}

bool SRect::Contains(SPoint p) const
{
	return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
}

bool SRect::Contains(SRect r) const
{
	return r.left >= left && r.right <= right &&
		   r.top >= top && r.bottom <= bottom;
}




bool TestLineIntersect(const SRect& r, float x1, float y1, float x2, float y2,
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
