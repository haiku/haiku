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
#include <stack.h>

#include "LayerData.h"
#include "Painter.h"
#include "PNGDump.h"
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

// constructor
DisplayDriverPainter::DisplayDriverPainter()
	: DisplayDriver(),
	  fPainter(new Painter()),
#if USE_ACCELERANT
	  fGraphicsCard(new AccelerantHWInterface()),
#else
	  fGraphicsCard(new ViewHWInterface()),
#endif
	  fAvailableHWAccleration(0)
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
	if (err >= B_OK) {
		fAvailableHWAccleration = fGraphicsCard->AvailableHWAcceleration();
		return DisplayDriver::Initialize();
	}
	return false;
}

// Shutdown
void
DisplayDriverPainter::Shutdown()
{
	DisplayDriver::Shutdown();
}

// ConstrainClippingRegion
void DisplayDriverPainter::ConstrainClippingRegion(BRegion *region)
{
	if (Lock()) {
		if (!region) {
//			BRegion empty;
//			fPainter->ConstrainClipping(empty);
			if (RenderingBuffer* buffer = fGraphicsCard->DrawingBuffer()) {
				BRegion all;
				all.Include(BRect(0, 0, buffer->Width() - 1, buffer->Height() - 1));
				fPainter->ConstrainClipping(all);
			}
		} else {
			fPainter->ConstrainClipping(*region);
		}
		Unlock();
	}
}

// CopyRegion() does a topological sort of the rects in the
// region. The algorithm was suggested by Ingo Weinhold.
// It compares each rect with each rect and builds a tree
// of successors so we know the order in which they can be copied.
// For example, let's suppose these rects are in a BRegion:
//                        ************
//                        *    B     *
//                        ************
//      *************
//      *           *
//      *     A     ****************
//      *           **             *
//      **************             *
//                   *     C       *
//                   *             *
//                   *             *
//                   ***************
// When copying stuff from LEFT TO RIGHT, TOP TO BOTTOM, the
// result of the sort will be C, A, B. For this direction, we search
// for the rects that have no neighbors to their right and to their
// bottom, These can be copied without drawing into the area of
// rects yet to be copied. If you move from RIGHT TO LEFT, BOTTOM TO TOP,
// you go look for the ones that have no neighbors to their top and left.
//
// Here I draw some rays to illustrate LEFT TO RIGHT, TOP TO BOTTOM:
//                        ************
//                        *    B     *
//                        ************
//      *************
//      *           *
//      *     A     ****************-----------------
//      *           **             *
//      **************             *
//                   *     C       *
//                   *             *
//                   *             *
//                   ***************
//                   |
//                   |
//                   |
//                   |
// There are no rects in the area defined by the rays to the right
// and bottom of rect C, so that's the one we want to copy first
// (for positive x and y offsets).
// Since A is to the left of C and B is to the top of C, The "node"
// for C will point to the nodes of A and B as its "successors". Therefor,
// A and B will have an "indegree" of 1 for C pointing to them. C will
// have and "indegree" of 0, because there was no rect to which C
// was to the left or top of. When comparing A and B, neither is left
// or top from the other and in the sense that the algorithm cares about.

// NOTE: comparisson of coordinates assumes that rects don't overlap
// and don't share the actual edge either (as is the case in BRegions).

struct node {
			node()
			{
				pointers = NULL;
			}
			node(const BRect& r, int32 maxPointers)
			{
				init(r, maxPointers);
			}
			~node()
			{
				delete [] pointers;
			}

	void	init(const BRect& r, int32 maxPointers)
			{
				rect = r;
				pointers = new node*[maxPointers];
				in_degree = 0;
				next_pointer = 0;
			}

	void	push(node* node)
			{
				pointers[next_pointer] = node;
				next_pointer++;
			}
	node*	top()
			{
				return pointers[next_pointer];
			}
	node*	pop()
			{
				node* ret = top();
				next_pointer--;
				return ret;
			}

	BRect	rect;
	int32	in_degree;
	node**	pointers;
	int32	next_pointer;
};

bool
is_left_of(const BRect& a, const BRect& b)
{
	return (a.right < b.left);
}
bool
is_above(const BRect& a, const BRect& b)
{
	return (a.bottom < b.top);
}

// CopyRegion
void
DisplayDriverPainter::CopyRegion(/*const*/ BRegion* region,
								 int32 xOffset, int32 yOffset)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor(region->Frame());

		int32 count = region->CountRects();

		// TODO: make this step unnecessary
		// (by using different stack impl inside node)
		node nodes[count];
		for (int32 i= 0; i < count; i++) {
			nodes[i].init(region->RectAt(i), count);
		}

		for (int32 i = 0; i < count; i++) {
			BRect a = region->RectAt(i);
			for (int32 k = i + 1; k < count; k++) {
				BRect b = region->RectAt(k);
				int cmp = 0;
				// compare horizontally
				if (xOffset > 0) {
					if (is_left_of(a, b)) {
						cmp -= 1;
					} else if (is_left_of(b, a)) {
						cmp += 1;
					}
				} else if (xOffset < 0) {
					if (is_left_of(a, b)) {
						cmp += 1;
					} else if (is_left_of(b, a)) {
						cmp -= 1;
					}
				}
				// compare vertically
				if (yOffset > 0) {
					if (is_above(a, b)) {
						cmp -= 1;	
					} else if (is_above(b, a)) {
						cmp += 1;
					}
				} else if (yOffset < 0) {
					if (is_above(a, b)) {
						cmp += 1;
					} else if (is_above(b, a)) {
						cmp -= 1;
					}
				}
				// add appropriate node as successor
				if (cmp > 0) {
					nodes[i].push(&nodes[k]);
					nodes[k].in_degree++;
				} else if (cmp < 0) {
					nodes[k].push(&nodes[i]);
					nodes[i].in_degree++;
				}
			}
		}
		// put all nodes onto a stack that have an "indegree" count of zero
		stack<node*> inDegreeZeroNodes;
		for (int32 i = 0; i < count; i++) {
			if (nodes[i].in_degree == 0) {
				inDegreeZeroNodes.push(&nodes[i]);
			}
		}
		// pop the rects from the stack, do the actual copy operation
		// and decrease the "indegree" count of the other rects not
		// currently on the stack and to which the current rect pointed
		// to. If their "indegree" count reaches zero, put them onto the
		// stack as well.

		clipping_rect* sortedRectList = NULL;
		int32 nextSortedIndex = 0;

		if (fAvailableHWAccleration & HW_ACC_COPY_REGION)
			sortedRectList = new clipping_rect[count];

		while (!inDegreeZeroNodes.empty()) {
			node* n = inDegreeZeroNodes.top();
			inDegreeZeroNodes.pop();

			// do the software implementation or add to sorted
			// rect list for using the HW accelerated version
			// later
			if (sortedRectList) {
				sortedRectList[nextSortedIndex].left	= (int32)n->rect.left;
				sortedRectList[nextSortedIndex].top		= (int32)n->rect.top;
				sortedRectList[nextSortedIndex].right	= (int32)n->rect.right;
				sortedRectList[nextSortedIndex].bottom	= (int32)n->rect.bottom;
				nextSortedIndex++;
			} else {
				BRect touched = _CopyRect(n->rect, xOffset, yOffset);
				fGraphicsCard->Invalidate(touched);
			}

			for (int32 k = 0; k < n->next_pointer; k++) {
				n->pointers[k]->in_degree--;
				if (n->pointers[k]->in_degree == 0)
					inDegreeZeroNodes.push(n->pointers[k]);
			}
		}

		// trigger the HW accelerated version if is was available
		if (sortedRectList)
			fGraphicsCard->CopyRegion(sortedRectList, count, xOffset, yOffset);

		delete[] sortedRectList;

		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// CopyRegionList
//
// * used to move windows arround on screen
// TODO: Since the regions are passed to CopyRegion() one by one,
// the case of different regions overlapping each other at source
// and/or dest locations is not handled.
// We also don't handle the case where multiple regions should have
// been combined into one larger region (with effectively the same
// rects), so that the sorting would compare them all at once. If
// this turns out to be a problem, we can easily rewrite the function
// here. In any case, to copy overlapping regions in app_server doesn't
// make much sense to me.
void
DisplayDriverPainter::CopyRegionList(BList* list, BList* pList,
									 int32 rCount, BRegion* clipReg)
{
	if (Lock()) {

		for (int32 i = 0; i < rCount; i++) {
			BRegion* region = (BRegion*)list->ItemAt(i);
			BPoint* offset = (BPoint*)pList->ItemAt(i);
			CopyRegion(region, (int32)offset->x, (int32)offset->y);
		}

		Unlock();
	}
}

// InvertRect
void
DisplayDriverPainter::InvertRect(const BRect &r)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor(fPainter->ClipRect(r));

		BRect touched = fPainter->InvertRect(r);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// DrawBitmap
void
DisplayDriverPainter::DrawBitmap(ServerBitmap *bitmap,
								 const BRect &source, const BRect &dest,
								 const DrawData *d)
{
	if (Lock()) {
		BRect clipped = fPainter->ClipRect(dest);
		fGraphicsCard->HideSoftwareCursor(clipped);

		fPainter->SetDrawData(d);
		fPainter->DrawBitmap(bitmap, source, dest);

		fGraphicsCard->Invalidate(clipped);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// FillArc
void
DisplayDriverPainter::FillArc(const BRect &r, const float &angle,
							  const float &span, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		BRect touched = fPainter->FillArc(center, xRadius, yRadius, angle, span);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// FillBezier
void
DisplayDriverPainter::FillBezier(BPoint *pts, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);

		BRect touched = fPainter->FillBezier(pts);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// FillEllipse
void
DisplayDriverPainter::FillEllipse(const BRect &r, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Height() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		BRect touched = fPainter->FillEllipse(center, xRadius, yRadius);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// FillPolygon
void
DisplayDriverPainter::FillPolygon(BPoint *ptlist, int32 numpts,
								  const BRect &bounds, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->FillPolygon(ptlist, numpts);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect& r, const RGBColor& color)
{
	if (Lock()) {
		BRect vr(min_c(r.left, r.right),
				 min_c(r.top, r.bottom),
				 max_c(r.left, r.right),
				 max_c(r.top, r.bottom));
		vr = fPainter->ClipRect(vr);
		if (vr.IsValid()) {
			fGraphicsCard->HideSoftwareCursor(vr);
	
			// try hardware optimized version first
			if (fAvailableHWAccleration & HW_ACC_FILL_REGION) {
				BRegion region(vr);
				region.IntersectWith(fPainter->ClippingRegion());
				fGraphicsCard->FillRegion(region, color);
			} else {
				fPainter->FillRect(vr, color.GetColor32());
		
				fGraphicsCard->Invalidate(vr);
			}
	
			fGraphicsCard->ShowSoftwareCursor();
		}

		Unlock();
	}
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const DrawData *d)
{
	if (Lock()) {
		BRect vr(min_c(r.left, r.right),
				 min_c(r.top, r.bottom),
				 max_c(r.left, r.right),
				 max_c(r.top, r.bottom));
		vr = fPainter->ClipRect(vr);
		if (vr.IsValid()) {
			fGraphicsCard->HideSoftwareCursor(vr);
	
			bool doInSoftware = true;
			// try hardware optimized version first
			if ((fAvailableHWAccleration & HW_ACC_FILL_REGION) &&
				(d->GetDrawingMode() == B_OP_COPY ||
				 d->GetDrawingMode() == B_OP_OVER)) {
	
				if (d->GetPattern() == B_SOLID_HIGH) {
					BRegion region(vr);
					region.IntersectWith(fPainter->ClippingRegion());
					fGraphicsCard->FillRegion(region, d->HighColor());
					doInSoftware = false;
				} else if (d->GetPattern() == B_SOLID_LOW) {
					BRegion region(vr);
					region.IntersectWith(fPainter->ClippingRegion());
					fGraphicsCard->FillRegion(region, d->LowColor());
					doInSoftware = false;
				}
			}
			if (doInSoftware) {
	
				fPainter->SetDrawData(d);
				BRect touched = fPainter->FillRect(vr);
		
				fGraphicsCard->Invalidate(touched);
			}
	
			fGraphicsCard->ShowSoftwareCursor();
		}

		Unlock();
	}
}

// FillRegion
void
DisplayDriverPainter::FillRegion(BRegion& r, const DrawData *d)
{
	if (Lock()) {

		fGraphicsCard->HideSoftwareCursor(fPainter->ClipRect(r.Frame()));

		bool doInSoftware = true;
		// try hardware optimized version first
		if ((fAvailableHWAccleration & HW_ACC_FILL_REGION) &&
			(d->GetDrawingMode() == B_OP_COPY ||
			 d->GetDrawingMode() == B_OP_OVER)) {

			if (d->GetPattern() == B_SOLID_HIGH) {
				fGraphicsCard->FillRegion(r, d->HighColor());
				doInSoftware = false;
			} else if (d->GetPattern() == B_SOLID_LOW) {
				fGraphicsCard->FillRegion(r, d->LowColor());
				doInSoftware = false;
			}
		}
		if (doInSoftware) {

			fPainter->SetDrawData(d);
	
			BRect touched = fPainter->FillRect(r.RectAt(0));
	
			int32 count = r.CountRects();
			for (int32 i = 1; i < count; i++) {
				touched = touched | fPainter->FillRect(r.RectAt(i));
			}
	
			fGraphicsCard->Invalidate(touched);
		}

		fGraphicsCard->ShowSoftwareCursor();

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
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->FillRoundRect(r, xrad, yrad);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

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
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->FillTriangle(pts[0], pts[1], pts[2]);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// StrokeArc
void
DisplayDriverPainter::StrokeArc(const BRect &r, const float &angle,
								const float &span, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		BRect touched = fPainter->StrokeArc(center, xRadius, yRadius, angle, span);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// StrokeBezier
void
DisplayDriverPainter::StrokeBezier(BPoint *pts, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->StrokeBezier(pts);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// StrokeEllipse
void
DisplayDriverPainter::StrokeEllipse(const BRect &r, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Height() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		BRect touched = fPainter->StrokeEllipse(center, xRadius, yRadius);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

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
		BRect touched(min_c(start.x, end.x),
					  min_c(start.y, end.y),
					  max_c(start.x, end.x),
					  max_c(start.y, end.y));
		touched = fPainter->ClipRect(touched);
		fGraphicsCard->HideSoftwareCursor(touched);

		if (!fPainter->StraightLine(start, end, color.GetColor32())) {
			static DrawData context;
			context.SetHighColor(color);
			context.SetDrawingMode(B_OP_OVER);
			StrokeLine(start, end, &context);
		} else {
			fGraphicsCard->Invalidate(touched);
		}
		fGraphicsCard->ShowSoftwareCursor();
		Unlock();
	}
}

// StrokeLine
void
DisplayDriverPainter::StrokeLine(const BPoint &start, const BPoint &end, DrawData* context)
{
	if (Lock()) {
		BRect touched(min_c(start.x, end.x),
					  min_c(start.y, end.y),
					  max_c(start.x, end.x),
					  max_c(start.y, end.y));
		touched = fPainter->ClipRect(touched);
		fGraphicsCard->HideSoftwareCursor(touched);

		touched = fPainter->StrokeLine(start, end, context);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
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
		fGraphicsCard->HideSoftwareCursor();

		DrawData context;
		context.SetDrawingMode(B_OP_COPY);
		const LineArrayData *data;
		
		data = (const LineArrayData *)&(linedata[0]);
		context.SetHighColor(data->color);
		BRect touched = fPainter->StrokeLine(data->pt1, data->pt2, &context);
		
		for (int32 i = 1; i < numlines; i++) {
			data = (const LineArrayData *)&(linedata[i]);
			context.SetHighColor(data->color);
			touched = touched | fPainter->StrokeLine(data->pt1, data->pt2, &context);
		}
		
		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

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
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->StrokePolygon(ptlist, numpts, closed);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

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
		fGraphicsCard->HideSoftwareCursor();

		// support invalid rects
		BRect vr(min_c(r.left, r.right),
				 min_c(r.top, r.bottom),
				 max_c(r.left, r.right),
				 max_c(r.top, r.bottom));

		fPainter->StrokeRect(vr, color.GetColor32());

/*		fGraphicsCard->Invalidate(fPainter->ClipRect(BRect(vr.left, vr.top,
														   vr.right, vr.top)));
		fGraphicsCard->Invalidate(fPainter->ClipRect(BRect(vr.left, vr.top + 1,
														   vr.left, vr.bottom - 1)));
		fGraphicsCard->Invalidate(fPainter->ClipRect(BRect(vr.right, vr.top + 1,
														   vr.right, vr.bottom - 1)));
		fGraphicsCard->Invalidate(fPainter->ClipRect(BRect(vr.left, vr.bottom,
														   vr.right, vr.bottom)));*/
		fGraphicsCard->Invalidate(fPainter->ClipRect(vr));
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// StrokeRect
void
DisplayDriverPainter::StrokeRect(const BRect &r, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->StrokeRect(r);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// StrokeRegion
void
DisplayDriverPainter::StrokeRegion(BRegion& r, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);

		BRect touched = fPainter->StrokeRect(r.RectAt(0));

		int32 count = r.CountRects();
		for (int32 i = 1; i < count; i++) {
			touched = touched | fPainter->StrokeRect(r.RectAt(i));
		}

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// StrokeRoundRect
void
DisplayDriverPainter::StrokeRoundRect(const BRect &r, const float &xrad,
									  const float &yrad, const DrawData *d)
{
	if (Lock()) {
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->StrokeRoundRect(r, xrad, yrad);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

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
		fGraphicsCard->HideSoftwareCursor();

		fPainter->SetDrawData(d);
		BRect touched = fPainter->StrokeTriangle(pts[0], pts[1], pts[2]);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

		Unlock();
	}
}

// DrawString
void
DisplayDriverPainter::DrawString(const char *string, const int32 &length,
								 const BPoint &pt, const RGBColor &color,
								 escapement_delta *delta)
{
	static DrawData d;
	d.SetHighColor(color);
	
	if (delta)
		d.SetEscapementDelta(*delta);

	DrawString(string, length, pt, &d);
}

// DrawString
void
DisplayDriverPainter::DrawString(const char *string, const int32 &length,
								 const BPoint &pt, DrawData *d)
{
	if (Lock()) {
//bigtime_t now = system_time();
// TODO: BoundingBox is quite slow!! Optimizing it will be beneficial.
// Cursiously, the actual DrawString after it is actually faster!?!
// TODO: make the availability of the hardware cursor part of the 
// HW acceleration flags and skip all calculations for HideSoftwareCursor
// in case we don't need one.
		BRect b = fPainter->BoundingBox(string, length, pt);
//printf("bounding box '%s': %lld µs\n", string, system_time() - now);
		fGraphicsCard->HideSoftwareCursor(b);

		fPainter->SetDrawData(d);

//now = system_time();
		BRect touched = fPainter->DrawString(string, length, pt);
//printf("drawing string: %lld µs\n", system_time() - now);

		fGraphicsCard->Invalidate(touched);
		fGraphicsCard->ShowSoftwareCursor();

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
		width = fPainter->StringWidth(string, length);
		Unlock();
	}
	return width;
}

// StringWidth
float
DisplayDriverPainter::StringWidth(const char *string, int32 length,
								  const ServerFont &font)
{
	static DrawData d;
	d.SetFont(font);
	return StringWidth(string, length, &d);
}

// StringHeight
float
DisplayDriverPainter::StringHeight(const char *string, int32 length,
								   const DrawData *d)
{
	float height = 0.0;
	if (Lock()) {
		fPainter->SetDrawData(d);
		static BPoint dummy(0.0, 0.0);
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
			fPainter->AttachToBuffer(fGraphicsCard->DrawingBuffer());
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
	if (Lock()) {
		RenderingBuffer* buffer = fGraphicsCard->DrawingBuffer();
		if (buffer) {
			BRect bounds(0.0, 0.0, buffer->Width() - 1, buffer->Height() - 1);
			// TODO: Don't assume B_RGBA32!!
			SaveToPNG(path, bounds, B_RGBA32,
					  buffer->Bits(),
					  buffer->BitsLength(),
					  buffer->BytesPerRow());
		}
		Unlock();
	}	
	return true;
}

// DumpToBitmap
ServerBitmap*
DisplayDriverPainter::DumpToBitmap()
{
	return NULL;
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

// _CopyRect
BRect
DisplayDriverPainter::_CopyRect(BRect src, int32 xOffset, int32 yOffset) const
{
	BRect dst;
	RenderingBuffer* buffer = fGraphicsCard->DrawingBuffer();
	if (buffer) {

		BRect clip(0, 0, buffer->Width() - 1, buffer->Height() - 1);

		dst = src;
		dst.OffsetBy(xOffset, yOffset);

		if (clip.Intersects(src) && clip.Intersects(dst)) {

			uint32 bpr = buffer->BytesPerRow();
			uint8* bits = (uint8*)buffer->Bits();

			// clip source rect
			src = src & clip;
			// clip dest rect
			dst = dst & clip;
			// move dest back over source and clip source to dest
			dst.OffsetBy(-xOffset, -yOffset);
			src = src & dst;

			// calc offset in buffer
			bits += (int32)src.left * 4 + (int32)src.top * bpr;

			uint32 width = src.IntegerWidth() + 1;
			uint32 height = src.IntegerHeight() + 1;

			_CopyRect(bits, width, height, bpr, xOffset, yOffset);

			// offset dest again, because it is return value
			dst.OffsetBy(xOffset, yOffset);
		}
	}
	return dst;
}

// _CopyRect
void
DisplayDriverPainter::_CopyRect(uint8* src, uint32 width, uint32 height,
								uint32 bpr, int32 xOffset, int32 yOffset) const
{
	int32 xIncrement;
	int32 yIncrement;

	if (xOffset > 0) {
		// copy from right to left
		xIncrement = -1;
		src += (width - 1) * 4;
	} else {
		// copy from left to right
		xIncrement = 1;
	}

	if (yOffset > 0) {
		// copy from bottom to top
		yIncrement = -bpr;
		src += (height - 1) * bpr;
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

