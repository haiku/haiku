#include <Debug.h>
#include <Region.h>

#include <clipping.h>

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
	
	int32 newCount = -1;
		
	if (region->count > 0) {
		for (int32 x = 0; x < region->count; x++) {
			clipping_rect rect = region->data[x];
						
			if ((rect.left == testRect.left)
					&& (rect.right == testRect.right)
					&& (rect.top == testRect.bottom + 1)) {
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
	} else if (count > 1) {
		do {
			again = false;
			for (int c = 1; c < count; c++) {
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

/*
void
sort_trans(long *lptr1, long *lptr2, long count)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));	
}
*/

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
	
	int32 newCount = -1;

	for (int x = 0; x < region->count; x++) {
		clipping_rect rect = region->data[x];
			if ((rect.top == testRect.top) && (rect.bottom == testRect.bottom)
								&& (rect.left == testRect.right + 1)) {
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


void
and_region_complex(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
		
	zero_region(dest);
			
	for (int32 f = 0; f < first->count; f++) {
		for (int32 s = 0; s < second->count; s++) {
			clipping_rect testRect = sect_rect(first->data[f],
				second->data[s]);
			if (valid_rect(testRect))
				dest->_AddRect(testRect);
		}
	}
	
	if (dest->count > 1)
		sort_rects(dest->data, dest->count);
}



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
	if ((first->bound.top <= second->bound.top) 
			&& (first->bound.bottom >= second->bound.bottom)
			&& (first->bound.left <= second->bound.left)
			&& (first->bound.right >= second->bound.right))
		copy_region(second, dest);
	else {
	// Otherwise, we check the rect of the first region against the rects
	// of the second, and we add their intersections to the destination region
		zero_region(dest);
		for (int32 x = 0; x < second->count; x++) {
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
	
	if ((first->count == 0) || (second->count == 0)
			|| (!valid_rect(intersection)))
		zero_region(dest);
	
	else if ((first->count == 1) && (second->count == 1)) {
		dest->data[0] = intersection;
		dest->bound = intersection;
		dest->count = 1;
	} 
	else if ((first->count > 1) && (second->count == 1))
		and_region_1_to_n(second, first, dest);
	
	else if ((first->count == 1) && (second->count > 1))
		and_region_1_to_n(first, second, dest);
	
	else
		and_region_complex(first, second, dest);		
}


void
append_region(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);
	
	copy_region(first, dest);

	for (int c = 0; c < second->count; c++)
		dest->_AddRect(second->data[c]);	
}


void
r_or(long top, long bottom, BRegion *first, BRegion *second, BRegion *dest, long *index1, long *index2)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	long i1 = *index1;
	long i2 = *index2;
	
	if (first->count <= i1) {
		if (second->count > i2) {
			clipping_rect secondRect = second->data[i2];
		
			secondRect.top = max_c(secondRect.top, top);
			secondRect.bottom = min_c(secondRect.bottom, bottom);
			
			if (valid_rect(secondRect))
				dest->_AddRect(secondRect);
			if (second->data[i2].bottom <= bottom)
				i2++;
	 	}
	 	
	 } else if (second->count <= i2) {
	 
	 	clipping_rect firstRect = first->data[i1];
	 	
	 	firstRect.top = max_c(firstRect.top, top);
	 	firstRect.bottom = min_c(firstRect.bottom, bottom);
	 	
	 	if (valid_rect(firstRect));
	 		dest->_AddRect(firstRect);
	 	if (first->data[i1].bottom <= bottom)
	 		i1++;
	 		
	 } else {
	 	clipping_rect firstRect = first->data[i1];
	 	clipping_rect secondRect = second->data[i2];
	 	
	 	firstRect.top = max_c(firstRect.top, top);
	 	firstRect.bottom = min_c(firstRect.bottom, bottom);
	 	
	 	secondRect.top = max_c(secondRect.top, top);
		secondRect.bottom = min_c(secondRect.bottom, bottom);
	 	
	 	if (valid_rect(sect_rect(firstRect, secondRect)))
	 		dest->_AddRect(union_rect(firstRect, secondRect));
	 	
	 	else {
	 		if (valid_rect(firstRect))
	 			dest->_AddRect(firstRect);
	 		if (valid_rect(secondRect))
	 			dest->_AddRect(secondRect);
	 	}
	 		
	 	if (first->data[i1].bottom <= bottom)
	 		i1++;
	 	if (second->data[i2].bottom <= bottom)
			i2++;
	}
		
	*index1 = i1;
	*index2 = i2;    
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
		top = bottom + 1;
		bottom = 0x10000000;
		
		for (int x = a; x < first->count; x++) {
			int32 n = first->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (first->data[x].bottom >= top && first->data[x].bottom < bottom)
				bottom = first->data[x].bottom;
		}	
		
		for (int x = b; x < second->count; x++) {
			int32 n = second->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (second->data[x].bottom >= top && second->data[x].bottom < bottom)
				bottom = second->data[x].bottom;
		}
		
		// We can stand a region which extends to 0x10000000, not more
		if (bottom >= 0x10000000)
			break;
			
		r_or(top, bottom, first, second, dest, &a, &b);
		
	} while (true);
	
	cleanup_region(dest);
}


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
	if ((first->bound.top <= second->bound.top) 
			&& (first->bound.bottom >= second->bound.bottom)
			&& (first->bound.left <= second->bound.left)
			&& (first->bound.right >= second->bound.right))
		copy_region(first, dest);
	else
		or_region_complex(first, second, dest);
}


void
or_region_no_x(BRegion *first, BRegion *second, BRegion *dest)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(first);
	ASSERT(second);
	ASSERT(dest);

	zero_region(dest);
	
	if (first->count == 0)
		for (int x = 0; x < second->count; x++)
			dest->_AddRect(second->data[x]);
	
	else if (second->count == 0)
		for (int x = 0; x < first->count; x++)
			dest->_AddRect(first->data[x]);
	
	else {
		long f = 0, s = 0;

		while ((f < first->count) && (s < second->count)) {
			
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
		top = bottom + 1;
		bottom = 0x10000000;
		
		for (int x = a; x < first->count; x++) {
			int32 n = first->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (first->data[x].bottom >= top && first->data[x].bottom < bottom)
				bottom = first->data[x].bottom;
		}	
		
		for (int x = b; x < second->count; x++) {
			int32 n = second->data[x].top - 1;
			if (n >= top && n < bottom)
				bottom = n;
			if (second->data[x].bottom >= top && second->data[x].bottom < bottom)
				bottom = second->data[x].bottom;
		}
		
		if (bottom >= 0x10000000)
			break;
			
		r_sub(top, bottom, first, second, dest, &a, &b);
		
	} while (true);
	
	cleanup_region(dest);
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
		
	else if ((second->count == 0) 
			|| (!valid_rect(sect_rect(first->bound, second->bound))))
		copy_region(first, dest);
	else
		sub_region_complex(second, first, dest);	
}


/* BRegion class */

/*! \brief Initializes a region. The region will have no rects, and its bound will be invalid.
*/
BRegion::BRegion()
	:data(NULL)
{
	data_size = 8;
	data = (clipping_rect*)malloc(data_size * sizeof(clipping_rect));
	
	zero_region(this);
}


/*! 	\brief Initializes a region to be a copy of another.
	\param region The region to copy.
*/
BRegion::BRegion(const BRegion &region)
	:data(NULL)
{
	if (&region != this) {
		bound = region.bound;
		count = region.count;
		data_size = region.data_size;
		
		if (data_size <= 0)
			data_size = 1;
			
		data = (clipping_rect*)malloc(data_size * sizeof(clipping_rect));
		
		memcpy(data, region.data, count * sizeof(clipping_rect));
	}
}


/*!	\brief Initializes a region to contain a BRect.
	\param rect The BRect to set the region to.
*/
BRegion::BRegion(const BRect rect)
	:data(NULL)
{
	data_size = 8;
	data = (clipping_rect*)malloc(data_size * sizeof(clipping_rect));
		
	Set(rect);	
}


/*!	\brief Frees the allocated memory.
*/
BRegion::~BRegion()
{
	free(data);
}


/*! \brief Returns the bounds of the region.
	\return A BRect which represents the bounds of the region.
*/
BRect
BRegion::Frame() const
{
	return to_BRect(bound);
}


/*! \brief Returns the bounds of the region as a clipping_rect (which has integer coordinates).
	\return A clipping_rect which represents the bounds of the region.
*/
clipping_rect
BRegion::FrameInt() const
{
	return bound;
}


/*! \brief Returns the regions's BRect at the given index.
	\param index The index (zero based) of the wanted rectangle.
	\return If the given index is valid, it returns the BRect at that index,
		otherwise, it returns an invalid BRect.
*/
BRect
BRegion::RectAt(int32 index)
{
	if (index >= 0 && index < count)
		return to_BRect(data[count]);
	
	return BRect(); //An invalid BRect
}


/*! \brief Returns the regions's clipping_rect at the given index.
	\param index The index (zero based) of the wanted rectangle.
	\return If the given index is valid, it returns the clipping_rect at that index,
		otherwise, it returns an invalid clipping_rect.
*/
clipping_rect
BRegion::RectAtInt(int32 index)
{
	clipping_rect rect = { 1, 1, 0, 0 };
	
	if (index >= 0 && index < count)
		return data[index];
	
	return rect;
}


/*!	\brief Counts the region rects.
	\return An int32 which is the total number of rects in the region.
*/
int32
BRegion::CountRects()
{
	return count;
}


/*!	\brief Set the region to contain just the given BRect.
	\param newBounds A BRect.
*/
void
BRegion::Set(BRect newBounds)
{
	Set(to_clipping_rect(newBounds));
}


/*!	\brief Set the region to contain just the given clipping_rect.
	\param newBounds A clipping_rect.
*/
void
BRegion::Set(clipping_rect newBounds)
{
	ASSERT(data_size > 0);

	if (valid_rect(newBounds)) {
		count = 1;
		data[0] = newBounds;
		bound = newBounds;
	}
	else
		zero_region(this);	
}


/*!	\brief Check if the region has any area in common with the given BRect.
	\param rect The BRect to check the region against to.
	\return \ctrue if the region has any area in common with the BRect, \cfalse if not.
*/
bool
BRegion::Intersects(BRect rect) const
{
	return Intersects(to_clipping_rect(rect));
}


/*!	\brief Check if the region has any area in common with the given clipping_rect.
	\param rect The clipping_rect to check the region against to.
	\return \ctrue if the region has any area in common with the clipping_rect, \cfalse if not.
*/
bool
BRegion::Intersects(clipping_rect rect) const
{
	if (!valid_rect(sect_rect(rect, bound)))
		return false;

	for (int c = 0; c < count; c++) {
		if (valid_rect(sect_rect(data[c], rect)))
			return true;
	}
	
	return false;	
}


/*!	\brief Check if the region contains the given BPoint.
	\param pt The BPoint to be checked.
	\return \ctrue if the region contains the BPoint, \cfalse if not.
*/
bool
BRegion::Contains(BPoint pt) const
{
	// If the point doesn't lie within the region's bounds,
	// don't ever try it against the region's rects.
	if (!point_in(bound, pt))
		return false;

	for (int c = 0; c < count; c++) {
		if (point_in(data[c], pt))
			return true;
	}
	return false;
}


/*!	\brief Check if the region contains the given coordinates.
	\param x The \cx coordinate of the point to be checked.
	\param y The \cy coordinate of the point to be checked.
	\return \ctrue if the region contains the point, \cfalse if not.
*/
bool
BRegion::Contains(int32 x, int32 y)
{
	// see above
	if (!point_in(bound, x, y))
		return false;

	for (int c = 0; c < count; c++) {
		if (point_in(data[c], x, y))
			return true;
	}
	return false;
}


/*!	\brief Prints the BRegion to stdout.
*/
void
BRegion::PrintToStream() const
{
	Frame().PrintToStream();
	
	for (int c = 0; c < count; c++) {
		clipping_rect *rect = &data[c];
		printf("data = BRect(l:%ld.0, t:%ld.0, r:%ld.0, b:%ld.0)\n",
			rect->left, rect->top, rect->right, rect->bottom);
	}	
}


/*!	\brief Offses all region's rects, and bounds by the given values.
	\param dh The horizontal offset.
	\param dv The vertical offset.
*/
void
BRegion::OffsetBy(int32 dh, int32 dv)
{
	if (count > 0) {
		for (int c = 0; c < count; c++) {
			data[c].left += dh;
			data[c].right += dh;
			data[c].top += dv;
			data[c].bottom += dv;
		}
		bound.left += dh;
		bound.right += dh;
		bound.top += dv;
		bound.bottom += dv;		
	}
}


/*!	\brief Empties the region, so that it doesn't include any rect, and invalidates its bounds.
*/
void
BRegion::MakeEmpty()
{
	zero_region(this);
}


/*!	\brief Modifies the region, so that it includes the given BRect.
	\param rect The BRect to be included by the region.
*/
void
BRegion::Include(BRect rect)
{
	Include(to_clipping_rect(rect));
}


/*!	\brief Modifies the region, so that it includes the given clipping_rect.
	\param rect The clipping_rect to be included by the region.
*/
void
BRegion::Include(clipping_rect rect)
{
	BRegion region;
	BRegion newRegion;	

	region.Set(rect);
	
	or_region(this, &region, &newRegion);
	copy_region(&newRegion, this);
}


/*!	\brief Modifies the region, so that it includes the area of the given region.
	\param region The region to be included.
*/
void
BRegion::Include(const BRegion *region)
{
	BRegion newRegion;
	
	or_region(this, const_cast<BRegion*>(region), &newRegion);
	copy_region(&newRegion, this);
}


/*!	\brief Modifies the region, excluding the area represented by the given BRect.
	\param rect The BRect to be excluded.
*/
void
BRegion::Exclude(BRect rect)
{
	Exclude(to_clipping_rect(rect));
}


/*!	\brief Modifies the region, excluding the area represented by the given clipping_rect.
	\param rect The clipping_rect to be excluded.
*/
void
BRegion::Exclude(clipping_rect rect)
{
	BRegion region;
	BRegion newRegion;
	
	region.Set(rect);
	sub_region(this, &region, &newRegion);
	copy_region(&newRegion, this);
}


/*!	\brief Modifies the region, excluding the area contained in the given BRegion.
	\param region The BRegion to be excluded.
*/
void
BRegion::Exclude(const BRegion *region)
{
	BRegion newRegion;
	
	sub_region(this, const_cast<BRegion*>(region), &newRegion);
	copy_region(&newRegion, this);
}


/*!	\brief Modifies the region, so that it will contain just the area in common with the given BRegion.
	\param region the BRegion to intersect to.
*/
void
BRegion::IntersectWith(const BRegion *region)
{
	BRegion newRegion;
	
	and_region(this, const_cast<BRegion*>(region), &newRegion);
	copy_region(&newRegion, this);
}


/*!	\brief Modifies the region to be a copy of the given BRegion.
	\param region the BRegion to copy.
	\return This function always returns \c *this.
*/
BRegion &
BRegion::operator=(const BRegion &region)
{
	if (&region != this) {
		free(data);
		bound = region.bound;
		count = region.count;
		data_size = region.data_size;
		
		if (data_size <= 0)
			data_size = 1;
			
		data = (clipping_rect*)malloc(data_size * sizeof(clipping_rect));
		
		memcpy(data, region.data, count * sizeof(clipping_rect));
	}
	
	return *this;
}


/*!	\brief Adds a rect to the region.
	\param rect The clipping_rect to be added.
	
	Adds the given rect to the region, merging it with another already contained in the region,
	if possible. Recalculate the region's bounds if needed.
*/
void
BRegion::_AddRect(clipping_rect rect)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	ASSERT(count >= 0);
	ASSERT(data_size >= 0);
	
	// Should we just reallocate the memory and
	// copy the rect ?
	bool add = false; 
		
	if (count <= 0)
		// Yeah, we don't have any rect here..	
	 	add = true;
	else {
		// Wait! We could merge "rect" with one of the
		// existing rectangles...
		int32 last = count - 1;
		if ((rect.top == data[last].bottom + 1) 
				&& (rect.left == data[last].left)
				&& (rect.right == data[last].right)) {
		 
			data[last].bottom = rect.bottom;
		
		} else if ((rect.top == data[last].top) && (rect.bottom = data[last].bottom)) {
			
			if (rect.left == data[last].right + 1)
				data[last].right = rect.right;
			else if (rect.right == data[last].left - 1)
				data[last].left = rect.left;
			else 
				add = true; //no luck... just add the rect
		
		} else
			add = true; //just add the rect
	}		
	
	// We weren't lucky.... just add the rect as a new one
	if (add) {
		if (data_size <= count)
			set_size(data_size + 16);
			
		data[count] = rect;
		
		count++;
	}
	
	// Recalculate bounds
	if (rect.top <= bound.top)
		bound.top = rect.top;

	if (rect.left <= bound.left)
		bound.left = rect.left;
 
	if (rect.right >= bound.right)
		bound.right = rect.right;
 
	if (rect.bottom >= bound.bottom)
		bound.bottom = rect.bottom;
}


/*!	\brief Reallocate the memory in the region.
	\param new_size The amount of rectangles that the region could contain.
*/
void
BRegion::set_size(long new_size)
{
	if (new_size <= 0)
		new_size = data_size + 16;
	
	data = (clipping_rect*)realloc(data, new_size * sizeof(clipping_rect));
	
	if (data == NULL)
		debugger("BRegion::set_size realloc error\n");
	
	data_size = new_size;
	
	ASSERT(count <= data_size);		
}


long
BRegion::find_small_bottom(long y1, long y2, long *hint, long *where)
{
	//XXX: What does this do?
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	return 0;
}
