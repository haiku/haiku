//------------------------------------------------------------------------------
//	Copyright (c) 2003, OpenBeOS
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
//	File Name:		Region.cpp
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	Region class consisting of multiple rectangles
//
//------------------------------------------------------------------------------

//	Notes: As now, memory is always allocated and never freed (except on destruction,
//	or sometimes when a copy is made).
//  This let us be a bit faster since we don't do many reallocations.
//	But that means that even an empty region could "waste" much space, if it contained
//	many rects before being emptied.
//	I.E: a region which contains 24 rects allocates more than 24 * 4 * sizeof(int32)
//  = 96 * sizeof(int32) bytes. If we call MakeEmpty(), that region will contain no rects,
//  but it will still keep the allocated memory.
//	This shouldnt' be an issue, since usually BRegions are just used for calculations, 
//	and don't last so long.
//	Anyway, we can change that behaviour if we want, but BeOS's BRegion seems to behave exactly
//	like this.


// Standard Includes -----------------------------------------------------------
#include <cstdlib>
#include <cstring>

// System Includes -------------------------------------------------------------
#include <Debug.h>
#include <Region.h>

// Private Includes -------------------------------------------------------------
#include <clipping.h>
#include <RegionSupport.h>


/*! \brief Initializes a region. The region will have no rects,
	and its bound will be invalid.
*/
BRegion::BRegion()
	:
	data_size(8),
	data(NULL)	
{
	data = (clipping_rect *)malloc(data_size * sizeof(clipping_rect));
	
	Support::ZeroRegion(this);
}


/*! \brief Initializes a region to be a copy of another.
	\param region The region to copy.
*/
BRegion::BRegion(const BRegion &region)
	:
	data(NULL)
{
	bound = region.bound;
	count = region.count;
	data_size = region.data_size;
	
	if (data_size <= 0)
		data_size = 1;
		
	data = (clipping_rect *)malloc(data_size * sizeof(clipping_rect));
	
	memcpy(data, region.data, count * sizeof(clipping_rect));
}


/*!	\brief Initializes a region to contain a BRect.
	\param rect The BRect to set the region to.
*/
BRegion::BRegion(const BRect rect)
	:
	data_size(8),
	data(NULL)	
{
	data = (clipping_rect *)malloc(data_size * sizeof(clipping_rect));
		
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
		return to_BRect(data[index]);
	
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
		Support::ZeroRegion(this);	
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
	if (!rects_intersect(rect, bound))
		return false;

	for (long c = 0; c < count; c++) {
		if (rects_intersect(data[c], rect))
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
	// don't even try it against the region's rects.
	if (!point_in(bound, pt))
		return false;

	for (long c = 0; c < count; c++) {
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

	for (long c = 0; c < count; c++) {
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
	
	for (long c = 0; c < count; c++) {
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
		for (long c = 0; c < count; c++)
			offset_rect(data[c], dh, dv);

		offset_rect(bound, dh, dv);	
	}
}


/*!	\brief Empties the region, so that it doesn't include any rect, and invalidates its bounds.
*/
void
BRegion::MakeEmpty()
{
	Support::ZeroRegion(this);
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
	
	Support::OrRegion(this, &region, &newRegion);
	Support::CopyRegion(&newRegion, this);
}


/*!	\brief Modifies the region, so that it includes the area of the given region.
	\param region The region to be included.
*/
void
BRegion::Include(const BRegion *region)
{
	BRegion newRegion;
	
	Support::OrRegion(this, const_cast<BRegion *>(region), &newRegion);
	Support::CopyRegion(&newRegion, this);
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

	Support::SubRegion(this, &region, &newRegion);
	Support::CopyRegion(&newRegion, this);
}


/*!	\brief Modifies the region, excluding the area contained in the given BRegion.
	\param region The BRegion to be excluded.
*/
void
BRegion::Exclude(const BRegion *region)
{
	BRegion newRegion;
	
	Support::SubRegion(this, const_cast<BRegion *>(region), &newRegion);
	Support::CopyRegion(&newRegion, this);
}


/*!	\brief Modifies the region, so that it will contain just the area in common with the given BRegion.
	\param region the BRegion to intersect to.
*/
void
BRegion::IntersectWith(const BRegion *region)
{
	BRegion newRegion;
	
	Support::AndRegion(this, const_cast<BRegion *>(region), &newRegion);
	Support::CopyRegion(&newRegion, this);
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
			
		data = (clipping_rect *)malloc(data_size * sizeof(clipping_rect));
		
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
	ASSERT(count >= 0);
	ASSERT(data_size >= 0);
	
	// Should we just reallocate the memory and
	// copy the rect ?
	bool addRect = true; 
		
	if (count > 0) {
		// Wait! We could merge the rect with one of the
		// existing rectangles, if it's adiacent.
		// We just check it against the last rectangle, since
		// we are keeping them sorted by their "top" coordinates.
		long last = count - 1;
		if (rect.left == data[last].left && rect.right == data[last].right
				&& rect.top == data[last].bottom + 1) {
		 
			data[last].bottom = rect.bottom;
			addRect = false;
		
		} else if (rect.top == data[last].top && rect.bottom == data[last].bottom) {			
			if (rect.left == data[last].right + 1) {

				data[last].right = rect.right;
				addRect = false;

			} else if (rect.right == data[last].left - 1) {

				data[last].left = rect.left;
				addRect = false;
			}	
		}
	}		
	
	// We weren't lucky.... just add the rect as a new one
	if (addRect) {
		if (data_size <= count)
			set_size(count + 16);
			
		data[count] = rect;
		
		count++;
	}
	
	// Recalculate bounds
	if (rect.top < bound.top)
		bound.top = rect.top;

	if (rect.left < bound.left)
		bound.left = rect.left;
 
	if (rect.right > bound.right)
		bound.right = rect.right;
 
	if (rect.bottom > bound.bottom)
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
	
	data = (clipping_rect *)realloc(data, new_size * sizeof(clipping_rect));
	
	if (data == NULL)
		debugger("BRegion::set_size realloc error\n");
	
	data_size = new_size;
	
	ASSERT(count <= data_size);		
}
