#include <Message.h>
#include <Messenger.h>
#include <Window.h>

#include <stdio.h>
#include <stack.h>

#include "MyView.h"
#include "Layer.h"

extern BWindow *wind;
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

MyView::MyView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	fTracking	= false;
	fIsResize	= false;
	fIs2ndButton= false;
	fMovingLayer = NULL;

	rgb_color	col;
	col.red		= 49;
	col.green	= 101;
	col.blue	= 156;
	topLayer = new Layer(Bounds(), "topLayer", B_FOLLOW_ALL, 0, col);
	topLayer->SetRootLayer(this);

	topLayer->rebuild_visible_regions(BRegion(Bounds()), BRegion(Bounds()), NULL);
	fRedrawReg.Set(Bounds());
}

MyView::~MyView()
{
	delete topLayer;
}

Layer* MyView::FindLayer(Layer *lay, BPoint &where) const
{
	if (lay->Visible()->Contains(where))
		return lay;
	else
		for (Layer *child = lay->BottomChild(); child; child = lay->UpperSibling())
		{
			Layer	*found = FindLayer(child, where);
			if (found)
				return found;
		}
	return NULL;
}

void MyView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	int32		buttons;
	Looper()->CurrentMessage()->FindInt32("buttons", &buttons);
	fLastPos = where;
	if (buttons == B_PRIMARY_MOUSE_BUTTON)
	{
		fTracking = true;
		fMovingLayer = FindLayer(topLayer, where);
		if (fMovingLayer == topLayer)
			fMovingLayer = NULL;
		if (fMovingLayer)
		{
			BRect	bounds(fMovingLayer->Bounds());
			fMovingLayer->ConvertToScreen2(&bounds);
			BRect	resizeRect(bounds.right-10, bounds.bottom-10, bounds.right, bounds.bottom);
			if (resizeRect.Contains(where))
				fIsResize = true;
			else
				fIsResize = false;
		}
	}
	else if (buttons == B_SECONDARY_MOUSE_BUTTON)
	{
		fIs2ndButton = true;
	}
	else if (buttons == B_TERTIARY_MOUSE_BUTTON)
	{
		DrawSubTree(topLayer);
	}
}

void MyView::MouseUp(BPoint where)
{
	fTracking = false;
	fIs2ndButton = false;
	fMovingLayer = NULL;
}

void MyView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	if (fTracking)
	{
		float dx, dy;
		dx = where.x - fLastPos.x;
		dy = where.y - fLastPos.y;
		fLastPos = where;

		if (dx != 0 || dy != 0)
		{
			if (fMovingLayer)
			{
bigtime_t now = system_time();
				if (fIsResize) {
					fMovingLayer->ResizeBy(dx, dy);
printf("resizing: %lld\n", system_time() - now);
				} else {
					fMovingLayer->MoveBy(dx, dy);
printf("moving: %lld\n", system_time() - now);
				}
			}
		}
	}
	else if (fIs2ndButton)
	{
		SetHighColor(0,0,0);
		StrokeLine(fLastPos, where);
		Flush();
		fLastPos = where;
	}
}

void MyView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_MOUSE_WHEEL_CHANGED:
		{
			float	dy;
			msg->FindFloat("be:wheel_delta_y", &dy);

			BPoint	pt;
			uint32	buttons;
			Layer	*lay;
			GetMouse(&pt, &buttons, false);
			if ((lay = FindLayer(topLayer, pt)))
				lay->ScrollBy(0, dy*5);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void MyView::CopyRegion(BRegion *region, int32 xOffset, int32 yOffset)
{
wind->Lock();
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

		while (!inDegreeZeroNodes.empty()) {
			node* n = inDegreeZeroNodes.top();
			inDegreeZeroNodes.pop();

			CopyBits(n->rect, BRect(n->rect).OffsetByCopy(xOffset, yOffset));

			for (int32 k = 0; k < n->next_pointer; k++) {
				n->pointers[k]->in_degree--;
				if (n->pointers[k]->in_degree == 0)
					inDegreeZeroNodes.push(n->pointers[k]);
			}
		}
wind->Unlock();
}

void MyView::RequestRedraw()
{
	wind->Lock();

	ConstrainClippingRegion(&fRedrawReg);
	PushState();
	DrawSubTree(topLayer);
	PopState();
	ConstrainClippingRegion(NULL);

	fRedrawReg.MakeEmpty();

	wind->Unlock();
}

void MyView::Draw(BRect area)
{
	// empty. you can trigger a redraw by clicking the middle mouse button.
}

void MyView::DrawSubTree(Layer* lay)
{
//printf("======== %s =======\n", lay->Name());
//	lay->Visible()->PrintToStream();
//	lay->FullVisible()->PrintToStream();
	for (Layer *child = lay->BottomChild(); child; child = lay->UpperSibling())
		DrawSubTree(child);

	ConstrainClippingRegion(lay->Visible());
	SetHighColor(lay->HighColor());
	BRegion	reg;
	lay->GetWantedRegion(reg);
	FillRect(reg.Frame());
	Flush();
	ConstrainClippingRegion(NULL);
}
