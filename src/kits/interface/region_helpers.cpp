#include <Debug.h>
#include <Region.h>

#include <clipping.h>
#include "region_helpers.h"

static const int32 kMaxPoints = 1024;
static const int32 kMaxVerticalExtent = 0x10000000;

/* friend functions */

/*!	\brief zeroes the given region, setting its rect count to 0,
	and invalidating its bound rectangle.
	\param region The region to be zeroed.
*/
void
zero_region(BRegion *region)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	
	region->count = 0;
	region->bound.left = 0x7ffffffd;
	region->bound.top = 0x7ffffffd;
	region->bound.right = 0x80000003;
	region->bound.bottom = 0x80000003;
}


void
clear_region(BRegion *region)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	region->count = 0;
	region->bound.left = 0xfffffff;
	region->bound.top = 0xfffffff;
	region->bound.right = 0xf0000001;
	region->bound.bottom = 0xf0000001;
}


/*!	\brief Cleanup the region vertically.
	\param region The region to be cleaned.
*/
void
cleanup_region_1(BRegion *region)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));

	clipping_rect testRect =
		{
			1, 1,
			-1, -2
		};
	
	long newCount = -1;
		
	if (region->count > 0) {
		for (long x = 0; x < region->count; x++) {
			clipping_rect rect = region->data[x];
						
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
	
		cleanup_region_horizontal(region);	
	}

}


/*!	\brief Cleanup the region, by merging rects that can be merged.
	\param region The region to be cleaned.
*/
void
cleanup_region(BRegion *region)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	long oldCount;
	
	do {
		oldCount = region->count;
		cleanup_region_1(region);
	} while (region->count < oldCount);
}


/*!	\brief Sorts the given rects by their top value.
	\param rects A pointer to an array of clipping_rects.
	\param count The number of rectangles in the array.
*/
void
sort_rects(clipping_rect *rects, long count)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	bool again; //flag that tells we changed rects positions
			
	if (count == 2) {
		if (rects[0].top > rects[1].top) {
			clipping_rect tmp = rects[0];
			rects[0] = rects[1];
			rects[1] = tmp;
		}
	} else if (count > 2) {
		do {
			again = false;
			for (long c = 1; c < count; c++) {
				if (rects[c - 1].top > rects[c].top) {
					clipping_rect tmp = rects[c - 1];
					rects[c - 1] = rects[c];
					rects[c] = tmp;
					again = true;
				}
			}
		} while (again); 
	}
}


void
sort_trans(long *lptr1, long *lptr2, long count)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	bool again; //flag that tells we changed trans positions
			
	if (count == 2) {
		if (lptr1[0] > lptr1[1]) {
			int32 tmp = lptr1[0];
			lptr1[0] = lptr1[1];
			lptr1[1] = tmp;
			
			tmp = lptr2[0];
			lptr2[0] = lptr2[1];
			lptr2[1] = tmp;
		}
	} else if (count > 2) {
		do {
			again = false;
			for (long c = 1; c < count; c++) {
				if (lptr1[c - 1] > lptr1[c]) {
					int32 tmp = lptr1[c - 1];
					lptr1[c - 1] = lptr1[c];
					lptr1[c] = tmp;
					
					tmp = lptr2[c - 1];
					lptr2[c - 1] = lptr2[c];
					lptr2[c] = tmp;
					again = true;
				}
			}
		} while (again); 
	}
}


/*!	\brief Cleanup the region horizontally.
	\param region The region to be cleaned.
*/
void
cleanup_region_horizontal(BRegion *region)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));

	clipping_rect testRect =
		{
			1, 1,
			-2, -1
		};
	
	long newCount = -1;

	for (long x = 0; x < region->count; x++) {
		clipping_rect rect = region->data[x];
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


/*!	\brief Copy a region to another.
	\param source The region to be copied.
	\param dest The destination region.
*/
void
copy_region(BRegion *source, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(source);
	ASSERT(dest);
	ASSERT(source != dest);
		
	// If there is not enough memory, allocate
	if (dest->data_size < source->count) {
		free(dest->data);
		dest->data_size = source->count + 8;
		dest->data = (clipping_rect*)malloc(dest->data_size * sizeof(clipping_rect));
	}
	
	dest->count = source->count;
	
	// Copy rectangles and bounds.
	memcpy(dest->data, source->data, source->count * sizeof(clipping_rect));
	dest->bound = source->bound;
}


/*!	\brief Copy a region to another, allocating some additional memory in the destination region.
	\param source The region to be copied.
	\param dest The destination region.
	\param count Amount of additional memory to be allocated in the destination region.
*/
void
copy_region_n(BRegion *source, BRegion *dest, long count)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(source);
	ASSERT(dest);
	ASSERT(source != dest);
	
	// If there is not enough memory, allocate
	if (dest->data_size < source->count) {
		free(dest->data);
		dest->data_size = source->count + count;
		dest->data = (clipping_rect*)malloc(dest->data_size * sizeof(clipping_rect));
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
and_region_complex(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
		
	zero_region(dest);
			
	for (long f = 0; f < first->count; f++) {
		for (long s = 0; s < second->count; s++) {
			clipping_rect testRect = sect_rect(first->data[f], second->data[s]);
			if (valid_rect(testRect))
				dest->_AddRect(testRect);
		}
	}
	
	if (dest->count > 1)
		sort_rects(dest->data, dest->count);
}


/*!	\brief Modify the destination region to be the intersection of the two given regions.
	\param first The first region to be intersected.
	\param second The second region to be intersected.
	\param dest The destination region.
	
	Called by and_region() when one of the two region contains just one rect.
*/	
void
and_region_1_to_n(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
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
		copy_region(second, dest);
	else {
	// Otherwise, we check the rect of the first region against the rects
	// of the second, and we add their intersections to the destination region
		zero_region(dest);
		for (long x = 0; x < second->count; x++) {
			clipping_rect testRect = sect_rect(first->data[0], second->data[x]);
			if (valid_rect(testRect))
				dest->_AddRect(testRect);
		}
	}
}


/*!	\brief Modify the destination region to be the intersection of the two given regions.
	\param first The first region to be intersected.
	\param second The second region to be intersected.
	\param dest The destination region.
	
	This function is a sort of method selector. It checks for some special
	cases, then it calls the appropriate specialized function.
*/
void
and_region(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	clipping_rect intersection = sect_rect(first->bound, second->bound);
	
	if (first->count == 0 || second->count == 0	|| !valid_rect(intersection))
		zero_region(dest);
	
	else if (first->count == 1 && second->count == 1) {
		dest->data[0] = intersection;
		dest->bound = intersection;
		dest->count = 1;
	} 
	else if (first->count > 1 && second->count == 1)
		and_region_1_to_n(second, first, dest);
	
	else if (first->count == 1 && second->count > 1)
		and_region_1_to_n(first, second, dest);
	
	else
		and_region_complex(first, second, dest);		
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
append_region(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	copy_region(first, dest);

	for (long c = 0; c < second->count; c++)
		dest->_AddRect(second->data[c]);	
}


void
r_or(long top, long bottom, BRegion *first, BRegion *second, BRegion *dest, long *indexA, long *indexB)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	
	int32 lefts[kMaxPoints];
	int32 rights[kMaxPoints];
	
	long i1 = *indexA;
	long i2 = *indexB;
	
	*indexA = -1;
	*indexB = -1;
	
	long foundCount = 0;
	long x = 0;
	
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
		sort_trans(lefts, rights, foundCount);
	
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
}


/*! \brief Divides the plane into horizontal bands, then passes those bands to r_or
	which does the real work.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
*/
void
or_region_complex(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
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
		
		r_or(top, bottom, first, second, dest, &a, &b);
		
	} while (true);
	
	cleanup_region(dest);
}


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
	
	This function is called by or_region when one of the two regions contains just
	one rect.
*/
void
or_region_1_to_n(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
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
		copy_region(first, dest);
	else
		or_region_complex(first, second, dest);
}


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be or-ed.
	\param second The second region to be or-ed.
	\param dest The destination region.
	
	This function is called by or_region when the two regions don't intersect.
*/
void
or_region_no_x(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);

	zero_region(dest);
	
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


/*!	\brief Modify the destination region to be the union of the two given regions.
	\param first The first region to be merged.
	\param second The second region to be merged.
	\param dest The destination region.
	
	This function is a sort of method selector. It checks for some special
	cases, then it calls the appropriate specialized function.
*/
void
or_region(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
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
		copy_region(regionA, dest);
	else {
		if (regionB->bound.top > regionA->bound.bottom)
			append_region(regionA, regionB, dest);
		
		else if (regionA->bound.top > regionB->bound.bottom)
			append_region(regionB, regionA, dest);
		
		else if (regionA->bound.left > regionB->bound.right)
			or_region_no_x(regionB, regionA, dest);
		
		else if (regionB->bound.left > regionA->bound.right)
			or_region_no_x(regionA, regionB, dest);
		
		else if (regionA->count == 1)
			or_region_1_to_n(regionA, regionB, dest);
		
		else if (regionB->count == 1)
			or_region_1_to_n(regionB, regionA, dest);
		
		else 
			or_region_complex(regionA, regionB, dest);		
	}
}


/*! \brief Divides the plane into horizontal bands, then passes those bands to r_sub
	which does the real work.
	\param first The subtraend region.
	\param second The minuend region.
	\param dest The destination region.
*/
void
sub_region_complex(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
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
			
		r_sub(top, bottom, first, second, dest, &a, &b);
		
	} while (true);
	
	cleanup_region(dest);
}


void
r_sub(long top, long bottom, BRegion *first, BRegion *second, BRegion *dest, long *indexA, long *indexB)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	
	int32 leftsA[kMaxPoints / 2];
	int32 rightsA[kMaxPoints / 2];
	int32 leftsB[kMaxPoints / 2];
	int32 rightsB[kMaxPoints / 2];
	
	long i1 = *indexA;
	long i2 = *indexB;
	
	*indexA = -1;
	*indexB = -1;
	
	long foundA = 0;
	long foundB = 0;
	long x = 0;
	
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
	
	if (*indexA == -1)
		*indexA = i1;
	
	if (foundA > 1)
		sort_trans(leftsA, rightsA, foundA);
					
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
	
	if (*indexB == -1)
		*indexB = i2;
		
	if (foundB > 1)
		sort_trans(leftsB, rightsB, foundB);

	// No minuend's rect, just add all the subtraend's rects.
	if (foundA == 0)
		for (x = 0; x < foundB; x++) {
			clipping_rect rect = { leftsB[x], top, rightsB[x], bottom };
			dest->_AddRect(rect);
		}
	else if (foundB > 0) {
		long f = 0, s = 0;
		
		clipping_rect minuendRect;
		minuendRect.top = top;
		minuendRect.bottom = bottom;
		minuendRect.left = 0x80000003;
		
		clipping_rect subRect;
		subRect.top = top;
		subRect.bottom = bottom;
			
		// We take the empty spaces between the minuend rects, and intersect
		// these with the subtraend rects. We then add their intersection
		// to the destination region.
		do {			
			subRect.left = leftsB[s];
			subRect.right = rightsB[s];	
			
			if (f != 0)
				minuendRect.left = rightsA[f - 1] + 1;
			
			minuendRect.right = leftsA[f] - 1;
				
			if (leftsB[s] > minuendRect.right) {
				if (++f > foundA)
					break;
				else
					continue;			
			}
			
			clipping_rect intersection = sect_rect(minuendRect, subRect);
			
			if (valid_rect(intersection))
				dest->_AddRect(intersection);
		
			if (rightsB[s] < minuendRect.left)
				s++;
					
			if (s >= foundB)
				break;
			
			f++;
							
		} while (f < foundA);
		
		//Last rect: we take the right coordinate of the last minuend rect
		//as left coordinate of the rect to intersect, and the maximum possible
		//value as the right coordinate.  
		minuendRect.left = rightsA[foundA - 1] + 1;
		minuendRect.right = 0x7ffffffd;
		
		// Skip the subtraend rects that could never intersect.
		while (s < foundB && rightsB[s] < minuendRect.left)
			s++;
		
		for (long c = s; c < foundB; c++) {
			subRect.left = leftsB[c];
			subRect.right = rightsB[c];
			
			clipping_rect intersection = sect_rect(minuendRect, subRect);
			
			if (valid_rect(intersection))
				dest->_AddRect(intersection);
		}	
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
sub_region(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);

	if (first->count == 0)
		zero_region(dest);
		
	else if (second->count == 0	|| !valid_rect(sect_rect(first->bound, second->bound)))
		copy_region(first, dest);
		
	else
		sub_region_complex(second, first, dest);	
}

