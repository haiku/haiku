//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		RectUtils.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Utilities for work around R5 Interface Kit bugs
//  
//------------------------------------------------------------------------------
#include "RectUtils.h"

/*!
	\brief Checks for the intersection of two rectangles. Works properly, 
	unlike BRect::Intersects
	
	\param r First rectangle
	\param r2 Second rectangle
	\return True if they intersect, false if they don't.
	
	The two rectangles intersect if they are equal, one contains the other,
	or if any edge intersects any edge of the other rectangle.
*/
bool TestRectIntersection(const BRect &r,const BRect &r2)
{
	return	TestLineIntersection(r, r2.left, r2.top, r2.left, r2.bottom)		||
			TestLineIntersection(r, r2.left, r2.top, r2.right, r2.top, false)	||
			TestLineIntersection(r, r2.right, r2.top, r2.right, r2.bottom)		||
			TestLineIntersection(r, r2.left, r2.bottom, r2.right, r2.bottom, false) ||
			r.Contains(r2) ||
			r2.Contains(r);
}

/*!
	\brief Checks to see if a region intersects with a particular rectangle
	
	\param r Region
	\param r2 Rectangle to check intersection with
	\return True if they intersect, false if they don't.
	
	The two intersect if they the rectangle intersects any one rectangle in the 
	region.
*/
bool TestRegionIntersection(BRegion *r,const BRect &r2)
{
	for(int32 i=0; i<r->CountRects(); i++)
		if(TestRectIntersection(r->RectAt(i),r2));
			return true;
	return false;
}

/*!
	\brief Modifies the region to intersect with the rectangle given
	
	\param r Region
	\param r2 Rectangle to intersect with
*/
void IntersectRegionWith(BRegion *r,const BRect &r2)
{
	// We have three conditions:
	// 1) Region frame contains rect. Action: call Include()
	// 2) Rect intersects region frame. Action: call IntersectWith
	// 3) Region frame does not intersect rectangle. Make the region empty
	if(r->Frame().Contains(r2))
		r->Include(r2);
	if(r->Frame().Intersects(r2))
	{
		BRegion reg(r2);
		r->IntersectWith(&reg);
	}
	else
		r->MakeEmpty();
}

/*!
	\brief Checks for the intersection of a line with a rectangle
	
	\param r Rectangle
	\param x1 starting x of line
	\param y1 starting y of line
	\param x2 ending x of line
	\param y2 ending y of line
	\param vertical Test for vertical intersection
	\return true if they intersect, false if not
*/
bool TestLineIntersection(const BRect& r, float x1, float y1, float x2, float y2,
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

/*!
	\brief Ensures that a BRect's IsValid() member returns true
	\param rect BRect to validate
*/
void ValidateRect(BRect *rect)
{
	float l,r,t,b;
	l=(rect->left<rect->right)?rect->left:rect->right;
	r=(rect->left>rect->right)?rect->left:rect->right;
	t=(rect->top<rect->bottom)?rect->top:rect->bottom;
	b=(rect->top>rect->bottom)?rect->top:rect->bottom;
	rect->Set(l,t,r,b);
}
