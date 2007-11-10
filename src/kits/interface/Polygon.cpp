/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 *		Marcus Overhagen
 */


#include <Polygon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Limit to avoid integer overflow when calculating the size to allocate
#define MAX_POINT_COUNT 10000000


BPolygon::BPolygon(const BPoint *ptArray, int32 numPoints)
	:
	fBounds(0.0, 0.0, 0.0, 0.0),
	fCount(numPoints),
	fPoints(NULL)
{
	if (fCount) {
		if (fCount > MAX_POINT_COUNT)
			debugger("BPolygon::BPolygon too many points");

		// Note the use of memcpy here.  The assumption is that an array of BPoints can
		// be copied bit by bit and not use a copy constructor or an assignment
		// operator.  This breaks the containment of BPoint but will result in better
		// performance.  An example where the memcpy will fail would be if BPoint begins
		// to do lazy copying through reference counting.  By copying the bits, we will
		// copy reference counting state which will not be relevant at the destination.
		// Luckily, BPoint is a very simple class which isn't likely to change much.
		//
		// Similar use of memcpy appears later in this implementation also.
		size_t size = fCount * sizeof(BPoint);
		fPoints = (BPoint *)malloc(size);
		if (!fPoints) {
			fprintf(stderr, "BPolygon::BPolygon out of memory\n");
			fCount = 0;
			return;
		}
		memcpy(fPoints, ptArray, size);
		_ComputeBounds();
	}
}


BPolygon::BPolygon(const BPolygon *poly)
	:
	fBounds(0.0, 0.0, 0.0, 0.0),
	fCount(0),
	fPoints(NULL)
{
	*this = *poly;
}


BPolygon::BPolygon()
	:
	fBounds(0.0, 0.0, 0.0, 0.0),
	fCount(0),
	fPoints(NULL)
{
}


BPolygon::~BPolygon()
{
	free(fPoints);
}


BPolygon &
BPolygon::operator=(const BPolygon &from)
{
	// Make sure we aren't trying to perform a "self assignment".
	if (this != &from) {
		free(fPoints);		
		fBounds = from.fBounds;
		fCount = from.fCount;
		if (fCount) {
			if (fCount > MAX_POINT_COUNT)
				debugger("BPolygon::operator= too many points");
			fPoints = (BPoint *)malloc(fCount * sizeof(BPoint));
			if (!fPoints) {
				fprintf(stderr, "BPolygon::operator= out of memory\n");
				fCount = 0;
			} else {
				memcpy(fPoints, from.fPoints, fCount * sizeof(BPoint));
			}
		} else {
			fPoints = NULL;
		}
	}
	return *this;
}


BRect
BPolygon::Frame() const
{
	return fBounds;
}


void
BPolygon::AddPoints(const BPoint *ptArray, int32 numPoints)
{
	if (numPoints < 0)
		debugger("BPolygon::AddPoints negative points");
	if (numPoints > MAX_POINT_COUNT || (fCount + numPoints) > MAX_POINT_COUNT)
		debugger("BPolygon::AddPoints too many points");

	if (numPoints > 0) {
		BPoint *points = (BPoint *)realloc(fPoints, (fCount + numPoints) * sizeof(BPoint));
		if (!points) {
			fprintf(stderr, "BPolygon::AddPoints out of memory\n");
		} else {
			fPoints = points;
			memcpy(fPoints + fCount, ptArray, numPoints * sizeof(BPoint));
			fCount += numPoints;
			_ComputeBounds();
		}
	}
}


int32
BPolygon::CountPoints() const
{
	return fCount;
}


void
BPolygon::MapTo(BRect srcRect, BRect dstRect)
{
	for (uint32 i = 0; i < fCount; i++)
		_MapPoint(fPoints + i, srcRect, dstRect);
	_MapRectangle(&fBounds, srcRect, dstRect);
}


void
BPolygon::PrintToStream () const
{
	for (uint32 i = 0; i < fCount; i++)
		fPoints[i].PrintToStream();
}


void
BPolygon::_ComputeBounds()
{
	if (fCount == 0) {
		fBounds = BRect(0.0, 0.0, 0.0, 0.0);
		return;
	}

	fBounds = BRect(fPoints[0], fPoints[0]);

	for (uint32 i = 1; i < fCount; i++) {
		if (fPoints[i].x < fBounds.left)
			fBounds.left = fPoints[i].x;
		if (fPoints[i].y < fBounds.top)
			fBounds.top = fPoints[i].y;
		if (fPoints[i].x > fBounds.right)
			fBounds.right = fPoints[i].x;
		if (fPoints[i].y > fBounds.bottom)
			fBounds.bottom = fPoints[i].y;
	}
}


void
BPolygon::_MapPoint(BPoint *point, BRect srcRect, BRect dstRect)
{
	point->x = (point->x - srcRect.left) * dstRect.Width() / srcRect.Width()
		+ dstRect.left;
	point->y = (point->y - srcRect.top) * dstRect.Height() / srcRect.Height()
		+ dstRect.top;
}


void
BPolygon::_MapRectangle(BRect *rect, BRect srcRect, BRect dstRect)
{
	BPoint leftTop = rect->LeftTop();
	BPoint bottomRight = rect->RightBottom();

	_MapPoint(&leftTop, srcRect, dstRect);
	_MapPoint(&bottomRight, srcRect, dstRect);

	*rect = BRect(leftTop, bottomRight);
}
