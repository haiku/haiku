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
//	File Name:		IPoint.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Based on BPoint code by Frans Van Nispen
//	Description:	Integer BPoint class
//------------------------------------------------------------------------------
#ifndef	_IPOINT_H
#define	_IPOINT_H

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <SupportDefs.h>
#include <Point.h>

class BRect;

class IPoint 
{
public:
	int32 x;
	int32 y;

	IPoint(void);
	IPoint(int32 X, int32 Y);
	IPoint(const IPoint &p);
	IPoint(const BPoint &p);
		
	IPoint &operator=(const IPoint &p);
	IPoint &operator=(const BPoint &p);
	void Set(int32 X, int32 Y);
	
	void ConstrainTo(BRect r);
	
	void PrintToStream() const;
			
	IPoint operator+(const IPoint &p) const;
	IPoint operator+(const BPoint &p) const;
	IPoint operator-(const IPoint &p) const;
	IPoint operator-(const BPoint &p) const;
	IPoint &operator+=(const IPoint &p);
	IPoint &operator+=(const BPoint &p);
	IPoint &operator-=(const IPoint &p);
	IPoint &operator-=(const BPoint &p);

	bool operator!=(const IPoint &p) const;
	bool operator!=(const BPoint &p) const;
	bool operator==(const IPoint &p) const;
	bool operator==(const BPoint &p) const;
};

//------------------------------------------------------------------------------
inline IPoint::IPoint(void)
{
	x = y = 0;
}
//------------------------------------------------------------------------------
inline IPoint::IPoint(int32 X, int32 Y)
{
	x = X;
	y = Y;
}
//------------------------------------------------------------------------------
inline IPoint::IPoint(const IPoint& pt)
{
	x = pt.x;
	y = pt.y;
}
//------------------------------------------------------------------------------
inline IPoint::IPoint(const BPoint& pt)
{
	x = (int32)pt.x;
	y = (int32)pt.y;
}
//------------------------------------------------------------------------------
inline IPoint &IPoint::operator=(const IPoint& from)
{
	x = from.x;
	y = from.y;
	return *this;
}
//------------------------------------------------------------------------------
inline IPoint &IPoint::operator=(const BPoint& from)
{
	x = (int32)from.x;
	y = (int32)from.y;
	return *this;
}
//------------------------------------------------------------------------------
inline void IPoint::Set(int32 X, int32 Y)
{
	x = X;
	y = Y;
}
//------------------------------------------------------------------------------

#endif
