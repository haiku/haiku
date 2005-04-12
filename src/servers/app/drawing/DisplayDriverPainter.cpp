//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		DisplayDriverPainter.cpp
//	Author:			Stephan Aßmus <superstippi@gmx.de>
//	Description:	Implementation of DisplayDriver based on top of Painter
//  
//------------------------------------------------------------------------------
#include <stdio.h>
#include <algo.h>

#include "Painter.h"
#include "RenderingBuffer.h"

#ifdef __HAIKU__
#define USE_ACCELERANT 1
#else
#define USE_ACCELERANT 0
#endif


#if USE_ACCELERANT
  #include "AccelerantHWInterface.h"
#else
  #include "ViewHWInterface.h"
#endif

#include "DisplayDriverPainter.h"

// TODO: fix invalidation, what about pensize, scale, origin?
// Painter calls could return the bounding box arround all
// pixels touched by a drawing operation


// constructor
DisplayDriverPainter::DisplayDriverPainter()
	: DisplayDriver(),
	  fPainter(new Painter()),
#if USE_ACCELERANT
	  fGraphicsCard(new AccelerantHWInterface())
#else
	  fGraphicsCard(new ViewHWInterface())
#endif
{
}

// destructor
DisplayDriverPainter::~DisplayDriverPainter()
{
	delete fGraphicsCard;
}

// Initialize
bool
DisplayDriverPainter::Initialize()
{
	status_t err = fGraphicsCard->Initialize();
	if (err < B_OK)
		fprintf(stderr, "HWInterface::Initialize() failed: %s\n", strerror(err));
	if (err >= B_OK)
		return DisplayDriver::Initialize();
	return false;
}

// Shutdown
void
DisplayDriverPainter::Shutdown()
{
	DisplayDriver::Shutdown();
}

// CopyBits
void
DisplayDriverPainter::CopyBits(const BRect &src, const BRect &dst,
							   const DrawData *d)
{
	if (Lock()) {
		// TODO: handle clipping to d->clipreg here?

		RenderingBuffer* backBuffer = fGraphicsCard->BackBuffer();

		BRect valid(0, 0, backBuffer->Width() - 1, backBuffer->Height() - 1);
		if (valid.Intersects(src) && valid.Intersects(dst)) {

			// calculate offset before any clipping
			int32 xOffset = (int32)(dst.left - src.left);
			int32 yOffset = (int32)(dst.top - src.top);

			// clip src and dest
			BRect a = src & valid;
			BRect b = dst & valid;
			a = a & b.OffsetByCopy(BPoint(-xOffset, -yOffset));
			b = b & a.OffsetByCopy(BPoint(xOffset, yOffset));
	
			uint8* bits = (uint8*)backBuffer->Bits();
			uint32 bpr = backBuffer->BytesPerRow();
	
			uint32 width = a.IntegerWidth() + 1;
			uint32 height = a.IntegerHeight() + 1;
	
			int32 left = (int32)a.left;
			int32 top = (int32)a.top;
	
			// offset to left top of src rect
			bits += top * bpr + left * 4;
	
			_MoveRect(bits, width, height, bpr, xOffset, yOffset);
	
			fGraphicsCard->Invalidate(dst);
		}

		Unlock();
	}
}

// CopyRegion
void
DisplayDriverPainter::CopyRegion(BRegion *src, const BPoint &lefttop)
{
printf("DisplayDriverPainter::CopyRegion()\n");
}

// InvertRect
void
DisplayDriverPainter::InvertRect(const BRect &r)
{
	if (Lock()) {

		fPainter->InvertRect(r);
		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// DrawBitmap
void
DisplayDriverPainter::DrawBitmap(BRegion *region, ServerBitmap *bitmap,
								 const BRect &source, const BRect &dest,
								 const DrawData *d)
{
	// catch PicturePlayer giving us a NULL region
	if (!region)
		return;

	if (Lock()) {

		fPainter->ConstrainClipping(*region);
		fPainter->SetDrawData(d);
		fPainter->DrawBitmap(bitmap, source, dest);

		fGraphicsCard->Invalidate(dest);

		Unlock();
	}
}

/*
inline int
compare_left_right_top_bottom(BRect* a, BRect* b)
{
	if (b->right < a->left || b->bottom < a->top)
		return 1;
	else
		return -1;
	return 0;
}

inline int
compare_right_left_top_bottom(BRect* a, BRect* b)
{
	if (b->right > a->left || b->bottom < a->top)
		return 1;
	else
		return -1;
	return 0;
}

inline int
compare_left_right_bottom_top(BRect* a, BRect* b)
{
	if (b->right < a->left || b->bottom > a->top)
		return 1;
	else
		return -1;
	return 0;
}

inline int
compare_right_left_bottom_top(BRect* a, BRect* b)
{
	if (b->right > a->left || b->bottom > a->top)
		return 1;
	else
		return -1;
	return 0;
}
*/

// TODO: the commented out code is completely broken
// what needs to be done is a topological sort of the rectangles
// in a given BRegion.
// For example, let's suppose these rects are in a BRegion:
//                        ************
//                        *    B     *
//      *************     ************
//      *           *
//      *     A     ***************
//      *           *             *
//      *************             *
//                  *     C       *
//                  *             *
//                  *             *
//                  ***************
// When moving stuff from LEFT TO RIGHT, TOP TO BOTTOM, the
// result of the sort should be C, B, A, ie, you take an unsorted
// list of rects, and you go look for the one that has no neighbors
// to the right and to the bottom, that's the first one you want
// to move. If you move from RIGHT TO LEFT, BOTTOM TO TOP, you
// go look for the one that has no neighbors to the top and left.
//
// Here I draw some rays to illustrate LEFT TO RIGHT, TOP TO BOTTOM:
//                        ************
//                        *    B     *
//      *************     ************
//      *           *
//      *     A     ***************-----------------
//      *           *             *
//      *************             *
//                  *     C       *
//                  *             *
//                  *             *
//                  ***************
//                  |
//                  |
//                  |
//                  |
// There are no rects in the area defined by the rays to the right
// and bottom of rect C, so that's the one we want to copy first
// (for positive x and y offsets). If the sorting of rects can
// be implemented, the speed-up of the CopyRegionList() function
// will be about 300% (I tested that) with "in-place" copying.
//
// Of course, the usage of the funcion is completely obscure to me.
// First of all, why is there a list of points? I checked and it
// is indeed always the same point (the same offset used for all rects).
// Second, if the Regions provided in the list somehow overlap, then
// an in-place copy cannot be performed, so this all makes no sense.
// However during my tests, the usage of the function
// (from Layer::move_to() btw) was only with a *single* BRegion in
// the list. Which makes most sense, but it doesn't make sense to pass
// a BList then.

// used to move windows arround on screen
//
// CopyRegionList
void
DisplayDriverPainter::CopyRegionList(BList* list, BList* pList,
									 int32 rCount, BRegion* clipReg)
{
	if (!clipReg || !Lock())
		return;
/*
// This is the same implementation as in DisplayDriverImpl for now

// since we're not using painter.... this won't do much good, will it?
//	fPainter->ConstrainClipping(*clipReg);
//bigtime_t now = system_time();

		BRect updateRect;

		RenderingBuffer* backBuffer = fGraphicsCard->BackBuffer();

		uint8* bits = (uint8*)backBuffer->Bits();
		uint32 bpr = backBuffer->BytesPerRow();

		// TODO: it's always the same point, no?
		BPoint offset = *((BPoint*)pList->ItemAt(0));

		// iterate over regions (and points)
		for (int32 i = 0; i < rCount; i++) {
			BRegion* region = (BRegion*)list->ItemAt(i);

			// copy rects into a list
			int32 rectCount = region->CountRects();
			BList rects(rectCount);
			for (int32 j = 0; j < rectCount; j++) {
				BRect* r = new BRect(region->RectAt(j));
				rects.AddItem((void*)r);
			}
			// sort the rects according to offset
			BRect** first = (BRect**)rects.Items();
			BRect** behindLast = (BRect**)rects.Items() + rects.CountItems();

			int32 xOffset = (int32)offset.x;
			int32 yOffset = (int32)offset.y;

			if (xOffset >= 0) {
				if (yOffset >= 0)
					sort(first, behindLast, compare_left_right_top_bottom);
				else
					sort(first, behindLast, compare_left_right_bottom_top);
			} else {
				if (yOffset >= 0)
					sort(first, behindLast, compare_right_left_top_bottom);
				else
					sort(first, behindLast, compare_right_left_bottom_top);
			}


			// iterate over rects in region
			for (int32 j = 0; j < rectCount; j++) {
				BRect* r = (BRect*)rects.ItemAt(j);

				uint32 width = r->IntegerWidth() + 1;
				uint32 height = r->IntegerHeight() + 1;

				int32 left = (int32)r->left;
				int32 top = (int32)r->top;

// TODO: out of bounds checking!
//				BPoint offset = *((BPoint*)pList->ItemAt(j % rCount));
				// keep track of dirty rect
				r->OffsetBy(offset);
				updateRect = updateRect.IsValid() ? updateRect | *r : *r;

printf("region: %ld, rect: %ld, offset(%ld, %ld)\n", i, j, xOffset, yOffset);

				// offset to left top of src rect
				uint8* src = bits + top * bpr + left * 4;

				_MoveRect(src, width, height, bpr, xOffset, yOffset);

				delete r;
			}
		}

*/
	RenderingBuffer* bmp = fGraphicsCard->BackBuffer();

	uint32		bytesPerPixel	= bmp->BytesPerRow() / bmp->Width();
	BList		rectList;
	int32		i, k;
	uint8		*bitmapBits = (uint8*)bmp->Bits();
	int32		Bwidth		= bmp->Width();
	int32		Bheight		= bmp->Height();

	BRect updateRect;

	for (k = 0; k < rCount; k++) {
		BRegion* reg = (BRegion*)list->ItemAt(k);

		int32 rectCount = reg->CountRects();
		for(i=0; i < rectCount; i++) {
			BRect		r		= reg->RectAt(i);
			uint8		*rectCopy;
			uint8		*srcAddress;
			uint8		*destAddress;
			int32		firstRow, lastRow;
			int32		firstCol, lastCol;
			int32		copyLength;
			int32		copyRows;
	
			firstRow	= (int32)(r.top < 0? 0: r.top);
			lastRow		= (int32)(r.bottom > (Bheight-1)? (Bheight-1): r.bottom);
			firstCol	= (int32)(r.left < 0? 0: r.left);
			lastCol		= (int32)(r.right > (Bwidth-1)? (Bwidth-1): r.right);
			copyLength	= (lastCol - firstCol + 1) < 0? 0: (lastCol - firstCol + 1);
			copyRows	= (lastRow - firstRow + 1) < 0? 0: (lastRow - firstRow + 1);
	
			rectCopy	= (uint8*)malloc(copyLength * copyRows * bytesPerPixel);
	
			srcAddress	= bitmapBits + (((firstRow) * Bwidth + firstCol) * bytesPerPixel);
			destAddress	= rectCopy;
	
			for (int32 j = 0; j < copyRows; j++) {
				uint8		*destRowAddress	= destAddress + (j * copyLength * bytesPerPixel);
				uint8		*srcRowAddress	= srcAddress + (j * Bwidth * bytesPerPixel);
				memcpy(destRowAddress, srcRowAddress, copyLength * bytesPerPixel );
			}
		
			rectList.AddItem(rectCopy);
		}
	}

	int32 item = 0;
	for(k = 0; k < rCount; k++) {
		BRegion* reg = (BRegion*)list->ItemAt(k);
	
		int32 rectCount = reg->CountRects();
		for(i=0; i < rectCount; i++)
		{
			BRect		r		= reg->RectAt(i);
			uint8		*rectCopy;
			uint8		*srcAddress;
			uint8		*destAddress;
			int32		firstRow, lastRow;
			int32		firstCol, lastCol;
			int32		copyLength, copyLength2;
			int32		copyRows, copyRows2;
	
			firstRow	= (int32)(r.top < 0? 0: r.top);
			lastRow		= (int32)(r.bottom > (Bheight-1)? (Bheight-1): r.bottom);
			firstCol	= (int32)(r.left < 0? 0: r.left);
			lastCol		= (int32)(r.right > (Bwidth-1)? (Bwidth-1): r.right);
			copyLength	= (lastCol - firstCol + 1) < 0? 0: (lastCol - firstCol + 1);
			copyRows	= (lastRow - firstRow + 1) < 0? 0: (lastRow - firstRow + 1);
	
			rectCopy	= (uint8*)rectList.ItemAt(item++);
	
			srcAddress	= rectCopy;
	
			r.Set(firstCol, firstRow, lastCol, lastRow);
			r.OffsetBy( *((BPoint*)pList->ItemAt(k%rCount)) );

			// keep track of invalid area
			updateRect = updateRect.IsValid() ? updateRect | r : r;

			firstRow	= (int32)(r.top < 0? 0: r.top);
			lastRow		= (int32)(r.bottom > (Bheight-1)? (Bheight-1): r.bottom);
			firstCol	= (int32)(r.left < 0? 0: r.left);
			lastCol		= (int32)(r.right > (Bwidth-1)? (Bwidth-1): r.right);
			copyLength2	= (lastCol - firstCol + 1) < 0? 0: (lastCol - firstCol + 1);
			copyRows2	= (lastRow - firstRow + 1) < 0? 0: (lastRow - firstRow + 1);
	
			destAddress	= bitmapBits + (((firstRow) * Bwidth + firstCol) * bytesPerPixel);
	
			int32		minLength	= copyLength < copyLength2? copyLength: copyLength2;
			int32		minRows		= copyRows < copyRows2? copyRows: copyRows2;
	
			for (int32 j = 0; j < minRows; j++)
			{
				uint8		*destRowAddress	= destAddress + (j * Bwidth * bytesPerPixel);
				uint8		*srcRowAddress	= srcAddress + (j * copyLength * bytesPerPixel);
				memcpy(destRowAddress, srcRowAddress, minLength * bytesPerPixel );
			}
		}
	}

	int32 count = rectList.CountItems();
	for (i = 0; i < count; i++) {
		if (void* rectCopy = rectList.ItemAt(i))
			free(rectCopy);
	}

	fGraphicsCard->Invalidate(updateRect);

	Unlock();
}

// FillArc
void
DisplayDriverPainter::FillArc(const BRect &r, const float &angle,
							  const float &span, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		fPainter->FillArc(center, xRadius, yRadius, angle, span);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillBezier
void
DisplayDriverPainter::FillBezier(BPoint *pts, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		fPainter->FillBezier(pts);

		// TODO: Invalidate

		Unlock();
	}
}

// FillEllipse
void
DisplayDriverPainter::FillEllipse(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		fPainter->FillEllipse(center, xRadius, yRadius);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillPolygon
void
DisplayDriverPainter::FillPolygon(BPoint *ptlist, int32 numpts,
								  const BRect &bounds, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->FillPolygon(ptlist, numpts);

		fGraphicsCard->Invalidate(bounds);

		Unlock();
	}
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const RGBColor &color)
{
	if (Lock()) {

		fPainter->SetHighColor(color);
		BRect touched = fPainter->FillRect(r);

		fGraphicsCard->Invalidate(touched);

		Unlock();
	}
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		BRect touched = fPainter->FillRect(r);

		fGraphicsCard->Invalidate(touched);

		Unlock();
	}
}

// FillRegion
void
DisplayDriverPainter::FillRegion(BRegion& r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		BRect invalid = r.RectAt(0);
		fPainter->FillRect(invalid);

		int32 count = r.CountRects();
		for (int32 i = 1; i < count; i++) {
			fPainter->FillRect(r.RectAt(i));
			invalid = invalid | r.RectAt(i);
		}

		fGraphicsCard->Invalidate(invalid);

		Unlock();
	}
}

// FillRoundRect
void
DisplayDriverPainter::FillRoundRect(const BRect &r,
									const float &xrad, const float &yrad,
									const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->FillRoundRect(r, xrad, yrad);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillShape
void
DisplayDriverPainter::FillShape(const BRect &bounds,
								const int32 &opcount, const int32 *oplist, 
								const int32 &ptcount, const BPoint *ptlist,
								const DrawData *d)
{
	if (Lock()) {

printf("DisplayDriverPainter::FillShape() - what is this stuff that gets passed here?\n");

		Unlock();
	}
}

// FillTriangle
void
DisplayDriverPainter::FillTriangle(BPoint *pts, const BRect &bounds,
								   const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->FillTriangle(pts[0], pts[1], pts[2]);

		fGraphicsCard->Invalidate(bounds);

		Unlock();
	}
}

// StrokeArc
void
DisplayDriverPainter::StrokeArc(const BRect &r, const float &angle,
								const float &span, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		fPainter->StrokeArc(center, xRadius, yRadius, angle, span);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// StrokeBezier
void
DisplayDriverPainter::StrokeBezier(BPoint *pts, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		fPainter->StrokeBezier(pts);

		// TODO: Invalidate

		Unlock();
	}
}

// StrokeEllipse
void
DisplayDriverPainter::StrokeEllipse(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		fPainter->StrokeEllipse(center, xRadius, yRadius);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// StrokeLine
//
// * this function is only used by Decorators
// * it assumes a one pixel wide line
void
DisplayDriverPainter::StrokeLine(const BPoint &start, const BPoint &end, const RGBColor &color)
{
	if (Lock()) {
		if (!fPainter->StraightLine(start, end, color.GetColor32())) {
			DrawData context;
			context.highcolor = color;
			context.draw_mode = B_OP_COPY;
			StrokeLine(start, end, &context);
		} else {
			fGraphicsCard->Invalidate(fPainter->ClipRect(BRect(start, end)));
		}
		Unlock();
	}
}

// StrokeLine
void
DisplayDriverPainter::StrokeLine(const BPoint &start, const BPoint &end, DrawData* context)
{
	if (Lock()) {
		BRect touched = fPainter->StrokeLine(start, end, context);
		fGraphicsCard->Invalidate(touched);
		Unlock();
	}
}

// StrokePoint
//
// * this function is only used by Decorators
void
DisplayDriverPainter::StrokePoint(const BPoint& pt, const RGBColor &color)
{
	StrokeLine(pt, pt, color);
}

// StrokePoint
void
DisplayDriverPainter::StrokePoint(const BPoint& pt, DrawData *context)
{
	StrokeLine(pt, pt, context);
}

// StrokePolygon
void
DisplayDriverPainter::StrokePolygon(BPoint *ptlist, int32 numpts,
									const BRect &bounds, const DrawData *d,
									bool closed)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->StrokePolygon(ptlist, numpts, closed);

		fGraphicsCard->Invalidate(bounds);

		Unlock();
	}
}

// StrokeRect
// 
// this function is used to draw a one pixel wide rect
void
DisplayDriverPainter::StrokeRect(const BRect &r, const RGBColor &color)
{
	if (Lock()) {
		fPainter->StrokeRect(r, color.GetColor32());

		fGraphicsCard->Invalidate(BRect(r.left, r.top, r.right, r.top));
		fGraphicsCard->Invalidate(BRect(r.left, r.top + 1, r.left, r.bottom - 1));
		fGraphicsCard->Invalidate(BRect(r.right, r.top + 1, r.right, r.bottom - 1));
		fGraphicsCard->Invalidate(BRect(r.left, r.bottom, r.right, r.bottom));

		Unlock();
	}
}

// StrokeRect
void
DisplayDriverPainter::StrokeRect(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		BRect touched = fPainter->StrokeRect(r);

		fGraphicsCard->Invalidate(touched);

		Unlock();
	}
}

// StrokeRegion
void
DisplayDriverPainter::StrokeRegion(BRegion& r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		BRect invalid = fPainter->StrokeRect(r.RectAt(0));

		int32 count = r.CountRects();
		for (int32 i = 1; i < count; i++) {
			invalid = invalid | fPainter->StrokeRect(r.RectAt(i));
		}

		fGraphicsCard->Invalidate(invalid);

		Unlock();
	}
}

// StrokeRoundRect
void
DisplayDriverPainter::StrokeRoundRect(const BRect &r, const float &xrad,
									  const float &yrad, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->StrokeRoundRect(r, xrad, yrad);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// StrokeShape
void
DisplayDriverPainter::StrokeShape(const BRect &bounds, const int32 &opcount,
								  const int32 *oplist, const int32 &ptcount,
								  const BPoint *ptlist, const DrawData *d)
{
	if (Lock()) {

printf("DisplayDriverPainter::StrokeShape() - what is this stuff that gets passed here?\n");

		Unlock();
	}
}

// StrokeTriangle
void
DisplayDriverPainter::StrokeTriangle(BPoint *pts, const BRect &bounds,
									 const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->StrokeTriangle(pts[0], pts[1], pts[2]);

		fGraphicsCard->Invalidate(bounds);

		Unlock();
	}
}

// DrawString
void
DisplayDriverPainter::DrawString(const char *string, const int32 &length,
								 const BPoint &pt, const RGBColor &color,
								 escapement_delta *delta)
{
	// what - without any clipping?!?
	DrawData d;
	d.highcolor=color;
	
	if(delta)
		d.edelta=*delta;
	DrawString(string,length,pt,&d);
}

// DrawString
void
DisplayDriverPainter::DrawString(const char *string, const int32 &length,
								 const BPoint &pt, DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		BRect boundingBox = fPainter->DrawString(string, length, pt);
		fGraphicsCard->Invalidate(boundingBox);

		Unlock();
	}
}

// StringWidth
float
DisplayDriverPainter::StringWidth(const char *string, int32 length,
								  const DrawData *d)
{
	float width = 0.0;
	if (Lock()) {
		fPainter->SetDrawData(d);
		BPoint dummy(0.0, 0.0);
		width = fPainter->BoundingBox(string, length, dummy).Width();
		Unlock();
	}
	return width;
}

// StringHeight
float
DisplayDriverPainter::StringHeight(const char *string, int32 length,
								   const DrawData *d)
{
	float height = 0.0;
	if (Lock()) {
		fPainter->SetDrawData(d);
		BPoint dummy(0.0, 0.0);
		height = fPainter->BoundingBox(string, length, dummy).Height();
		Unlock();
	}
	return height;
}

// GetBoundingBoxes
void
DisplayDriverPainter::GetBoundingBoxes(const char *string, int32 count, 
									   font_metric_mode mode,
									   escapement_delta *delta,
									   BRect *rectarray, const DrawData *d)
{
	// ?!? each glyph or what?
	printf("DisplayDriverPainter::GetBoundingBoxes()\n");
}

// GetEscapements
void
DisplayDriverPainter::GetEscapements(const char *string, int32 charcount, 
									 escapement_delta *delta,
									 escapement_delta *escapements,
									 escapement_delta *offsets,
									 const DrawData *d)
{
	printf("DisplayDriverPainter::GetEscapements()\n");
}

// GetEdges
void
DisplayDriverPainter::GetEdges(const char *string, int32 charcount,
							   edge_info *edgearray, const DrawData *d)
{
	printf("DisplayDriverPainter::GetEdges()\n");
}

// GetHasGlyphs
void DisplayDriverPainter::GetHasGlyphs(const char *string, int32 charcount,
										bool *hasarray)
{
	printf("DisplayDriverPainter::GetHasGlyphs()\n");
}

// GetTruncatedStrings
void
DisplayDriverPainter::GetTruncatedStrings(const char **instrings,
										  const int32 &stringcount, 
										  const uint32 &mode,
										  const float &maxwidth,
										  char **outstrings)
{
	printf("DisplayDriverPainter::GetTruncatedStrings()\n");
}

// HideCursor
void
DisplayDriverPainter::HideCursor()
{
	fGraphicsCard->SetCursorVisible(false);
}

// IsCursorHidden
bool
DisplayDriverPainter::IsCursorHidden()
{
	return !fGraphicsCard->IsCursorVisible();
}

// MoveCursorTo
void
DisplayDriverPainter::MoveCursorTo(const float &x, const float &y)
{
	fGraphicsCard->MoveCursorTo(x, y);
}

// ShowCursor
void
DisplayDriverPainter::ShowCursor()
{
	fGraphicsCard->SetCursorVisible(true);
}

// ObscureCursor
void
DisplayDriverPainter::ObscureCursor()
{
	// TODO: I don't think this has anything to do with the DisplayDriver
	// implement elsewhere!!
}

// SetCursor
void
DisplayDriverPainter::SetCursor(ServerCursor *cursor)
{
	fGraphicsCard->SetCursor(cursor);
}

// GetCursorPosition
BPoint
DisplayDriverPainter::GetCursorPosition()
{
	return fGraphicsCard->GetCursorPosition();
}

// IsCursorObscured
bool
DisplayDriverPainter::IsCursorObscured(bool state)
{
	// TODO: I don't think this has anything to do with the DisplayDriver
	// implement elsewhere!!
	return false;
}

// Lock
bool
DisplayDriverPainter::Lock(bigtime_t timeout)
{
	bool success = false;
	// NOTE: I'm hoping I don't change the semantics and implications of
	// the original implementation, but I need the locker to be somewhere
	// else in order to serialize only the access to the back buffer
	if (timeout == B_INFINITE_TIMEOUT) {
		success = fGraphicsCard->Lock();
//printf("DisplayDriverPainter::Lock()\n");
	} else {
		success = (fGraphicsCard->LockWithTimeout(timeout) >= B_OK) ? true : false;
//printf("DisplayDriverPainter::LockWithTimeout(): %d\n", success);
	}
	
	return success;
}

// Unlock
void
DisplayDriverPainter::Unlock()
{
//printf("DisplayDriverPainter::Unlock()\n");
	fGraphicsCard->Unlock();
}

// SetMode
void
DisplayDriverPainter::SetMode(const display_mode &mode)
{
	if (Lock()) {
		if (fGraphicsCard->SetMode(mode) >= B_OK) {
			fPainter->AttachToBuffer(fGraphicsCard->BackBuffer());
			DisplayDriver::SetMode(mode);
		} else {
			fprintf(stderr, "DisplayDriverPainter::SetMode() - unsupported "
				"mode!\n");
		}
		Unlock();
	}
}

// DumpToFile
bool
DisplayDriverPainter::DumpToFile(const char *path)
{
#if 0
	// TODO: find out why this does not work
	SaveToPNG(	path,
				BRect(0, 0, fDisplayMode.virtual_width - 1, fDisplayMode.virtual_height - 1),
				(color_space)fDisplayMode.space,
				mFrameBufferConfig.frame_buffer,
				fDisplayMode.virtual_height * mFrameBufferConfig.bytes_per_row,
				mFrameBufferConfig.bytes_per_row);*/
#else
	// TODO: remove this when SaveToPNG works properly
	
	// this does dump each line at a time to ensure that everything
	// gets written even if we crash somewhere.
	// it's a bit overkill, but works for now.
	FILE *output = fopen(path, "w");
	if (!output)
		return false;
	
	RenderingBuffer *back_buffer = fGraphicsCard->BackBuffer();
	uint32 row = back_buffer->BytesPerRow() / 4;
	for (int i = 0; i < fDisplayMode.virtual_height; i++) {
		fwrite((uint32 *)back_buffer->Bits() + i * row, 4, row, output);
		sync();
	}
	fclose(output);
	sync();
#endif
	
	return true;
}

// DumpToBitmap
ServerBitmap*
DisplayDriverPainter::DumpToBitmap()
{
	return NULL;
}

// StrokeLineArray
void
DisplayDriverPainter::StrokeLineArray(const int32 &numlines,
									  const LineArrayData *linedata,
									  const DrawData *d)
{
	if(!d || !linedata || numlines <= 0)
		return;
	
	if (Lock()) {
		DrawData context;
		context.draw_mode = B_OP_COPY;
		const LineArrayData *data;
		
		data = (const LineArrayData *)&(linedata[0]);
		context.highcolor = data->color;
		BRect touched = fPainter->StrokeLine(data->pt1, data->pt2, &context);
		
		for (int32 i = 1; i < numlines; i++) {
			data = (const LineArrayData *)&(linedata[i]);
			context.highcolor = data->color;
			touched = touched | fPainter->StrokeLine(data->pt1, data->pt2, &context);
		}
		
		fGraphicsCard->Invalidate(touched);
		Unlock();
	}
}

// SetDPMSMode
status_t
DisplayDriverPainter::SetDPMSMode(const uint32 &state)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->SetDPMSMode(state);
		Unlock();
	}
	return ret;
}

// DPMSMode
uint32
DisplayDriverPainter::DPMSMode()
{
	uint32 mode = 0;
	if (Lock()) {
		mode = fGraphicsCard->DPMSMode();
		Unlock();
	}
	return mode;
}

// DPMSCapabilities
uint32
DisplayDriverPainter::DPMSCapabilities()
{
	uint32 caps = 0;
	if (Lock()) {
		caps = fGraphicsCard->DPMSMode();
		Unlock();
	}
	return caps;
}

// GetDeviceInfo
status_t
DisplayDriverPainter::GetDeviceInfo(accelerant_device_info *info)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->GetDeviceInfo(info);
		Unlock();
	}
	return ret;
}

// GetModeList
status_t
DisplayDriverPainter::GetModeList(display_mode **mode_list, uint32 *count)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->GetModeList(mode_list, count);
		Unlock();
	}
	return ret;
}

// GetPixelClockLimits
status_t DisplayDriverPainter::GetPixelClockLimits(display_mode *mode,
												   uint32 *low,
												   uint32 *high)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->GetPixelClockLimits(mode, low, high);
		Unlock();
	}
	return ret;
}

// GetTimingConstraints
status_t
DisplayDriverPainter::GetTimingConstraints(display_timing_constraints *dtc)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->GetTimingConstraints(dtc);
		Unlock();
	}
	return ret;
}

// ProposeMode
status_t
DisplayDriverPainter::ProposeMode(display_mode *candidate,
								  const display_mode *low,
								  const display_mode *high)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->ProposeMode(candidate, low, high);
		Unlock();
	}
	return ret;
}

// WaitForRetrace
status_t
DisplayDriverPainter::WaitForRetrace(bigtime_t timeout)
{
	status_t ret = B_ERROR;
	if (Lock()) {
		ret = fGraphicsCard->WaitForRetrace(timeout);
		Unlock();
	}
	return ret;
}

// CopyBitmap
void
DisplayDriverPainter::CopyBitmap(ServerBitmap *bitmap,
								 const BRect &source, const BRect &dest,
								 const DrawData *d)
{
}

// CopyToBitmap
void DisplayDriverPainter::CopyToBitmap(ServerBitmap *target,
										const BRect &source)
{
}

// Invalidate
void DisplayDriverPainter::Invalidate(const BRect &r)
{
	// nothing to be done here
}

// ConstrainClippingRegion
void DisplayDriverPainter::ConstrainClippingRegion(BRegion *region)
{
	if (Lock()) {
		if (!region) { 
//			BRegion empty;
//			fPainter->ConstrainClipping(empty);
			if (RenderingBuffer* buffer = fGraphicsCard->BackBuffer()) {
				BRegion all;
				all.Include(BRect(0, 0, buffer->Width() - 1,
					buffer->Height() - 1));
				fPainter->ConstrainClipping(all);
			}
		} else {
			fPainter->ConstrainClipping(*region);
		}
		Unlock();
	}
}

// _MoveRect
void
DisplayDriverPainter::_MoveRect(uint8* src, uint32 width, uint32 height,
								uint32 bpr, int32 xOffset, int32 yOffset) const
{
	int32 xIncrement;
	int32 yIncrement;

	if (xOffset > 0) {
		// copy from right to left
		xIncrement = -1;
		src += width * 4;
	} else {
		// copy from left to right
		xIncrement = 1;
	}

	if (yOffset > 0) {
		// copy from bottom to top
		yIncrement = -bpr;
		src += height * bpr;
	} else {
		// copy from top to bottom
		yIncrement = bpr;
	}

	uint8* dst = src + yOffset * bpr + xOffset * 4;

	for (uint32 y = 0; y < height; y++) {
		uint32* srcHandle = (uint32*)src;
		uint32* dstHandle = (uint32*)dst;
		for (uint32 x = 0; x < width; x++) {
			*dstHandle = *srcHandle;
			srcHandle += xIncrement;
			dstHandle += xIncrement;
		}
		src += yIncrement;
		dst += yIncrement;
	}
}

