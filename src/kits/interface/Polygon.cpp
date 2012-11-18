/*
 * Copyright 2001-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 *		Marcus Overhagen
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <Polygon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <AffineTransform.h>


// Limit to avoid integer overflow when calculating the size to allocate
#define MAX_POINT_COUNT 10000000


BPolygon::BPolygon(const BPoint* points, int32 count)
	:
	fBounds(0.0f, 0.0f, -1.0f, -1.0f),
	fCount(0),
	fPoints(NULL)
{
	_AddPoints(points, count, true);
}


BPolygon::BPolygon(const BPolygon& other)
	:
	fBounds(0.0f, 0.0f, -1.0f, -1.0f),
	fCount(0),
	fPoints(NULL)
{
	*this = other;
}


BPolygon::BPolygon(const BPolygon* other)
	:
	fBounds(0.0f, 0.0f, -1.0f, -1.0f),
	fCount(0),
	fPoints(NULL)
{
	*this = *other;
}


BPolygon::BPolygon()
	:
	fBounds(0.0f, 0.0f, -1.0f, -1.0f),
	fCount(0),
	fPoints(NULL)
{
}


BPolygon::~BPolygon()
{
	free(fPoints);
}


BPolygon&
BPolygon::operator=(const BPolygon& other)
{
	// Make sure we aren't trying to perform a "self assignment".
	if (this == &other)
		return *this;

	free(fPoints);
	fPoints = NULL;
	fCount = 0;
	fBounds.Set(0.0f, 0.0f, -1.0f, -1.0f);

	if (_AddPoints(other.fPoints, other.fCount, false))
		fBounds = other.fBounds;

	return *this;
}


BRect
BPolygon::Frame() const
{
	return fBounds;
}


void
BPolygon::AddPoints(const BPoint* points, int32 count)
{
	_AddPoints(points, count, true);
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


//void
//BPolygon::TransformBy(const BAffineTransform& transform)
//{
//	transform.Apply(fPoints, (int32)fCount);
//	_ComputeBounds();
//}
//
//
//BPolygon&
//BPolygon::TransformBySelf(const BAffineTransform& transform)
//{
//	TransformBy(transform);
//	return *this;
//}
//
//
//BPolygon
//BPolygon::TransformByCopy(const BAffineTransform& transform) const
//{
//	BPolygon copy(this);
//	copy.TransformBy(transform);
//	return copy;
//}


// #pragma mark -


bool
BPolygon::_AddPoints(const BPoint* points, int32 count, bool computeBounds)
{
	if (points == NULL || count <= 0)
		return false;
	if (count > MAX_POINT_COUNT || (fCount + count) > MAX_POINT_COUNT) {
		fprintf(stderr, "BPolygon::_AddPoints(%" B_PRId32 ") - too many points"
			"\n", count);
		return false;
	}

	BPoint* newPoints = (BPoint*)realloc(fPoints, (fCount + count)
		* sizeof(BPoint));
	if (newPoints == NULL) {
		fprintf(stderr, "BPolygon::_AddPoints(%" B_PRId32 ") out of memory\n",
			count);
		return false;
	}

	fPoints = newPoints;
	memcpy(fPoints + fCount, points, count * sizeof(BPoint));
	fCount += count;

	if (computeBounds)
		_ComputeBounds();

	return true;
}


void
BPolygon::_ComputeBounds()
{
	if (fCount == 0) {
		fBounds = BRect(0.0, 0.0, -1.0f, -1.0f);
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
BPolygon::_MapPoint(BPoint* point, const BRect& srcRect, const BRect& dstRect)
{
	point->x = (point->x - srcRect.left) * dstRect.Width() / srcRect.Width()
		+ dstRect.left;
	point->y = (point->y - srcRect.top) * dstRect.Height() / srcRect.Height()
		+ dstRect.top;
}


void
BPolygon::_MapRectangle(BRect* rect, const BRect& srcRect,
	const BRect& dstRect)
{
	BPoint leftTop = rect->LeftTop();
	BPoint bottomRight = rect->RightBottom();

	_MapPoint(&leftTop, srcRect, dstRect);
	_MapPoint(&bottomRight, srcRect, dstRect);

	*rect = BRect(leftTop, bottomRight);
}
