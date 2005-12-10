
#include <stdio.h>
#include <stack.h>

#include <Region.h>

#include "AccelerantHWInterface.h"
#include "DirectWindowBuffer.h"

#include "DrawingEngine.h"

// constructor
DrawingEngine::DrawingEngine(AccelerantHWInterface* interface,
							 DirectWindowBuffer* buffer)
	: fHWInterface(interface),
	  fBuffer(buffer),
	  fCurrentClipping()
{
}

// destructor
DrawingEngine::~DrawingEngine()
{
}

// Lock
bool
DrawingEngine::Lock()
{
	return fHWInterface->Lock();
}

// Unlock
void
DrawingEngine::Unlock()
{
	fHWInterface->Unlock();
}

// ConstrainClipping
void
DrawingEngine::ConstrainClipping(BRegion* region)
{
	if (region)
		fCurrentClipping = *region;
	else
		fCurrentClipping.MakeEmpty();
}

// StraightLine
bool
DrawingEngine::StraightLine(BPoint a, BPoint b, const rgb_color& c)
{
	uint8* dst = (uint8*)fBuffer->Bits();
	uint32 bpr = fBuffer->BytesPerRow();

	if (dst && fCurrentClipping.Frame().IsValid()) {

		int32 clipBoxCount = fCurrentClipping.CountRects();

		uint32 color;
		color = (255 << 24) | (c.red << 16) | (c.green << 8) | (c.blue);

		if (a.x == b.x) {
			// vertical
			int32 x = (int32)a.x;
			dst += x * 4;
			int32 y1 = (int32)min_c(a.y, b.y);
			int32 y2 = (int32)max_c(a.y, b.y);
			// draw a line, iterate over clipping boxes
			for (int32 i = 0; i < clipBoxCount; i++) {
				clipping_rect rect = fCurrentClipping.RectAtInt(i);

				if (rect.left <= x &&
					rect.right >= x) {
					int32 i = max_c(rect.top, y1);
					int32 end = min_c(rect.bottom, y2);
					uint8* handle = dst + i * bpr;
					for (; i <= end; i++) {
						*(uint32*)handle = color;
						handle += bpr;
					}
				}
			}
	
			return true;
	
		} else if (a.y == b.y) {
			// horizontal
			int32 y = (int32)a.y;
			dst += y * bpr;
			int32 x1 = (int32)min_c(a.x, b.x);
			int32 x2 = (int32)max_c(a.x, b.x);
			// draw a line, iterate over clipping boxes
			for (int32 i = 0; i < clipBoxCount; i++) {
				clipping_rect rect = fCurrentClipping.RectAtInt(i);

				if (rect.top <= y &&
					rect.bottom >= y) {
					int32 i = max_c(rect.left, x1);
					int32 end = min_c(rect.right, x2);
					uint32* handle = (uint32*)(dst + i * 4);
					for (; i <= end; i++) {
						*handle++ = color;
					}
				}
			}	

			return true;
		}
	}
	return false;
}

// StrokeLine
void
DrawingEngine::StrokeLine(BPoint a, BPoint b, const rgb_color& color)
{
	if (!StraightLine(a, b, color)) {
		// ...
	}
}

// StrokeRect
void
DrawingEngine::StrokeRect(BRect r, const rgb_color& color)
{
	StrokeLine(r.LeftTop(), r.RightTop(), color);
	StrokeLine(r.RightTop(), r.RightBottom(), color);
	StrokeLine(r.RightBottom(), r.LeftBottom(), color);
	StrokeLine(r.LeftBottom(), r.LeftTop(), color);
}

// FillRegion
void
DrawingEngine::FillRegion(BRegion *region, const rgb_color& color)
{
	if (Lock()) {
		// for speed reasons, expected to be already clipped
		fHWInterface->FillRegion(*region, color);

		Unlock();
	}
}

// DrawString
void
DrawingEngine::DrawString(const char* string, BPoint baseLine,
						  const rgb_color& color)
{
}




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

void
DrawingEngine::CopyRegion(BRegion* region, int32 xOffset, int32 yOffset)
{
	if (Lock()) {

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

		clipping_rect* sortedRectList = new clipping_rect[count];
		int32 nextSortedIndex = 0;

		while (!inDegreeZeroNodes.empty()) {
			node* n = inDegreeZeroNodes.top();
			inDegreeZeroNodes.pop();

			sortedRectList[nextSortedIndex].left	= (int32)n->rect.left;
			sortedRectList[nextSortedIndex].top		= (int32)n->rect.top;
			sortedRectList[nextSortedIndex].right	= (int32)n->rect.right;
			sortedRectList[nextSortedIndex].bottom	= (int32)n->rect.bottom;
			nextSortedIndex++;

			for (int32 k = 0; k < n->next_pointer; k++) {
				n->pointers[k]->in_degree--;
				if (n->pointers[k]->in_degree == 0)
					inDegreeZeroNodes.push(n->pointers[k]);
			}
		}

		// trigger the HW accelerated blit
		fHWInterface->CopyRegion(sortedRectList, count, xOffset, yOffset);

		delete[] sortedRectList;

		Unlock();
	}
}
