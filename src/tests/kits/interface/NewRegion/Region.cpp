//------------------------------------------------------------------------------
//	Copyright (c) 2003-2005, Haiku, Inc.
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


#include <cstdlib>
#include <cstring>

#include <new>

#include <Debug.h>
#include "Region.h"

#include "clipping.h"
#include "ClipRegion.h"



/*! \brief Initializes a region. The region will have no rects,
	and its bound will be invalid.
*/
BRegion::BRegion()
	:
	fRegion(new (nothrow) ClipRegion)
{
}


/*! \brief Initializes a region to be a copy of another.
	\param region The region to copy.
*/
BRegion::BRegion(const BRegion &region)
	:
	fRegion(new (nothrow) ClipRegion(*region.fRegion))
{	
}


/*!	\brief Initializes a region to contain a BRect.
	\param rect The BRect to set the region to.
*/
BRegion::BRegion(const BRect rect)
	:
	fRegion(new (nothrow) ClipRegion(to_clipping_rect(rect)))
{
}


/*!	\brief Frees the allocated memory.
*/
BRegion::~BRegion()
{
	delete fRegion;
}


/*! \brief Returns the bounds of the region.
	\return A BRect which represents the bounds of the region.
*/
BRect
BRegion::Frame() const
{
	return fRegion ? to_BRect(fRegion->Frame()) : BRect();
}


/*! \brief Returns the bounds of the region as a clipping_rect (which has integer coordinates).
	\return A clipping_rect which represents the bounds of the region.
*/
clipping_rect
BRegion::FrameInt() const
{
	return fRegion ? fRegion->Frame() : to_clipping_rect(BRect());
}


/*! \brief Returns the regions's BRect at the given index.
	\param index The index (zero based) of the wanted rectangle.
	\return If the given index is valid, it returns the BRect at that index,
		otherwise, it returns an invalid BRect.
*/
BRect
BRegion::RectAt(int32 index)
{
	return fRegion ? to_BRect(fRegion->RectAt(index)) : BRect();
}


/*! \brief Returns the regions's clipping_rect at the given index.
	\param index The index (zero based) of the wanted rectangle.
	\return If the given index is valid, it returns the clipping_rect at that index,
		otherwise, it returns an invalid clipping_rect.
*/
clipping_rect
BRegion::RectAtInt(int32 index)
{
	return fRegion ? fRegion->RectAt(index) : to_clipping_rect(BRect());
}


/*!	\brief Counts the region rects.
	\return An int32 which is the total number of rects in the region.
*/
int32
BRegion::CountRects()
{
	return fRegion ? fRegion->CountRects() : 0;
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
	if (fRegion)
		fRegion->Set(newBounds);
	else
		fRegion = new (nothrow) ClipRegion(newBounds);
}


/*!	\brief Check if the region has any area in common with the given BRect.
	\param rect The BRect to check the region against to.
	\return \ctrue if the region has any area in common with the BRect, \cfalse if not.
*/
bool
BRegion::Intersects(BRect rect) const
{
	return fRegion ? fRegion->Intersects(to_clipping_rect(rect)) : false;
}


/*!	\brief Check if the region has any area in common with the given clipping_rect.
	\param rect The clipping_rect to check the region against to.
	\return \ctrue if the region has any area in common with the clipping_rect, \cfalse if not.
*/
bool
BRegion::Intersects(clipping_rect rect) const
{
	return fRegion ? fRegion->Intersects(rect) : false;
}


/*!	\brief Check if the region contains the given BPoint.
	\param pt The BPoint to be checked.
	\return \ctrue if the region contains the BPoint, \cfalse if not.
*/
bool
BRegion::Contains(BPoint pt) const
{
	return fRegion ? fRegion->Contains(pt) : false;
}


/*!	\brief Check if the region contains the given coordinates.
	\param x The \cx coordinate of the point to be checked.
	\param y The \cy coordinate of the point to be checked.
	\return \ctrue if the region contains the point, \cfalse if not.
*/
bool
BRegion::Contains(int32 x, int32 y)
{
	if (fRegion == NULL)
		return false;
		
	const BPoint pt(x, y);
	return fRegion->Contains(pt);
}


/*!	\brief Prints the BRegion to stdout.
*/
void
BRegion::PrintToStream() const
{
	if (fRegion == NULL)
		return;
		
	Frame().PrintToStream();
	
	for (int32 c = 0; c < fRegion->CountRects(); c++) {
		clipping_rect rect = fRegion->RectAt(c);
		printf("data = BRect(l:%ld.0, t:%ld.0, r:%ld.0, b:%ld.0)\n",
			rect.left, rect.top, rect.right, rect.bottom);
	}	
}


/*!	\brief Offsets all region's rects, and bounds by the given values.
	\param dh The horizontal offset.
	\param dv The vertical offset.
*/
void
BRegion::OffsetBy(int32 dh, int32 dv)
{
	if (fRegion != NULL)
		fRegion->OffsetBy(dh, dv);
}


/*!	\brief Empties the region, so that it doesn't include any rect, and invalidates its bounds.
*/
void
BRegion::MakeEmpty()
{
	clipping_rect invalid = { 0, 0, -1, -1 };
	Set(invalid);
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
	if (fRegion != NULL)
		fRegion->Include(rect);
}


/*!	\brief Modifies the region, so that it includes the area of the given region.
	\param region The region to be included.
*/
void
BRegion::Include(const BRegion *region)
{
	if (fRegion != NULL)
		fRegion->Include(*region->fRegion);
}


/*!	\brief Modifies the region, excluding the area represented by the given BRect.
	\param rect The BRect to be excluded.
*/
void
BRegion::Exclude(BRect rect)
{
	if (fRegion)
		fRegion->Exclude(to_clipping_rect(rect));
}


/*!	\brief Modifies the region, excluding the area represented by the given clipping_rect.
	\param rect The clipping_rect to be excluded.
*/
void
BRegion::Exclude(clipping_rect rect)
{
	if (fRegion)
		fRegion->Exclude(rect);
}


/*!	\brief Modifies the region, excluding the area contained in the given BRegion.
	\param region The BRegion to be excluded.
*/
void
BRegion::Exclude(const BRegion *region)
{
	if (fRegion)
		fRegion->Exclude(*region->fRegion);
}


/*!	\brief Modifies the region, so that it will contain just the area in common with the given BRegion.
	\param region the BRegion to intersect to.
*/
void
BRegion::IntersectWith(const BRegion *region)
{
	if (fRegion)
		fRegion->IntersectWith(*region->fRegion);
}


/*!	\brief Modifies the region to be a copy of the given BRegion.
	\param region the BRegion to copy.
	\return This function always returns \c *this.
*/
BRegion &
BRegion::operator=(const BRegion &region)
{
	if (&region != this) {
		delete fRegion;
		fRegion = new (nothrow) ClipRegion(*region.fRegion);
	}
	
	return *this;
}
