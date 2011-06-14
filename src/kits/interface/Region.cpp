/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <Region.h>

#include <stdlib.h>
#include <string.h>

#include <Debug.h>

#include "clipping.h"
#include "RegionSupport.h"


const static int32 kDataBlockSize = 8;


/*! \brief Initializes a region. The region will have no rects,
	and its fBounds will be invalid.
*/
BRegion::BRegion()
	:
	fCount(0),
	fDataSize(0),
	fBounds((clipping_rect){ 0, 0, 0, 0 }),
	fData(NULL)
{
	_SetSize(kDataBlockSize);
}


/*! \brief Initializes a region to be a copy of another.
	\param region The region to copy.
*/
BRegion::BRegion(const BRegion& region)
	:
	fCount(0),
	fDataSize(0),
	fBounds((clipping_rect){ 0, 0, 0, 0 }),
	fData(NULL)
{
	*this = region;
}


/*!	\brief Initializes a region to contain a BRect.
	\param rect The BRect to set the region to.
*/
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

// NOTE: private constructor
/*!	\brief Initializes a region to contain a clipping_rect.
	\param rect The clipping_rect to set the region to, already in
	internal rect format.
*/
BRegion::BRegion(const clipping_rect& rect)
	: fCount(1)
	, fDataSize(1)
	, fBounds(rect)
	, fData(&fBounds)
{
}


/*!	\brief Frees the allocated memory.
*/
BRegion::~BRegion()
{
	if (fData != &fBounds)
		free(fData);
}


// #pragma mark -


/*!	\brief Modifies the region to be a copy of the given BRegion.
	\param region the BRegion to copy.
	\return This function always returns \c *this.
*/
BRegion&
BRegion::operator=(const BRegion& region)
{
	if (&region == this)
		return *this;

	// handle reallocation if we're too small to contain
	// the other region
	if (_SetSize(region.fDataSize)) {
		memcpy(fData, region.fData, region.fCount * sizeof(clipping_rect));

		fBounds = region.fBounds;
		fCount = region.fCount;
	}

	return *this;
}


/*!	\brief Compares this region to another (by value).
	\param other the BRegion to compare to.
	\return \ctrue if the two regions are the same, \cfalse otherwise.
*/
bool
BRegion::operator==(const BRegion& other) const
{
	if (&other == this)
		return true;

	if (fCount != other.fCount)
		return false;

	return memcmp(fData, other.fData, fCount * sizeof(clipping_rect)) == 0;
}


/*!	\brief Set the region to contain just the given BRect.
	\param newBounds A BRect.
*/
void
BRegion::Set(BRect newBounds)
{
	Set(_Convert(newBounds));
}


/*!	\brief Set the region to contain just the given clipping_rect.
	\param newBounds A clipping_rect.
*/
void
BRegion::Set(clipping_rect newBounds)
{
	_SetSize(1);

	if (valid_rect(newBounds) && fData) {
		fCount = 1;
		// cheap convert to internal rect format
		newBounds.right++;
		newBounds.bottom++;
		fData[0] = fBounds = newBounds;
	} else
		MakeEmpty();
}


// #pragma mark -


/*! \brief Returns the bounds of the region.
	\return A BRect which represents the bounds of the region.
*/
BRect
BRegion::Frame() const
{
	return BRect(fBounds.left, fBounds.top,
		fBounds.right - 1, fBounds.bottom - 1);
}


/*! \brief Returns the bounds of the region as a clipping_rect (which has integer coordinates).
	\return A clipping_rect which represents the bounds of the region.
*/
clipping_rect
BRegion::FrameInt() const
{
	return (clipping_rect){ fBounds.left, fBounds.top,
		fBounds.right - 1, fBounds.bottom - 1 };
}


BRect
BRegion::RectAt(int32 index)
{
	return const_cast<const BRegion*>(this)->RectAt(index);
}


/*! \brief Returns the regions's BRect at the given index.
	\param index The index (zero based) of the wanted rectangle.
	\return If the given index is valid, it returns the BRect at that index,
		otherwise, it returns an invalid BRect.
*/
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


clipping_rect
BRegion::RectAtInt(int32 index)
{
	return const_cast<const BRegion*>(this)->RectAtInt(index);
}


/*! \brief Returns the regions's clipping_rect at the given index.
	\param index The index (zero based) of the wanted rectangle.
	\return If the given index is valid, it returns the clipping_rect at that index,
		otherwise, it returns an invalid clipping_rect.
*/
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


/*!	\brief Counts the region rects.
	\return An int32 which is the total number of rects in the region.
*/
int32
BRegion::CountRects()
{
	return fCount;
}


/*!	\brief Counts the region rects.
	\return An int32 which is the total number of rects in the region.
*/
int32
BRegion::CountRects() const
{
	return fCount;
}


// #pragma mark -


/*!	\brief Check if the region has any area in common with the given BRect.
	\param rect The BRect to check the region against to.
	\return \ctrue if the region has any area in common with the BRect, \cfalse if not.
*/
bool
BRegion::Intersects(BRect rect) const
{
	return Intersects(_Convert(rect));
}


/*!	\brief Check if the region has any area in common with the given clipping_rect.
	\param rect The clipping_rect to check the region against to.
	\return \ctrue if the region has any area in common with the clipping_rect, \cfalse if not.
*/
bool
BRegion::Intersects(clipping_rect rect) const
{
	// cheap convert to internal rect format
	rect.right ++;
	rect.bottom ++;

	int result = Support::XRectInRegion(this, rect);

	return result > Support::RectangleOut;
}


/*!	\brief Check if the region contains the given BPoint.
	\param pt The BPoint to be checked.
	\return \ctrue if the region contains the BPoint, \cfalse if not.
*/
bool
BRegion::Contains(BPoint point) const
{
	return Support::XPointInRegion(this, (int)point.x, (int)point.y);
}


/*!	\brief Check if the region contains the given coordinates.
	\param x The \cx coordinate of the point to be checked.
	\param y The \cy coordinate of the point to be checked.
	\return \ctrue if the region contains the point, \cfalse if not.
*/
bool
BRegion::Contains(int32 x, int32 y)
{
	return Support::XPointInRegion(this, x, y);
}


/*!	\brief Check if the region contains the given coordinates.
	\param x The \cx coordinate of the point to be checked.
	\param y The \cy coordinate of the point to be checked.
	\return \ctrue if the region contains the point, \cfalse if not.
*/
bool
BRegion::Contains(int32 x, int32 y) const
{
	return Support::XPointInRegion(this, x, y);
}


/*!	\brief Prints the BRegion to stdout.
*/
void
BRegion::PrintToStream() const
{
	Frame().PrintToStream();

	for (long i = 0; i < fCount; i++) {
		clipping_rect *rect = &fData[i];
		printf("data[%ld] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
}


// #pragma mark -


/*!	\brief Offsets all region's rects, and bounds by the given values.
	\param dh The horizontal offset.
	\param dv The vertical offset.
*/
void
BRegion::OffsetBy(int32 x, int32 y)
{
	if (x == 0 && y == 0)
		return;

	if (fCount > 0) {
		if (fData != &fBounds) {
			for (long i = 0; i < fCount; i++)
				offset_rect(fData[i], x, y);
		}

		offset_rect(fBounds, x, y);
	}
}


/*!	\brief Empties the region, so that it doesn't include any rect, and invalidates its bounds.
*/
void
BRegion::MakeEmpty()
{
	fBounds = (clipping_rect){ 0, 0, 0, 0 };
	fCount = 0;
}


// #pragma mark -


/*!	\brief Modifies the region, so that it includes the given BRect.
	\param rect The BRect to be included by the region.
*/
void
BRegion::Include(BRect rect)
{
	Include(_Convert(rect));
}


/*!	\brief Modifies the region, so that it includes the given clipping_rect.
	\param rect The clipping_rect to be included by the region.
*/
void
BRegion::Include(clipping_rect rect)
{
	if (!valid_rect(rect))
		return;

	// convert to internal rect format
	rect.right ++;
	rect.bottom ++;

	// use private clipping_rect constructor which avoids malloc()
	BRegion t(rect);

	BRegion result;
	Support::XUnionRegion(this, &t, &result);

	_AdoptRegionData(result);
}


/*!	\brief Modifies the region, so that it includes the area of the given region.
	\param region The region to be included.
*/
void
BRegion::Include(const BRegion* region)
{
	BRegion result;
	Support::XUnionRegion(this, region, &result);

	_AdoptRegionData(result);
}


// #pragma mark -


/*!	\brief Modifies the region, excluding the area represented by the given BRect.
	\param rect The BRect to be excluded.
*/
void
BRegion::Exclude(BRect rect)
{
	Exclude(_Convert(rect));
}


/*!	\brief Modifies the region, excluding the area represented by the given clipping_rect.
	\param rect The clipping_rect to be excluded.
*/
void
BRegion::Exclude(clipping_rect rect)
{
	if (!valid_rect(rect))
		return;

	// convert to internal rect format
	rect.right ++;
	rect.bottom ++;

	// use private clipping_rect constructor which avoids malloc()
	BRegion t(rect);

	BRegion result;
	Support::XSubtractRegion(this, &t, &result);

	_AdoptRegionData(result);
}


/*!	\brief Modifies the region, excluding the area contained in the given
		BRegion.
	\param region The BRegion to be excluded.
*/
void
BRegion::Exclude(const BRegion* region)
{
	BRegion result;
	Support::XSubtractRegion(this, region, &result);

	_AdoptRegionData(result);
}


// #pragma mark -


/*!	\brief Modifies the region, so that it will contain just the area
		in common with the given BRegion.
	\param region the BRegion to intersect with.
*/
void
BRegion::IntersectWith(const BRegion* region)
{
	BRegion result;
	Support::XIntersectRegion(this, region, &result);

	_AdoptRegionData(result);
}


// #pragma mark -


/*!	\brief Modifies the region, so that it will contain just the area
		which both regions do not have in common.
	\param region the BRegion to exclusively include.
*/
void
BRegion::ExclusiveInclude(const BRegion* region)
{
	BRegion result;
	Support::XXorRegion(this, region, &result);

	_AdoptRegionData(result);
}


// #pragma mark -


/*!	\brief Takes over the data of a region and marks that region empty.
	\param region The region to adopt the data from.
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


/*!	\brief Reallocate the memory in the region.
	\param newSize The amount of rectangles that the region should be
		able to hold.
*/
bool
BRegion::_SetSize(long newSize)
{
	// we never shrink the size
	newSize = max_c(fDataSize, newSize);
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

