#include <string.h>
#include <stdlib.h>

#include "clipping.h"
#include "ClipRegion.h"
//#include <ServerLink.h>

#include <Debug.h>

using namespace std;

const static int32 kMaxPoints = 512;
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
	if (rect_contains(fBound, rect))
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
		
	else if (fBound.left > region.fBound.right) {
		ClipRegion dest(region);		
		dest._IncludeNoIntersection(*this);
		_Adopt(dest);

	} else if (region.fBound.left > fBound.right)
		_IncludeNoIntersection(region);
		
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

#if 0
status_t
ClipRegion::ReadFromLink(BPrivate::ServerLink &link)
{
	link.Read(&fCount, sizeof(fCount));
	link.Read(&fBound, sizeof(fBound));
	_Resize(fCount + 1);
	return link.Read(fData, fCount * sizeof(clipping_rect));
}


status_t
ClipRegion::WriteToLink(BPrivate::ServerLink &link)
{
	link.Attach(&fCount, sizeof(fCount));
	link.Attach(&fBound, sizeof(fBound));
	return link.Attach(fData, fCount * sizeof(clipping_rect));
}
#endif

// private methods
static int
leftComparator(const void *a, const void *b)
{
	return ((clipping_rect *)(a))->left > ((clipping_rect *)(b))->left;
}


static int
topComparator(const void *a, const void *b)
{
	return ((clipping_rect *)(a))->top > ((clipping_rect *)(b))->top;
}


void
ClipRegion::_IntersectWithComplex(const ClipRegion &region)
{
	ClipRegion dest;
			
	for (int32 f = 0; f < fCount; f++) {
		for (int32 s = 0; s < region.fCount; s++) {
			clipping_rect intersection = sect_rect(fData[f], region.fData[s]);
			if (valid_rect(intersection))
				dest._AddRect(intersection);
		}
	}
	
	if (dest.fCount > 1)
		qsort(dest.fData, sizeof(clipping_rect), dest.fCount, topComparator);

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
	if (rect_contains(rect, fBound))
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
ClipRegion::_IncludeNoIntersection(const ClipRegion &region)
{
	ASSERT(fCount != 0 || region.fCount != 0);
	ASSERT(!rects_intersect(region.fBound, fBound));

	int32 first = 0, second = 0;
	
	ClipRegion dest;
	// Add the rects in the right order, so that
	// they are kept sorted by their top coordinate
	while (first < fCount && second < region.fCount) {
		if (fData[first].top < region.fData[second].top)
			dest._AddRect(fData[first++]);
		else
			dest._AddRect(region.fData[second++]);
	} 		
	
	if (first == fCount) {
		while (second < region.fCount)
			dest._AddRect(region.fData[second++]);

	} else if (second == region.fCount) {
		while (first < fCount)
			dest._AddRect(fData[first++]);
	}
	
	_Adopt(dest);

	ASSERT(first == fCount && second == region.fCount);
}



void
ClipRegion::_IncludeComplex(const ClipRegion &region)
{
	int32 bottom = min_c(fBound.top, region.fBound.top) - 1;
	int32 a = 0, b = 0;
	ClipRegion dest;
	while (true) {
		int32 top = bottom + 1;
		bottom = region._FindSmallestBottom(_FindSmallestBottom(top, a), b);
		
		if (bottom == kMaxVerticalExtent)
			break;
		
		clipping_rect rects[kMaxPoints];
		rects[0].top = top;
		rects[0].bottom = bottom;		

		int32 rectsCount = _ExtractStripRects(top, bottom, rects, &a)
				+ region._ExtractStripRects(top, bottom, &rects[rectsCount], &b); 
		
		if (rectsCount > 0) {
			if (rectsCount > 1)
				qsort(rects, sizeof(clipping_rect), rectsCount, leftComparator);
			dest._MergeAndInclude(rects, rectsCount);
		}
	}

	dest._Coalesce();
	_Adopt(dest);
	
}


void
ClipRegion::_ExcludeComplex(const ClipRegion &region)
{
/*
	int32 bottom = min_c(fBound.top, region.fBound.top) - 1;
	int32 a = 0, b = 0;
	ClipRegion dest;
	while (true) {
		int32 top = bottom + 1;
		bottom = region._FindSmallestBottom(_FindSmallestBottom(top, a), b);
		
		if (bottom == kMaxVerticalExtent)
			break;
		
		clipping_rect rectsA[kMaxPoints];
		rectsA[0].top = top;
		rectsA[0].bottom = bottom;		
		
		clipping_rect rectsB[kMaxPoints];
		rectsB[0].top = top;
		rectsB[0].bottom = bottom;	
		
		int32 foundA = _ExtractStripRects(top, bottom, rectsA, &a);
		int32 foundB = region._ExtractStripRects(top, bottom, rectsB, &b); 
		
		if (foundA > 1)
			qsort(rectsA, sizeof(clipping_rect), foundA, leftComparator);
		
		if (foundB > 1)
			qsort(rectsB, sizeof(clipping_rect), foundB, leftComparator);
		
		// TODO: Do the actual exclusion
		
	}

	dest._Coalesce();
	_Adopt(dest);
*/
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
		const int32 last = fCount - 1;
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
		if (fDataSize <= fCount && !_Resize(fCount + 16))
			return;
			
		fData[fCount] = rect;
		fCount++;
	}
	
	_RecalculateBounds(rect);
}


void
ClipRegion::_RecalculateBounds(const clipping_rect &rect)
{
	ASSERT(fCount > 0);
		
	if (rect.top < fBound.top)
		fBound.top = rect.top;
	if (rect.left < fBound.left)
		fBound.left = rect.left;
	if (rect.right > fBound.right)
		fBound.right = rect.right;
	if (rect.bottom > fBound.bottom)
		fBound.bottom = rect.bottom;
}


int32
ClipRegion::_FindSmallestBottom(const int32 &top, const int32 &startIndex) const
{
	int32 bottom = kMaxVerticalExtent;
	for (int32 i = startIndex; i < fCount; i++) {
		const int32 n = fData[i].top - 1;
		if (n >= top && n < bottom)
			bottom = n;
		if (fData[i].bottom >= top && fData[i].bottom < bottom)
			bottom = fData[i].bottom;
	}
	
	return bottom;
}


int32
ClipRegion::_ExtractStripRects(const int32 &top, const int32 &bottom, clipping_rect *rects, int32 *inOutIndex) const
{
	int32 currentIndex = *inOutIndex;
	*inOutIndex = -1;

	int32 foundCount = 0;
	// Store left and right points to the appropriate array
	for (int32 x = currentIndex; x < fCount; x++) {
		// Look if this rect can be used next time we are called, 
		// thus correctly maintaining the "index" parameters.
		if (fData[x].bottom >= top && *inOutIndex == -1)
			*inOutIndex = x;
		
		if (fData[x].top <= top && fData[x].bottom >= bottom) {
			rects[foundCount].left = fData[x].left;
			rects[foundCount].right = fData[x].right;
			foundCount++;	
		} else if (fData[x].top > bottom)
			break;	
	}
	
	if (*inOutIndex == -1)
		*inOutIndex = currentIndex;

	return foundCount;
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
ClipRegion::_MergeAndInclude(clipping_rect *rects, const int32 &count)
{
	// rects share the top and bottom coordinates,
	// so we can get those from the first one and ignore the others
	clipping_rect rect;
	rect.top = rects[0].top;
	rect.bottom = rects[0].bottom;
	
	// Check if a rect intersects with the next one.
	// If so, merge the two rects, if not, just add the rect.
	int32 current = 0;
	while (current < count) {
		int32 next = current + 1;
		
		rect.left = rects[current].left;
		rect.right = rects[current].right;
		
		while (next < count && rect.right >= rects[next].left) {
			if (rect.right < rects[next].right)
				rect.right = rects[next].right;
			next++;
		}
		
		_AddRect(rect);		
		current = next;	
	}
}


inline void
ClipRegion::_Coalesce()
{
	if (fCount > 1) {
		while (_CoalesceVertical() || _CoalesceHorizontal())
			;
	}
}


bool
ClipRegion::_CoalesceVertical()
{
	int32 oldCount = fCount;
	
	clipping_rect testRect = { 1, 1, -1, -1 };	
	int32 newCount = -1;
	
	// First, try coalescing rects vertically	
	for (int32 x = 0; x < fCount; x++) {
		clipping_rect &rect = fData[x];
					
		if (rect.left == testRect.left && rect.right == testRect.right
				&& rect.top == testRect.bottom + 1)
			fData[newCount].bottom = rect.bottom;
		else {
			newCount++;
			fData[newCount] = fData[x];			
		}
		testRect = fData[newCount];
	}
	
	fCount = newCount + 1;
	
	return fCount < oldCount;
}


bool
ClipRegion::_CoalesceHorizontal()
{
	int32 oldCount = fCount;
	
	clipping_rect testRect = { 1, 1, -1, -1 };	
	int32 newCount = -1;

	for (int32 x = 0; x < fCount; x++) {
		clipping_rect &rect = fData[x];
		
		if (rect.top == testRect.top && rect.bottom == testRect.bottom
					&& rect.left == testRect.right + 1)	
			fData[newCount].right = rect.right;					
		else {
			newCount++;
			fData[newCount] = rect;
		}
		testRect = fData[newCount];			
	}

	fCount = newCount + 1;

	return fCount < oldCount;
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


