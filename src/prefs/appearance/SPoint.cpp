/*
	SPoint.cpp:
		Point class which utilizes Frans van Nispen's original SPoint sources
		and extends them slightly for app_server use.
*/

#include <math.h>
#include <stdio.h>

#include <BeBuild.h>
#include <SupportDefs.h>
#include "SPoint.h"
#include "SRect.h"



void SPoint::ConstrainTo(SRect r)
{
	x = max_c(min_c(x, r.right), r.left);
	y = max_c(min_c(y, r.bottom), r.top);
}

void SPoint::PrintToStream() const
{
	printf("SPoint(x:%.0f, y:%.0f)\n", x, y);
}

SPoint SPoint::operator+(const SPoint& p) const
{
	return SPoint(x + p.x, y + p.y);
}

SPoint SPoint::operator-(const SPoint& p) const
{
	return SPoint(x - p.x, y - p.y);
}

SPoint SPoint::operator*(const SPoint& p) const
{
	return SPoint(x * p.x, y * p.y);
}

SPoint SPoint::operator/(const SPoint& p) const
{
	return SPoint((p.x!=0)?(x / p.x):0,(p.y!=0)?(y / p.y):0);
}

SPoint&	SPoint::operator+=(const SPoint& p)
{
	x += p.x;
	y += p.y;

	return *this;
}

SPoint&	SPoint::operator-=(const SPoint& p)
{
	x -= p.x;
	y -= p.y;

	return *this;
}

bool SPoint::operator!=(const SPoint& p) const
{
	return x != p.x || y != p.y;
}

bool SPoint::operator==(const SPoint& p) const
{
	return x == p.x && y == p.y;
}

bool SPoint::operator<=(const SPoint& p) const
{
	return x <= p.x && y <= p.y;
}

bool SPoint::operator>=(const SPoint& p) const
{
	return x >= p.x && y >= p.y;
}

bool SPoint::operator<(const SPoint& p) const
{
	return x < p.x || y < p.y;
}

bool SPoint::operator>(const SPoint& p) const
{
	return x > p.x && y > p.y;
}
