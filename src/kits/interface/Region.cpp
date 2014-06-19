/*
 * Copyright 2003-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Stefano Ceccherini, burton666@libero.it
 */


#include <Region.h>

#include <stdlib.h>
#include <string.h>

#include <Debug.h>

#include "clipping.h"
#include "RegionSupport.h"


const static int32 kDataBlockSize = 8;


// Initializes an empty region.
BRegion::BRegion()
	:
	fCount(0),
	fDataSize(0),
	fBounds((clipping_rect){ 0, 0, 0, 0 }),
	fData(NULL)
{
	_SetSize(kDataBlockSize);
}


// Initializes a region to be a copy of another.
BRegion::BRegion(const BRegion& other)
	:
	fCount(0),
	fDataSize(0),
	fBounds((clipping_rect){ 0, 0, 0, 0 }),
	fData(NULL)
{
	*this = other;
}


// Initializes a region to contain a BRect.
BRegion::BRegion(const BRect rect)
	:
	fCount(0),
	fDataSize(1),
	fBounds((clipping_rect){ 0, 0, 0, 0 }),
	fData(&fBounds)
{
	if (!rect.IsValid())
		return;

	fBounds = _ConvertToInternal(rect);
	fCount = 1;
}


// Initializes a region to contain a clipping_rect.
// NOTE: private constructor
BRegion::BRegion(const clipping_rect& clipping)
	:
	fCount(1),
	fDataSize(1),
	fBounds(clipping),
	fData(&fBounds)
{
}


BRegion::~BRegion()
{
	if (fData != &fBounds)
		free(fData);
}


// Modifies the region to be a copy of the given BRegion.
BRegion&
BRegion::operator=(const BRegion& other)
{
	if (&other == this)
		return *this;

	// handle reallocation if we're too small to contain
	// the other other
	if (_SetSize(other.fDataSize)) {
		memcpy(fData, other.fData, other.fCount * sizeof(clipping_rect));

		fBounds = other.fBounds;
		fCount = other.fCount;
	}

	return *this;
}


// Compares this region to another (by value).
bool
BRegion::operator==(const BRegion& other) const
{
	if (&other == this)
		return true;

	if (fCount != other.fCount)
		return false;

	return memcmp(fData, other.fData, fCount * sizeof(clipping_rect)) == 0;
}


// Set the region to contain just the given BRect.
void
BRegion::Set(BRect rect)
{
	Set(_Convert(rect));
}


//Set the region to contain just the given clipping_rect.
void
BRegion::Set(clipping_rect clipping)
{
	_SetSize(1);

	if (valid_rect(clipping) && fData != NULL) {
		fCount = 1;
		// cheap convert to internal rect format
		clipping.right++;
		clipping.bottom++;
		fData[0] = fBounds = clipping;
	} else
		MakeEmpty();
}


// Returns the bounds of the region.
BRect
BRegion::Frame() const
{
	return BRect(fBounds.left, fBounds.top,
		fBounds.right - 1, fBounds.bottom - 1);
}


// Returns the bounds of the region as a clipping_rect
// (which has integer coordinates).
clipping_rect
BRegion::FrameInt() const
{
	return (clipping_rect){ fBounds.left, fBounds.top,
		fBounds.right - 1, fBounds.bottom - 1 };
}


// Returns the rect contained in the region at the given index.
BRect
BRegion::RectAt(int32 index)
{
	return const_cast<const BRegion*>(this)->RectAt(index);
}


// Returns the rect contained in the region at the given index. (const)
BRect
BRegion::RectAt(int32 index) const
{
	if (index >= 0 && index < fCount) {
		const clipping_rect& r = fData[index];
		return BRect(r.left, r.top, r.right - 1, r.bottom - 1);
	}

	return BRect();
		// an invalid BRect
}


// Returns the clipping_rect contained in the region at the given index.
clipping_rect
BRegion::RectAtInt(int32 index)
{
	return const_cast<const BRegion*>(this)->RectAtInt(index);
}


// Returns the clipping_rect contained in the region at the given index.
clipping_rect
BRegion::RectAtInt(int32 index) const
{
	if (index >= 0 && index < fCount) {
		const clipping_rect& r = fData[index];
		return (clipping_rect){ r.left, r.top, r.right - 1, r.bottom - 1 };
	}

	return (clipping_rect){ 1, 1, 0, 0 };
		// an invalid clipping_rect
}


// Returns the number of rects contained in the region.
int32
BRegion::CountRects()
{
	return fCount;
}


// Returns the number of rects contained in the region.
int32
BRegion::CountRects() const
{
	return fCount;
}


// Check if the region has any area in common with the given BRect.
bool
BRegion::Intersects(BRect rect) const
{
	return Intersects(_Convert(rect));
}


// Check if the region has any area in common with the given clipping_rect.
bool
BRegion::Intersects(clipping_rect clipping) const
{
	// cheap convert to internal rect format
	clipping.right++;
	clipping.bottom++;

	int result = Support::XRectInRegion(this, clipping);

	return result > Support::RectangleOut;
}


// Check if the region contains the given BPoint.
bool
BRegion::Contains(BPoint point) const
{
	return Support::XPointInRegion(this, (int)point.x, (int)point.y);
}


// Check if the region contains the given coordinates.
bool
BRegion::Contains(int32 x, int32 y)
{
	return Support::XPointInRegion(this, x, y);
}


// Check if the region contains the given coordinates.
bool
BRegion::Contains(int32 x, int32 y) const
{
	return Support::XPointInRegion(this, x, y);
}


// Prints the BRegion to stdout.
void
BRegion::PrintToStream() const
{
	Frame().PrintToStream();

	for (int32 i = 0; i < fCount; i++) {
		clipping_rect *rect = &fData[i];
		printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
}


void
BRegion::OffsetBy(const BPoint& point)
{
	OffsetBy(point.x, point.y);
}


// Applies the given x and y offsets to each rect contained by
// the region and recalculates the region's bounds.
void
BRegion::OffsetBy(int32 x, int32 y)
{
	if (x == 0 && y == 0)
		return;

	if (fCount > 0) {
		if (fData != &fBounds) {
			for (int32 i = 0; i < fCount; i++)
				offset_rect(fData[i], x, y);
		}

		offset_rect(fBounds, x, y);
	}
}


void
BRegion::ScaleBy(BSize scale)
{
	ScaleBy(scale.Width(), scale.Height());
}


void
BRegion::ScaleBy(float x, float y)
{
	if (x == 1.0 && y == 1.0)
		return;

	if (fCount > 0) {
		if (fData != &fBounds) {
			for (int32 i = 0; i < fCount; i++)
				scale_rect(fData[i], x, y);
		}

		scale_rect(fBounds, x, y);
	}
}


// Empties the region, so that it doesn't include any rect, and invalidates
// its bounds.
void
BRegion::MakeEmpty()
{
	fBounds = (clipping_rect){ 0, 0, 0, 0 };
	fCount = 0;
}


// Modifies the region, so that it includes the given BRect.
void
BRegion::Include(BRect rect)
{
	Include(_Convert(rect));
}


// Modifies the region, so that it includes the given clipping_rect.
void
BRegion::Include(clipping_rect clipping)
{
	if (!valid_rect(clipping))
		return;

	// convert to internal clipping format
	clipping.right++;
	clipping.bottom++;

	// use private clipping_rect constructor which avoids malloc()
	BRegion temp(clipping);

	BRegion result;
	Support::XUnionRegion(this, &temp, &result);

	_AdoptRegionData(result);
}


// Modifies the region, so that it includes the area of the given region.
void
BRegion::Include(const BRegion* region)
{
	BRegion result;
	Support::XUnionRegion(this, region, &result);

	_AdoptRegionData(result);
}


/*!	\brief Modifies the region, excluding the area represented by the given BRect.
	\param rect The BRect to be excluded.
*/
void
BRegion::Exclude(BRect rect)
{
	Exclude(_Convert(rect));
}


// Modifies the region, excluding the area represented by the given clipping_rect.
void
BRegion::Exclude(clipping_rect clipping)
{
	if (!valid_rect(clipping))
		return;

	// convert to internal clipping format
	clipping.right++;
	clipping.bottom++;

	// use private clipping_rect constructor which avoids malloc()
	BRegion temp(clipping);

	BRegion result;
	Support::XSubtractRegion(this, &temp, &result);

	_AdoptRegionData(result);
}


// Modifies the region, excluding the area contained in the given BRegion.
void
BRegion::Exclude(const BRegion* region)
{
	BRegion result;
	Support::XSubtractRegion(this, region, &result);

	_AdoptRegionData(result);
}


// Modifies the region, so that it will contain only the area in common
// with the given BRegion.
void
BRegion::IntersectWith(const BRegion* region)
{
	BRegion result;
	Support::XIntersectRegion(this, region, &result);

	_AdoptRegionData(result);
}


// Modifies the region, so that it will contain just the area which both
// regions do not have in common.
void
BRegion::ExclusiveInclude(const BRegion* region)
{
	BRegion result;
	Support::XXorRegion(this, region, &result);

	_AdoptRegionData(result);
}


//	#pragma mark - BRegion private methods


/*!
	\fn void BRegion::_AdoptRegionData(BRegion& region)
	\brief Takes over the data of \a region and empties it.

	\param region The \a region to adopt data from.
*/
void
BRegion::_AdoptRegionData(BRegion& region)
{
	fCount = region.fCount;
	fDataSize = region.fDataSize;
	fBounds = region.fBounds;
	if (fData != &fBounds)
		free(fData);
	if (region.fData != &region.fBounds)
		fData = region.fData;
	else
		fData = &fBounds;

	// NOTE: MakeEmpty() is not called since _AdoptRegionData is only
	// called with internally allocated regions, so they don't need to
	// be left in a valid state.
	region.fData = NULL;
//	region.MakeEmpty();
}


/*!
	\fn bool BRegion::_SetSize(int32 newSize)
	\brief Reallocate the memory in the region.

	\param newSize The amount of rectangles that the region should be
		able to hold.
*/
bool
BRegion::_SetSize(int32 newSize)
{
	// we never shrink the size
	newSize = max_c(fDataSize, newSize);
		// The amount of rectangles that the region should be able to hold.
	if (newSize == fDataSize)
		return true;

	// align newSize to multiple of kDataBlockSize
	newSize = ((newSize + kDataBlockSize - 1) / kDataBlockSize) * kDataBlockSize;

	if (newSize > 0) {
		if (fData == &fBounds) {
			fData = (clipping_rect*)malloc(newSize * sizeof(clipping_rect));
			fData[0] = fBounds;
		} else if (fData) {
			clipping_rect* resizedData = (clipping_rect*)realloc(fData,
				newSize * sizeof(clipping_rect));
			if (!resizedData) {
				// failed to resize, but we cannot keep the
				// previous state of the object
				free(fData);
				fData = NULL;
			} else
				fData = resizedData;
		} else
			fData = (clipping_rect*)malloc(newSize * sizeof(clipping_rect));
	} else {
		// just an empty region, but no error
		MakeEmpty();
		return true;
	}

	if (!fData) {
		// allocation actually failed
		fDataSize = 0;
		MakeEmpty();
		return false;
	}

	fDataSize = newSize;
	return true;
}


clipping_rect
BRegion::_Convert(const BRect& rect) const
{
	return (clipping_rect){ (int)floorf(rect.left), (int)floorf(rect.top),
		(int)ceilf(rect.right), (int)ceilf(rect.bottom) };
}


clipping_rect
BRegion::_ConvertToInternal(const BRect& rect) const
{
	return (clipping_rect){ (int)floorf(rect.left), (int)floorf(rect.top),
		(int)ceilf(rect.right) + 1, (int)ceilf(rect.bottom) + 1 };
}


clipping_rect
BRegion::_ConvertToInternal(const clipping_rect& rect) const
{
	return (clipping_rect){ rect.left, rect.top,
		rect.right + 1, rect.bottom + 1 };
}
