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
//	File Name:		Polygon.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BPolygon represents a n-sided area.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Polygon.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BPolygon::BPolygon(const BPoint *ptArray, int32 numPoints) :
	fBounds(0.0, 0.0, 0.0, 0.0), fCount(numPoints), fPts(NULL)
{
	if (fCount > 0) {
		fPts = new BPoint[numPoints];
		
		// Note the use of memcpy here.  The assumption is that an array of BPoints can
		// be copied bit by bit and not use a copy constructor or an assignment
		// operator.  This breaks the containment of BPoint but will result in better
		// performance.  An example where the memcpy will fail would be if BPoint begins
		// to do lazy copying through reference counting.  By copying the bits, we will
		// copy reference counting state which will not be relevant at the destination.
		// Luckily, BPoint is a very simple class which isn't likely to change much.
		// However, it is a risk of this implementation.
		//
		// If necessary, this code can be changed to iterate over the input array of
		// BPoints and use the assignment operator to copy from the source to the
		// destination array, one element at a time.
		//
		// Similar use of memcpy appears later in this implementation also.
		//
		memcpy(fPts, ptArray, numPoints * sizeof(BPoint));
		compute_bounds();
	}
}
//------------------------------------------------------------------------------
BPolygon::BPolygon(const BPolygon *poly)
{
	*this = *poly;
}
//------------------------------------------------------------------------------
BPolygon::BPolygon ()
	:	fBounds(0.0, 0.0, 0.0, 0.0),
	    fCount(0),
		fPts(NULL)
{
}
//------------------------------------------------------------------------------
BPolygon::~BPolygon ()
{
	if (fPts)
		delete[] fPts;
}
//------------------------------------------------------------------------------
BPolygon &BPolygon::operator=(const BPolygon &from)
{
	// Make sure we aren't trying to perform a "self assignment".
	if (this != &from) {
		fBounds = from.fBounds;
		fCount = from.fCount;
		if (fCount > 0) {
			fPts = new BPoint[fCount];
			memcpy(fPts, from.fPts, fCount * sizeof(BPoint));
		}
	}
	return *this;
}
//------------------------------------------------------------------------------
BRect BPolygon::Frame() const
{
	return fBounds;
}
//------------------------------------------------------------------------------
void BPolygon::AddPoints(const BPoint *ptArray, int32 numPoints)
{
	if (numPoints > 0) {
		BPoint *newPts = new BPoint[fCount + numPoints];
		if (fPts) {
			memcpy(newPts, fPts, fCount * sizeof(BPoint));
			delete fPts;
		}
		memcpy(newPts + fCount, ptArray, numPoints * sizeof(BPoint));
		fPts = newPts;
		fCount += numPoints;
		compute_bounds();
	}
}
//------------------------------------------------------------------------------
int32 BPolygon::CountPoints() const
{
	return fCount;
}
//------------------------------------------------------------------------------
void BPolygon::MapTo(BRect srcRect, BRect dstRect)
{
	for (int32 i = 0; i < fCount; i++)
		map_pt(fPts + i, srcRect, dstRect);
	map_rect(&fBounds, srcRect, dstRect);
}
//------------------------------------------------------------------------------
void BPolygon::PrintToStream () const
{
	for (int32 i = 0; i < fCount; i++)
		fPts[i].PrintToStream();
}
//------------------------------------------------------------------------------
void BPolygon::compute_bounds()
{
	if (fCount == 0) {
		fBounds = BRect(0.0, 0.0, 0.0, 0.0);
		return;
	}

	fBounds = BRect(fPts[0], fPts[0]);

	for (int32 i = 1; i < fCount; i++)
	{
		if (fPts[i].x < fBounds.left)
			fBounds.left = fPts[i].x;
		if (fPts[i].y < fBounds.top)
			fBounds.top = fPts[i].y;
		if (fPts[i].x > fBounds.right)
			fBounds.right = fPts[i].x;
		if (fPts[i].y > fBounds.bottom)
			fBounds.bottom = fPts[i].y;
	}
}
//------------------------------------------------------------------------------
void BPolygon::map_pt(BPoint *point, BRect srcRect, BRect dstRect)
{
	point->x = (point->x - srcRect.left) * dstRect.Width() / srcRect.Width()
		+ dstRect.left;
	point->y = (point->y - srcRect.top) * dstRect.Height() / srcRect.Height()
		+ dstRect.top;
}
//------------------------------------------------------------------------------
void BPolygon::map_rect(BRect *rect, BRect srcRect, BRect dstRect)
{
	BPoint leftTop = rect->LeftTop();
	BPoint bottomRight = rect->RightBottom();

	map_pt(&leftTop, srcRect, dstRect);
	map_pt(&bottomRight, srcRect, dstRect);

	*rect = BRect(leftTop, bottomRight);
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

