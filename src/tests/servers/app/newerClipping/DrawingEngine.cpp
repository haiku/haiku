
#include <stdio.h>
#include <stack.h>

#include <Bitmap.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>

#include "Desktop.h"

#include "DrawingEngine.h"

// constructor
DrawingEngine::DrawingEngine(BRect frame, DrawView* drawView)
	: BView(frame, "drawing engine", B_FOLLOW_ALL, B_WILL_DRAW),
	  fDrawView(drawView)
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
	return Window()->Lock();
}

// Unlock
void
DrawingEngine::Unlock()
{
#if RUN_WITH_FRAME_BUFFER
	Sync();
#else
//	Flush();
#endif
	Window()->Unlock();
}

// MarkDirty
void
DrawingEngine::MarkDirty(BRegion* region)
{
#if RUN_WITH_FRAME_BUFFER
	BRect frame = region->Frame();
	if (frame.IsValid()) {
		BMessage message(MSG_INVALIDATE);
		message.AddRect("area", frame);
		fDrawView->Looper()->PostMessage(&message, fDrawView);
	}
#endif
}

// MarkDirty
void
DrawingEngine::MarkDirty(BRect rect)
{
#if RUN_WITH_FRAME_BUFFER
	if (rect.IsValid()) {
		BMessage message(MSG_INVALIDATE);
		message.AddRect("area", rect);
		fDrawView->Looper()->PostMessage(&message, fDrawView);
	}
#endif
}

// MarkDirty
void
DrawingEngine::MarkDirty()
{
	if (Lock()) {
		Invalidate();
		Unlock();
	}
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
}

// constructor
DrawView::DrawView(BRect frame)
#if RUN_WITH_FRAME_BUFFER
	: BView(frame, "desktop", B_FOLLOW_ALL, B_WILL_DRAW),
#else
	: DrawingEngine(frame, NULL),
#endif
	  fDesktop(NULL),

#if RUN_WITH_FRAME_BUFFER
	  fFrameBuffer(new BBitmap(Bounds(), B_RGB32, true)),
	  fDrawingEngine(new DrawingEngine(Bounds(), this))
#else
	  fDrawingEngine(this)
#endif
{
	SetViewColor(B_TRANSPARENT_COLOR);
#if RUN_WITH_FRAME_BUFFER
	if (fFrameBuffer->Lock()) {
		fFrameBuffer->AddChild(fDrawingEngine);
		fFrameBuffer->Unlock();
	}
#endif
}

// destructor
DrawView::~DrawView()
{
#if RUN_WITH_FRAME_BUFFER
	delete fFrameBuffer;
#endif
}

// MessageReceived
void
DrawView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_INVALIDATE: {
			BRect area;
			if (message->FindRect("area", &area) == B_OK) {
				Invalidate(area);
			}
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}

// Draw
void
DrawView::Draw(BRect updateRect)
{
#if RUN_WITH_FRAME_BUFFER
	DrawBitmap(fFrameBuffer, updateRect, updateRect);
#else
	BMessage message(MSG_DRAW);
	message.AddRect("area", updateRect);
	fDesktop->PostMessage(&message);
#endif
}

// MouseDown
void
DrawView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	fDesktop->PostMessage(Window()->CurrentMessage());
}

// MouseUp
void
DrawView::MouseUp(BPoint where)
{
	fDesktop->PostMessage(Window()->CurrentMessage());
}

// MouseMoved
void
DrawView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	fDesktop->PostMessage(Window()->CurrentMessage());
}

// SetDesktop
void
DrawView::SetDesktop(Desktop* desktop)
{
	fDesktop = desktop;
}


