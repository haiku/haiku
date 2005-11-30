#include <string.h>
#include <stdlib.h>

#include "clipping.h"
#include "ClipRegion.h"

#include <Debug.h>

const static int32 kMaxPoints = 1024;
const static int32 kMaxVerticalExtent = 0x10000000;
const static int32 kMaxPositive = 0x7ffffffd;
const static int32 kMaxNegative = 0x80000003;
const static int32 kInitialDataSize = 8;


ClipRegion::ClipRegion()
	:
	fDataSize(0),
	fData(NULL)	
{
	_Resize(kInitialDataSize);
	_Invalidate();
}


ClipRegion::ClipRegion(const ClipRegion &region)
	:
	fDataSize(0),
	fData(NULL)	
{
	*this = region;
}


ClipRegion::ClipRegion(const clipping_rect &rect)
	:
	fDataSize(0),
	fData(NULL)	
{
	Set(rect);
}


ClipRegion::ClipRegion(const BRect &rect)
	:
	fDataSize(0),
	fData(NULL)	
{
	Set(rect);
}


ClipRegion::ClipRegion(const clipping_rect *rects, const int32 &count)
	:
	fDataSize(0),
	fData(NULL)	
{
	Set(rects, count);
}


ClipRegion::~ClipRegion()
{
	free(fData);
}


void
ClipRegion::Set(const clipping_rect &rect)
{
	if (!valid_rect(rect) || (fDataSize <= 1 && !_Resize(kInitialDataSize)))
		_Invalidate();
	else {
		fCount = 1;
		fData[0] = fBound = rect;
	}
}


void
ClipRegion::Set(const BRect &rect)
{
	Set(to_clipping_rect(rect));
}


void
ClipRegion::Set(const clipping_rect *rects, const int32 &count)
{
	_Invalidate();
	_Append(rects, count);
}


bool
ClipRegion::Intersects(const clipping_rect &rect) const
{
	if (!rects_intersect(rect, fBound))
		return false;

	for (int32 c = 0; c < fCount; c++) {
		if (rects_intersect(fData[c], rect))
			return true;
	}
	
	return false;
}


bool
ClipRegion::Intersects(const BRect &rect) const
{
	return Intersects(to_clipping_rect(rect));
}


bool
ClipRegion::Contains(const BPoint &pt) const
{
	// If the point doesn't lie within the region's bounds,
	// don't even try it against the region's rects.
	if (!point_in(fBound, pt))
		return false;

	for (int32 c = 0; c < fCount; c++) {
		if (point_in(fData[c], pt))
			return true;
	}
	
	return false;
}


void
ClipRegion::OffsetBy(const int32 &dh, const int32 &dv)
{
	if (fCount > 0) {
		for (int32 c = 0; c < fCount; c++)
			offset_rect(fData[c], dh, dv);

		offset_rect(fBound, dh, dv);	
	}
}


void
ClipRegion::OffsetBy(const BPoint &point)
{
	return OffsetBy((int32)point.x, (int32)point.y);
}


void
ClipRegion::IntersectWith(const clipping_rect &rect)
{
	if (!rects_intersect(rect, fBound))
		_Invalidate();
	else
		_IntersectWith(rect);	
}


void
ClipRegion::IntersectWith(const BRect &rect)
{
	IntersectWith(to_clipping_rect(rect));
}

	
void
ClipRegion::IntersectWith(const ClipRegion &region)
{
	clipping_rect intersection = sect_rect(fBound, region.fBound);
	
	if (fCount == 0 || region.fCount == 0 || !valid_rect(intersection))
		_Invalidate();
		
	else if (fCount == 1 && region.fCount == 1)
		Set(intersection);
		
	else if (fCount > 1 && region.fCount == 1)
		_IntersectWith(region.fBound);
		
	else if (fCount == 1 && region.fCount > 1) {
		ClipRegion dest = region;
		dest._IntersectWith(fBound);
		_Adopt(dest);
	
	} else
		_IntersectWithComplex(region);
}


void
ClipRegion::Include(const clipping_rect &rect)
{
	// The easy case first: if the rectangle completely contains
	// ourself, the union is the rectangle itself.
	if (rect.top <= fBound.top 
			&& rect.bottom >= fBound.bottom
			&& rect.left <= fBound.left
			&& rect.right >= fBound.right)
		Set(rect);
	else {
		const ClipRegion region(rect);
		_IncludeComplex(region);
	}
}


void
ClipRegion::Include(const BRect &rect)
{
	Include(to_clipping_rect(rect));
}


void
ClipRegion::Include(const ClipRegion &region)
{
	if (region.fCount == 0)
		return;
	
	if (fCount == 0)
		*this = region;
		
	else if (region.fBound.top > fBound.bottom)
		_Append(region);
		
	else if (fBound.top > region.fBound.bottom)
		_Append(region, true);
	/*	
	else if (regionA->bound.left > regionB->bound.right)
		OrRegionNoX(*regionB, *regionA, dest);
		
	else if (regionB->bound.left > regionA->bound.right)
		OrRegionNoX(*regionA, *regionB, dest);
	*/	
	else if (region.fCount == 1)
		Include(region.fBound);
		
	else if (fCount == 1) {
		ClipRegion dest = region;
		dest.Include(fBound);
		_Adopt(dest);
	} else 
		_IncludeComplex(region);
}

	
void
ClipRegion::Exclude(const clipping_rect &rect)
{
	const ClipRegion region(rect);
	Exclude(region);
}


void
ClipRegion::Exclude(const BRect &rect)
{
	const ClipRegion region(rect);
	Exclude(region);
}


void
ClipRegion::Exclude(const ClipRegion &region)
{
	if (fCount == 0 || region.fCount == 0 || !rects_intersect(fBound, region.fBound))
		return;

	_ExcludeComplex(region);	
}
	

ClipRegion &
ClipRegion::operator=(const ClipRegion &region)
{
	const int32 size = region.fDataSize > 0 ? region.fDataSize : kInitialDataSize;
	if (_Resize(size, false)) {
		fBound = region.fBound;
		fCount = region.fCount;
		memcpy(fData, region.fData, fCount * sizeof(clipping_rect));
	} else
		_Invalidate();
	
	return *this;
}


// private methods	
void
ClipRegion::_IntersectWithComplex(const ClipRegion &region)
{
	ClipRegion dest;
			
	for (int32 f = 0; f < fCount; f++) {
		for (int32 s = 0; s < region.fCount; s++) {
			clipping_rect testRect = sect_rect(fData[f], region.fData[s]);
			if (valid_rect(testRect))
				dest._AddRect(testRect);
		}
	}
	
	if (dest.fCount > 1)
		dest._SortRects();
	
	_Adopt(dest);
}


void
ClipRegion::_IntersectWith(const clipping_rect &rect)
{
	ASSERT(rects_intersect(rect, fBound));
	
	// The easy case first: We already know that the region intersects,
	// with the passed rect, so we check if the rect completely contains
	// the region.
	// If it's the case, the intersection is exactly the region itself.
	if (rect.top <= fBound.top && rect.bottom >= fBound.bottom
			&& rect.left <= fBound.left && rect.right >= fBound.right)
		return;
	
	// Otherwise, we add the intersections of the region's rects
	// with the passed rect
	ClipRegion dest;
	for (int32 x = 0; x < fCount; x++) {
		clipping_rect testRect = sect_rect(rect, fData[x]);
		if (valid_rect(testRect))
			dest._AddRect(testRect);
	}
	_Adopt(dest);
}


void
ClipRegion::_IncludeComplex(const ClipRegion &region)
{
	
}


void
ClipRegion::_ExcludeComplex(const ClipRegion &region)
{
}


// Helper method to swap two rects
static inline void
SwapRects(clipping_rect &rect, clipping_rect &anotherRect)
{
	clipping_rect tmpRect = rect;
	rect = anotherRect;
	anotherRect = tmpRect;
}


void
ClipRegion::_SortRects()
{
	bool again;
			
	if (fCount == 2) {
		if (fData[0].top > fData[1].top)
			SwapRects(fData[0], fData[1]);
		
	} else if (fCount > 2) {
		do {
			again = false;
			for (int32 c = 1; c < fCount; c++) {
				if (fData[c - 1].top > fData[c].top) {
					SwapRects(fData[c - 1], fData[c]);
					again = true;
				}
			}
		} while (again); 
	}
}


void
ClipRegion::_AddRect(const clipping_rect &rect)
{
	ASSERT(fCount >= 0);
	ASSERT(fDataSize >= 0);
	ASSERT(valid_rect(rect));

	// Should we just reallocate the memory and
	// copy the rect ?
	bool addRect = true; 
		
	if (fCount > 0) {
		// Wait! We could merge the rect with one of the
		// existing rectangles, if it's adiacent.
		// We just check it against the last rectangle, since
		// we are keeping them sorted by their "top" coordinates.
		int32 last = fCount - 1;
		if (rect.left == fData[last].left && rect.right == fData[last].right
				&& rect.top == fData[last].bottom + 1) {
		 
			fData[last].bottom = rect.bottom;
			addRect = false;
		
		} else if (rect.top == fData[last].top && rect.bottom == fData[last].bottom) {			
			if (rect.left == fData[last].right + 1) {

				fData[last].right = rect.right;
				addRect = false;

			} else if (rect.right == fData[last].left - 1) {

				fData[last].left = rect.left;
				addRect = false;
			}	
		}
	}		
	
	// We weren't lucky.... just add the rect as a new one
	if (addRect) {
		if (fDataSize <= fCount)
			_Resize(fCount + 16);
			
		fData[fCount] = rect;
		
		fCount++;
	}
	
	_RecalculateBounds(rect);
}


void
ClipRegion::_RecalculateBounds(const clipping_rect &rect)
{
	if (fCount <= 0)
		return;
		
	if (rect.top < fBound.top)
		fBound.top = rect.top;
	if (rect.left < fBound.left)
		fBound.left = rect.left;
	if (rect.right > fBound.right)
		fBound.right = rect.right;
	if (rect.bottom > fBound.bottom)
		fBound.bottom = rect.bottom;
}


void
ClipRegion::_Append(const ClipRegion &region, const bool &aboveThis)
{
	if (aboveThis) {
		ClipRegion dest = region;
		for (int32 c = 0; c < fCount; c++)
			dest._AddRect(fData[c]);
		_Adopt(dest);
	} else {
		for (int32 c = 0; c < region.fCount; c++)
			_AddRect(region.fData[c]);
	}
}


void
ClipRegion::_Append(const clipping_rect *rects, const int32 &count, const bool &aboveThis)
{
	if (aboveThis) {
		ClipRegion dest;
		dest.Set(rects, count);
		for (int32 c = 0; c < fCount; c++)
			dest._AddRect(fData[c]);
		_Adopt(dest);
	} else {
		for (int32 c = 0; c < count; c++)
			_AddRect(rects[c]);
	}
}


void
ClipRegion::_Invalidate()
{
	fCount = 0;
	fBound.left = kMaxPositive;
	fBound.top = kMaxPositive;
	fBound.right = kMaxNegative;
	fBound.bottom = kMaxNegative;
}


bool
ClipRegion::_Resize(const int32 &newSize, const bool &keepOld)
{
	if (!keepOld) {
		free(fData);
		fData = (clipping_rect *)malloc(newSize * sizeof(clipping_rect));		
	} else
		fData = (clipping_rect *)realloc(fData, newSize * sizeof(clipping_rect));
	
	if (fData == NULL)
		return false;
	
	fDataSize = newSize;
	
	return true;
}


void
ClipRegion::_Adopt(ClipRegion &region)
{
	free(fData);
	fData = region.fData;
	fDataSize = region.fDataSize;
	fCount = region.fCount;
	fBound = region.fBound;
	
	region._Invalidate();
	region.fDataSize = 0;
	region.fData = NULL;
}


