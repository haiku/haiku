/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */
#include "CurPos.h"

CurPos::CurPos()
{
}

CurPos::CurPos(int32 X, int32 Y)
{
  x = X;
  y = Y;
}

CurPos::CurPos(const CurPos& cp)
{
  x = cp.x;
  y = cp.y;
}

CurPos &
CurPos::operator=(const CurPos& from)
{
  x = from.x;
  y = from.y;
  return *this;
}

void
CurPos::Set(int32 X, int32 Y)
{
  x = X;
  y = Y;
}

bool
CurPos::operator!=(const CurPos& from) const
{
  return ((x != from.x) || (y != from.y));
}

bool
CurPos::operator==(const CurPos& from) const
{
  return ((x == from.x) && (y == from.y));
}

CurPos
CurPos::operator+(const CurPos& from) const
{
  return CurPos(x + from.x, y + from.y);
}

CurPos
CurPos::operator- (const CurPos& from) const
{
  return CurPos(x - from.x, y - from.y);
}

bool
CurPos::operator> (const CurPos& from) const
{
	if (y > from.y)
		return true;
	else
	if (y == from.y && x > from.x)
		return true;
	
	return false;
}

bool
CurPos::operator>= (const CurPos& from) const
{
	if (y > from.y)
		return true;
	else
	if (y == from.y && x >= from.x)
		return true;
	
	return false;
}

bool
CurPos::operator< (const CurPos& from) const
{
	if (y < from.y)
		return true;
	else
	if (y == from.y && x < from.x)
		return true;
		
	return false;
}

bool
CurPos::operator<= (const CurPos& from) const
{
	if (y < from.y)
		return true;
	else
	if (y == from.y && x <= from.x)
		return true;
	
	return false;
}
