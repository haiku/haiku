#include <Debug.h>
#include <Region.h>

#include <clipping.h>

/* friend functions */
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

/*
void
cleanup_region_1(BRegion *region)
{

}
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


void
sort_rects(clipping_rect *rects, long count)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));
	bool again; //flag that tells we changed rects positions
			
	if (count == 2) {
		if (rects[0].left > rects[1].left) {
			clipping_rect tmp = rects[0];
			rects[0] = rects[1];
			rects[1] = tmp;
		}
	} else if (count > 1) {
		do {
			again = false;
			for (int c = 1; c < count; c++) {
				if (rects[c - 1].left > rects[c].left) {
					clipping_rect tmp = rects[c - 1];
					rects[c - 1] = rects[c];
					rects[c] = tmp;
					again = true;
				}
			}
		} while (again != true); 
	}
}

/*
void
sort_trans(long *lptr1, long *lptr2, long count)
{
	PRINT(("%s\n", __PRETTY_FUNCTION__));	
}
*/
/*
void
cleanup_region_horizontal(BRegion *region)
{

}
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

/*
void
r_or(long, long, BRegion *first, BRegion *second, BRegion *dest, long *L1, long *L2)
{	      
}
*/

/*
void
or_region_complex(BRegion *first, BRegion *second, BRegion *dest)
{
}
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

/*
void
sub_region_complex(BRegion *first, BRegion *second, BRegion *dest)
{
}
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
BRegion::BRegion()
	:data(NULL)
{
	data_size = 8;
	data = (clipping_rect*)malloc(data_size * sizeof(clipping_rect));
	
	zero_region(this);
}


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


BRegion::BRegion(const BRect rect)
	:data(NULL)
{
	data_size = 8;
	data = (clipping_rect*)malloc(data_size * sizeof(clipping_rect));
		
	Set(rect);	
}



BRegion::~BRegion()
{
	free(data);
}


BRect
BRegion::Frame() const
{
	return to_BRect(bound);
}


clipping_rect
BRegion::FrameInt() const
{
	return bound;
}


BRect
BRegion::RectAt(int32 index)
{
	if (index >= 0 && index < count)
		return to_BRect(data[count]);
	
	return BRect(); //An invalid BRect
}


clipping_rect
BRegion::RectAtInt(int32 index)
{
	clipping_rect rect = { 1, 1, 0, 0 };
	
	if (index >= 0 && index < count)
		return data[index];
	
	return rect;
}


int32
BRegion::CountRects()
{
	return count;
}


void
BRegion::Set(BRect newBounds)
{
	Set(to_clipping_rect(newBounds));
}


void
BRegion::Set(clipping_rect newBounds)
{
	ASSERT(data_size > 0);

	if (valid_rect(newBounds)) {
		count = 1;
		data[0] = newBounds;
	}
	else
		count = 0;
	
	// Set bounds anyway.
	bound = newBounds;	
}


bool
BRegion::Intersects(BRect rect) const
{
	return Intersects(to_clipping_rect(rect));
}


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


void
BRegion::PrintToStream() const
{
	Frame().PrintToStream();
	
	for (int c = 0; c < count; c++) {
		clipping_rect *rect = &data[c];
		printf("data = BRect(l:%ld, t:%ld.0, r:%ld.0, b:%ld.0)\n",
			rect->left, rect->top, rect->right, rect->bottom);
	}	
}


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


void
BRegion::MakeEmpty()
{
	zero_region(this);
}


void
BRegion::Include(BRect rect)
{
	Include(to_clipping_rect(rect));
}


void
BRegion::Include(clipping_rect rect)
{
	BRegion region;
	BRegion newRegion;	

	region.Set(rect);
	
	or_region(this, &region, &newRegion);
	copy_region(&newRegion, this);
}


void
BRegion::Include(const BRegion *region)
{
	BRegion newRegion;
	
	or_region(this, const_cast<BRegion*>(region), &newRegion);
	copy_region(&newRegion, this);
}


void
BRegion::Exclude(BRect rect)
{
	Exclude(to_clipping_rect(rect));
}


void
BRegion::Exclude(clipping_rect rect)
{
	BRegion region;
	BRegion newRegion;
	
	region.Set(rect);
	sub_region(this, &region, &newRegion);
	copy_region(&newRegion, this);
}


void
BRegion::Exclude(const BRegion *region)
{
	BRegion newRegion;
	
	sub_region(this, const_cast<BRegion*>(region), &newRegion);
	copy_region(&newRegion, this);
}


void
BRegion::IntersectWith(const BRegion *region)
{
	BRegion newRegion;
	
	and_region(this, const_cast<BRegion*>(region), &newRegion);
	copy_region(&newRegion, this);
}


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
			set_size(16);
			
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
