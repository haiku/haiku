//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, OpenBeOS
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
//	File Name:		RegionSupport.cpp
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	Class that does the dirty work for BRegion.
//
//------------------------------------------------------------------------------

// TODO: check for possible performance issue in ROr() and RSub().
// Check if inlining some methods can make us be faster.
  
// Standard Includes -----------------------------------------------------------
#include <cstring>
#include <new>

// System Includes -------------------------------------------------------------
#include <Debug.h>
#include <Region.h>

// Private Includes -------------------------------------------------------------
#include <clipping.h>
#include <RegionSupport.h>

// Constants --------------------------------------------------------------------
static const int32 kMaxPoints = 1024;
static const int32 kMaxVerticalExtent = 0x10000000;
static const int32 kMaxPositive = 0x7ffffffd;
static const int32 kMaxNegative = 0x80000003;


#define TRACE_REGION 0
#define ARGS (const char *, ...)
#if TRACE_REGION
	#define RTRACE(ARGS) printf ARGS
	#define CALLED() printf("%s\n", __PRETTY_FUNCTION__)
#else
	#define RTRACE(ARGS) ;
 	#define CALLED()
#endif

using namespace std;

/*!	\brief zeroes the given region, setting its rect count to 0,
	and invalidating its bound rectangle.
	\param region The region to be zeroed.
*/
void
BRegion::Support::ZeroRegion(BRegion *region)
{
	CALLED();
	
	region->count = 0;
	region->bound.left = kMaxPositive;
	region->bound.top = kMaxPositive;
	region->bound.right = kMaxNegative;
	region->bound.bottom = kMaxNegative;
}


/*!	\brief clear the given region, setting its rect count to 0,
	and setting its bound rectangle to 0xFFFFFFF, 0xFFFFFFF, 0xF0000001, 0xF0000001.
	\param region The region to be cleared.
*/
void
BRegion::Support::ClearRegion(BRegion *region)
{
	CALLED();

	// XXX: What is it used for ?
	// Could be that a cleared region represents an infinite one ?
	
	region->count = 0;
	region->bound.left = 0xfffffff;
	region->bound.top = 0xfffffff;
	region->bound.right = 0xf0000001;
	region->bound.bottom = 0xf0000001;
}


/*!	\brief Copy a region to another.
	\param source The region to be copied.
	\param dest The destination region.
*/
void
BRegion::Support::CopyRegion(BRegion *source, BRegion *dest)
{
	CALLED();
	ASSERT(source);
	ASSERT(dest);
	ASSERT(source != dest);
		
	// If there is not enough memory, allocate
	if (dest->data_size < source->count) {
		free(dest->data);
		dest->data_size = source->count + 8;
		dest->data = (clipping_rect *)malloc(dest->data_size * sizeof(clipping_rect));
	}
	
	dest->count = source->count;
	
	// Copy rectangles and bounds.
	memcpy(dest->data, source->data, source->count * sizeof(clipping_rect));
	dest->bound = source->bound;
}


/*!	\brief Modify the destination region to be the intersection of the two given regions.
	\param first The first region to be intersected.
	\param second The second region to be intersected.
	\param dest The destination region.
	
	This function is a sort of method selector. It checks for some special
	cases, then it calls the appropriate specialized function.
*/
void
BRegion::Support::AndRegion(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	clipping_rect intersection = sect_rect(first->bound, second->bound);
	
	if (first->count == 0 || second->count == 0	|| !valid_rect(intersection))
		ZeroRegion(dest);
	
	else if (first->count == 1 && second->count == 1) {
		dest->data[0] = intersection;
		dest->bound = intersection;
		dest->count = 1;
	} 
	else if (first->count > 1 && second->count == 1)
		AndRegion1ToN(second, first, dest);
	
	else if (first->count == 1 && second->count > 1)
		AndRegion1ToN(first, second, dest);
	
	else
		AndRegionComplex(first, second, dest);		
}


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be merged.
	\param second The second region to be merged.
	\param dest The destination region.
	
	This function is a sort of method selector. It checks for some special
	cases, then it calls the appropriate specialized function.
*/
void
BRegion::Support::OrRegion(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	BRegion *regionA, *regionB;		
	
	// A little trick, to save some work...
	if (first->count != 0) {
		regionA = first;
		regionB = second;
	} else {
		regionA = second;
		regionB = first;
	}
	
	if (regionB->count == 0)
		CopyRegion(regionA, dest);
	else {
		if (regionB->bound.top > regionA->bound.bottom)
			AppendRegion(regionA, regionB, dest);
		
		else if (regionA->bound.top > regionB->bound.bottom)
			AppendRegion(regionB, regionA, dest);
		
		else if (regionA->bound.left > regionB->bound.right)
			OrRegionNoX(regionB, regionA, dest);
		
		else if (regionB->bound.left > regionA->bound.right)
			OrRegionNoX(regionA, regionB, dest);
		
		else if (regionA->count == 1)
			OrRegion1ToN(regionA, regionB, dest);
		
		else if (regionB->count == 1)
			OrRegion1ToN(regionB, regionA, dest);
		
		else 
			OrRegionComplex(regionA, regionB, dest);		
	}
}


/*!	\brief Modify the destination region to be the difference of the two given regions.
	\param first The subtraend region.
	\param second The minuend region.
	\param dest The destination region.
	
	This function is a sort of method selector. It checks for some special
	cases, then it calls the appropriate specialized function.
*/
void
BRegion::Support::SubRegion(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);

	if (first->count == 0)
		ZeroRegion(dest);
		
	else if (second->count == 0	|| !rects_intersect(first->bound, second->bound))
		CopyRegion(first, dest);
		
	else
		SubRegionComplex(second, first, dest);	
}


/*!	\brief Cleanup the region, by merging rects that can be merged.
	\param region The region to be cleaned.
*/
void
BRegion::Support::CleanupRegion(BRegion *region)
{
	CALLED();
	
	long oldCount;
	
	do {
		oldCount = region->count;
		CleanupRegionVertical(region);
		CleanupRegionHorizontal(region);
	} while (region->count < oldCount);
}


/*!	\brief Cleanup the region vertically.
	\param region The region to be cleaned.
*/
void
BRegion::Support::CleanupRegionVertical(BRegion *region)
{
	CALLED();
	
	clipping_rect testRect = { 1, 1, -1, -2 };	
	long newCount = -1;
		
	for (long x = 0; x < region->count; x++) {
		clipping_rect &rect = region->data[x];
					
		if (rect.left == testRect.left && rect.right == testRect.right
				&& rect.top == testRect.bottom + 1) {
			
			ASSERT(newCount >= 0);
			region->data[newCount].bottom = rect.bottom;
					
		} else {
			newCount++;
			region->data[newCount] = region->data[x];
			testRect = region->data[x]; 
		}
	}
	region->count = newCount + 1;
}


/*!	\brief Cleanup the region horizontally.
	\param region The region to be cleaned.
*/
void
BRegion::Support::CleanupRegionHorizontal(BRegion *region)
{
	CALLED();
	
	clipping_rect testRect = { 1, 1, -2, -1 };
	long newCount = -1;

	for (long x = 0; x < region->count; x++) {
		clipping_rect &rect = region->data[x];
		
		if (rect.top == testRect.top && rect.bottom == testRect.bottom
					&& rect.left == testRect.right + 1) {
			
			ASSERT(newCount >= 0);
			region->data[newCount].right = rect.right;
							
		} else {
			newCount++;
			region->data[newCount] = rect;
		}				
		testRect = region->data[newCount];
	}
	region->count = newCount + 1;
}


// Helper method to swap two rects
static inline void
SwapRects(clipping_rect &rect, clipping_rect &anotherRect)
{
	clipping_rect tmpRect;
	tmpRect = rect;
	rect = anotherRect;
	anotherRect = tmpRect;
}


/*!	\brief Sorts the given rects by their top value.
	\param rects A pointer to an array of clipping_rects.
	\param count The number of rectangles in the array.
*/
void
BRegion::Support::SortRects(clipping_rect *rects, long count)
{
	CALLED();
	
	bool again; //flag that tells we changed rects positions
			
	if (count == 2) {
		if (rects[0].top > rects[1].top)
			SwapRects(rects[0], rects[1]);
		
	} else if (count > 2) {
		do {
			again = false;
			for (long c = 1; c < count; c++) {
				if (rects[c - 1].top > rects[c].top) {
					SwapRects(rects[c - 1], rects[c]);
					again = true;
				}
			}
		} while (again); 
	}
}


// Helper methods to swap transition points in two given arrays
static inline void
SwapTrans(long *leftPoints, long *rightPoints, long index1, long index2)
{
	// First, swap the left points
	long tmp = leftPoints[index1];
	leftPoints[index1] = leftPoints[index2];
	leftPoints[index2] = tmp;
	
	// then the right points
	tmp = rightPoints[index1];
	rightPoints[index1] = rightPoints[index2];
	rightPoints[index2] = tmp;	
}


void
BRegion::Support::SortTrans(long *lptr1, long *lptr2, long count)
{
	CALLED();
	
	bool again; //flag that tells we changed trans positions
			
	if (count == 2) {
		if (lptr1[0] > lptr1[1])
			SwapTrans(lptr1, lptr2, 0, 1);
		
	} else if (count > 2) {
		do {
			again = false;
			for (long c = 1; c < count; c++) {
				if (lptr1[c - 1] > lptr1[c]) {
					SwapTrans(lptr1, lptr2, c - 1, c);
					again = true;
				}
			}
		} while (again);	
	}
}


/*!	\brief Copy a region to another, allocating some additional memory in the destination region.
	\param source The region to be copied.
	\param dest The destination region.
	\param count Amount of additional memory to be allocated in the destination region.
*/
void
BRegion::Support::CopyRegionMore(BRegion *source, BRegion *dest, long count)
{
	CALLED();
	ASSERT(source);
	ASSERT(dest);
	ASSERT(source != dest);
	
	// If there is not enough memory, allocate
	if (dest->data_size < source->count) {
		free(dest->data);
		dest->data_size = source->count + count;
		dest->data = (clipping_rect *)malloc(dest->data_size * sizeof(clipping_rect));
	}
	
	dest->count = source->count;
	
	// Copy rectangles and bounds.
	memcpy(dest->data, source->data, source->count * sizeof(clipping_rect));
	dest->bound = source->bound;
}


/*!	\brief Modify the destination region to be the intersection of the two given regions.
	\param first The first region to be intersected.
	\param second The second region to be intersected.
	\param dest The destination region.
	
	Called by and_region() when the intersection is complex.
*/	
void
BRegion::Support::AndRegionComplex(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
		
	ZeroRegion(dest);
			
	for (long f = 0; f < first->count; f++) {
		for (long s = 0; s < second->count; s++) {
			clipping_rect testRect = sect_rect(first->data[f], second->data[s]);
			if (valid_rect(testRect))
				dest->_AddRect(testRect);
		}
	}
	
	if (dest->count > 1)
		SortRects(dest->data, dest->count);
}


/*!	\brief Modify the destination region to be the intersection of the two given regions.
	\param first The first region to be intersected.
	\param second The second region to be intersected.
	\param dest The destination region.
	
	Called by and_region() when one of the two region contains just one rect.
*/	
void
BRegion::Support::AndRegion1ToN(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	// The easy case first: We already know that the regions intersect,
	// so we check if the first region contains the second.
	// If it's the case, the intersection is exactly the second region.
	if (first->bound.top <= second->bound.top
			&& first->bound.bottom >= second->bound.bottom
			&& first->bound.left <= second->bound.left
			&& first->bound.right >= second->bound.right)
		CopyRegion(second, dest);
	else {
	// Otherwise, we check the rect of the first region against the rects
	// of the second, and we add their intersections to the destination region
		ZeroRegion(dest);
		for (long x = 0; x < second->count; x++) {
			clipping_rect testRect = sect_rect(first->data[0], second->data[x]);
			if (valid_rect(testRect))
				dest->_AddRect(testRect);
		}
	}
}


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
	
	This function is called by or_region when the two regions don't intersect,
	and when the second region top coordinate is bigger than first region's bottom
	coordinate.
*/
void
BRegion::Support::AppendRegion(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	CopyRegion(first, dest);

	for (long c = 0; c < second->count; c++)
		dest->_AddRect(second->data[c]);	
}


void
BRegion::Support::ROr(long top, long bottom, BRegion *first, BRegion *second, BRegion *dest, long *indexA, long *indexB)
{
	CALLED();
	
	int32 stackLefts[kMaxPoints];
	int32 stackRights[kMaxPoints];

	int32 *lefts = stackLefts;
	int32 *rights = stackRights;
	
	long i1 = *indexA;
	long i2 = *indexB;
	
	*indexA = -1;
	*indexB = -1;
	
	long foundCount = 0;
	long x = 0;
	
	// allocate arrays on the heap, if the ones one the stack are too small
	int32 *allocatedBuffer = NULL;
	int32 maxCount = first->count - i1 + second->count - i2;
	
	if (maxCount > kMaxPoints) {
		RTRACE(("Stack space isn't sufficient. Allocating %ld bytes on the heap...\n",
				2 * maxCount));
		lefts = allocatedBuffer = new(nothrow) int32[2 * maxCount];
		if (!allocatedBuffer)
			return;
		rights = allocatedBuffer + maxCount;
	}

	// Store left and right points to the appropriate array
	for (x = i1; x < first->count; x++) {
		
		// Look if this rect can be used next time we are called, 
		// thus correctly maintaining the "index" parameters.
		if (first->data[x].bottom >= top && *indexA == -1)
			*indexA = x;
			
		if (first->data[x].top <= top && first->data[x].bottom >= bottom) {
			lefts[foundCount] = first->data[x].left;
			rights[foundCount] = first->data[x].right;
			foundCount++;	
		} else if (first->data[x].top > bottom)
			break;	
	}
	
	if (*indexA == -1)
		*indexA = i1;
					
	for (x = i2; x < second->count; x++) {
		if (second->data[x].bottom >= top && *indexB == -1)
			*indexB = x;
		
		if (second->data[x].top <= top && second->data[x].bottom >= bottom) {
			lefts[foundCount] = second->data[x].left;
			rights[foundCount] = second->data[x].right;
			foundCount++;
		} else if (second->data[x].top > bottom)
			break;
	}
	
	if (*indexB == -1)
		*indexB = i2;
		
	if (foundCount > 1)
		SortTrans(lefts, rights, foundCount);
	
	ASSERT(foundCount > 0);
	
	clipping_rect rect;
	rect.top = top;
	rect.bottom = bottom;
	
	// Check if a rect intersects with the next one.
	// If so, merge the two rects, if not, just add the rect.
	long current = 0;
	while (current < foundCount) {
		long next = current + 1;
		
		rect.left = lefts[current];
		rect.right = rights[current];
		
		while (next < foundCount && rect.right >= lefts[next]) {
			if (rect.right < rights[next])
				rect.right = rights[next];
			next++;
		}
		
		dest->_AddRect(rect);		
		current = next;	
	}
	
	if (allocatedBuffer) {
		RTRACE(("Freeing heap...\n"));
		delete[] allocatedBuffer;
	}
}


/*! \brief Divides the plane into horizontal bands, then passes those bands to r_or
	which does the real work.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
*/
void
BRegion::Support::OrRegionComplex(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	long a = 0, b = 0;
	
	int32 top;
	int32 bottom = min_c(first->bound.top, second->bound.top) - 1;
	do {
		long x;
		top = bottom + 1;
		bottom = kMaxVerticalExtent;
		
		for (x = a; x < first->count; x++) {
			int32 n = first->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (first->data[x].bottom >= top && first->data[x].bottom < bottom)
				bottom = first->data[x].bottom;
		}	
		
		for (x = b; x < second->count; x++) {
			int32 n = second->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (second->data[x].bottom >= top && second->data[x].bottom < bottom)
				bottom = second->data[x].bottom;
		}
		
		// We can stand a region which extends to kMaxVerticalExtent, not more
		if (bottom >= kMaxVerticalExtent)
			break;
		
		ROr(top, bottom, first, second, dest, &a, &b);
		
	} while (true);
	
	CleanupRegion(dest);
}


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
	
	This function is called by or_region when one of the two regions contains just
	one rect.
*/
void
BRegion::Support::OrRegion1ToN(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);

	// The easy case first: if the first region contains the second,
	// the union is exactly the first region, since its bound is the 
	// only rectangle.
	if (first->bound.top <= second->bound.top 
			&& first->bound.bottom >= second->bound.bottom
			&& first->bound.left <= second->bound.left
			&& first->bound.right >= second->bound.right)
		CopyRegion(first, dest);
	else
		OrRegionComplex(first, second, dest);
}


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
	
	This function is called by or_region when the two regions don't intersect.
*/
void
BRegion::Support::OrRegionNoX(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);

	ZeroRegion(dest);
	
	long x;
	
	if (first->count == 0)
		for (x = 0; x < second->count; x++)
			dest->_AddRect(second->data[x]);
	
	else if (second->count == 0)
		for (x = 0; x < first->count; x++)
			dest->_AddRect(first->data[x]);
	
	else {
		long f = 0, s = 0;

		while (f < first->count && s < second->count) {
			
			if (first->data[f].top < second->data[s].top) {
				dest->_AddRect(first->data[f]);
				f++;
			
			} else {
				dest->_AddRect(second->data[s]);
				s++;
			}
		} 		
		
		if (f == first->count)
			for (; s < second->count; s++)
				dest->_AddRect(second->data[s]);

		else if (s == second->count)
			for (; f < first->count; f++)
				dest->_AddRect(first->data[f]);
	}
}


/*! \brief Divides the plane into horizontal bands, then passes those bands to r_sub
	which does the real work.
	\param first The subtraend region.
	\param second The minuend region.
	\param dest The destination region.
*/
void
BRegion::Support::SubRegionComplex(BRegion *first, BRegion *second, BRegion *dest)
{
	CALLED();
	long a = 0, b = 0;
	
	int32 top;
	int32 bottom = min_c(first->bound.top, second->bound.top) - 1;

	do {
		long x;
		top = bottom + 1;
		bottom = kMaxVerticalExtent;
		
		for (x = a; x < first->count; x++) {
			int32 n = first->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (first->data[x].bottom >= top && first->data[x].bottom < bottom)
				bottom = first->data[x].bottom;
		}	
		
		for (x = b; x < second->count; x++) {
			int32 n = second->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (second->data[x].bottom >= top && second->data[x].bottom < bottom)
				bottom = second->data[x].bottom;
		}
		
		if (bottom >= kMaxVerticalExtent)
			break;
			
		RSub(top, bottom, first, second, dest, &a, &b);
		
	} while (true);
	
	CleanupRegion(dest);
}


/*! \brief Converts the empty spaces between rectangles to rectangles,
	and the rectangles to empty spaces.

	Watch out!!! We write 1 element more than count, so be sure that the passed
	arrays have the needed space
*/
static void
InvertRectangles(long *lefts, long *rights, long count)
{
	long tmpLeft, tmpRight = kMaxNegative;
	
	for (int i = 0; i <= count; i++) {	
		tmpLeft = lefts[i] - 1;
		
		lefts[i] = (i == 0) ? kMaxNegative : tmpRight;
		tmpRight = rights[i] + 1;
		
		rights[i] = (i == count) ? kMaxPositive : tmpLeft;				
	}	
}


void
BRegion::Support::RSub(long top, long bottom, BRegion *first, BRegion *second, BRegion *dest, long *indexA, long *indexB)
{
	CALLED();
	
	// TODO: review heap management 
	int32 stackLeftsA[kMaxPoints / 2];
	int32 stackLeftsB[kMaxPoints / 2];
	int32 stackRightsA[kMaxPoints / 2];
	int32 stackRightsB[kMaxPoints / 2];

	int32 *leftsA = stackLeftsA;
	int32 *leftsB = stackLeftsB;
	int32 *rightsA = stackRightsA;
	int32 *rightsB = stackRightsB;
		
	long i1 = *indexA;
	long i2 = *indexB;
	
	*indexA = -1;
	*indexB = -1;
	
	long foundA = 0;
	long foundB = 0;
	long x = 0;
	
	// allocate arrays on the heap, if the ones one the stack are too small
	int32 *allocatedBuffer = NULL;
	int32 maxCountA = first->count - i1;
	int32 maxCountB = second->count - i2;
	
	if (maxCountA + maxCountB > kMaxPoints) {
		RTRACE(("Stack space isn't sufficient. Allocating %ld bytes on the heap...\n",
				2 * (maxCountA + maxCountB)));
		leftsA = allocatedBuffer = new(nothrow) int32[2 * (maxCountA + maxCountB)];
		if (!allocatedBuffer)
			return;
		rightsA = allocatedBuffer + maxCountA;
		leftsB = rightsA + maxCountA;
		rightsB = leftsB + maxCountB; 
	}

	// Store left and right points to the appropriate array
	for (x = i1; x < first->count; x++) {		
		// Look if this rect can be used next time we are called, 
		// thus correctly maintaining the "index" parameters.
		if (first->data[x].bottom >= top && *indexA == -1)
			*indexA = x;
			
		if (first->data[x].top <= top && first->data[x].bottom >= bottom) {
			leftsA[foundA] = first->data[x].left;
			rightsA[foundA] = first->data[x].right;
			foundA++;	
		} else if (first->data[x].top > bottom)
			break;	
	}
					
	for (x = i2; x < second->count; x++) {
		if (second->data[x].bottom >= top && *indexB == -1)
			*indexB = x;
		
		if (second->data[x].top <= top && second->data[x].bottom >= bottom) {
			leftsB[foundB] = second->data[x].left;
			rightsB[foundB] = second->data[x].right;
			foundB++;
		} else if (second->data[x].top > bottom)
			break;
	}
	
	if (*indexA == -1)
		*indexA = i1;
	if (*indexB == -1)
		*indexB = i2;
	
	if (foundA > 1)
		SortTrans(leftsA, rightsA, foundA);
	
	if (foundB > 1)
		SortTrans(leftsB, rightsB, foundB);
	
	// No minuend's rect, just add all the subtraend's rects.
	if (foundA == 0) {
		for (x = 0; x < foundB; x++) {
			clipping_rect rect = { leftsB[x], top, rightsB[x], bottom };
			dest->_AddRect(rect);
		}
	} else if (foundB > 0) {
		
		InvertRectangles(leftsA, rightsA, foundA);
		
		clipping_rect A, B;
		A.top = B.top = top;
		A.bottom = B.bottom = bottom;
		
		for (long f = 0; f <= foundA; f++) {
			for (long s = 0; s < foundB; s++) {
				A.left = leftsA[f];
				A.right = rightsA[f];
				
				B.left = leftsB[s];
				B.right = rightsB[s];
				if (rects_intersect(A, B))
					dest->_AddRect(sect_rect(A, B));
			}
		}
	}

	if (allocatedBuffer) {
		RTRACE(("Freeing heap...\n"));
		delete[] allocatedBuffer;
	}
}


#undef TRACE_REGION
