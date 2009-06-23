/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <SplitView.h>

#include <stdio.h>

#include <ControlLook.h>
#include <Cursor.h>

#include "SplitLayout.h"


static const unsigned char kHSplitterCursor[] = {
	16, 1, 8, 8,
	0x03, 0xc0, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
	0x1a, 0x58, 0x2a, 0x54, 0x4a, 0x52, 0x8a, 0x51,
	0x8a, 0x51, 0x4a, 0x52, 0x2a, 0x54, 0x1a, 0x58,
	0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x03, 0xc0,

	0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0,
	0x1b, 0xd8, 0x3b, 0xdc, 0x7b, 0xde, 0xfb, 0xdf,
	0xfb, 0xdf, 0x7b, 0xde, 0x3b, 0xdc, 0x1b, 0xd8,
	0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0
};

static const unsigned char kVSplitterCursor[] = {
	16, 1, 8, 8,
	0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10,
	0x0f, 0xf0, 0x00, 0x00, 0xff, 0xff, 0x80, 0x01,
	0x80, 0x01, 0xff, 0xff, 0x00, 0x00, 0x0f, 0xf0,
	0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80,

	0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0,
	0x0f, 0xf0, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x0f, 0xf0,
	0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80
};


// constructor
BSplitView::BSplitView(enum orientation orientation, float spacing)
	:
	BView(NULL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_INVALIDATE_AFTER_LAYOUT,
		fSplitLayout = new BSplitLayout(orientation, spacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// destructor
BSplitView::~BSplitView()
{
}

// SetInsets
void
BSplitView::SetInsets(float left, float top, float right, float bottom)
{
	fSplitLayout->SetInsets(left, top, right, bottom);
}

// GetInsets
void
BSplitView::GetInsets(float* left, float* top, float* right,
	float* bottom) const
{
	fSplitLayout->GetInsets(left, top, right, bottom);
}

// Spacing
float
BSplitView::Spacing() const
{
	return fSplitLayout->Spacing();
}

// SetSpacing
void
BSplitView::SetSpacing(float spacing)
{
	fSplitLayout->SetSpacing(spacing);
}

// Orientation
orientation
BSplitView::Orientation() const
{
	return fSplitLayout->Orientation();
}

// SetOrientation
void
BSplitView::SetOrientation(enum orientation orientation)
{
	fSplitLayout->SetOrientation(orientation);
}

// SplitterSize
float
BSplitView::SplitterSize() const
{
	return fSplitLayout->SplitterSize();
}

// SetSplitterSize
void
BSplitView::SetSplitterSize(float size)
{
	fSplitLayout->SetSplitterSize(size);
}

// SetCollapsible
void
BSplitView::SetCollapsible(bool collapsible)
{
	fSplitLayout->SetCollapsible(collapsible);
}

// SetCollapsible
void
BSplitView::SetCollapsible(int32 index, bool collapsible)
{
	fSplitLayout->SetCollapsible(index, collapsible);
}

// SetCollapsible
void
BSplitView::SetCollapsible(int32 first, int32 last, bool collapsible)
{
	fSplitLayout->SetCollapsible(first, last, collapsible);
}

// AddChild
void
BSplitView::AddChild(BView* child, BView* sibling)
{
	BView::AddChild(child, sibling);
}

// AddChild
bool
BSplitView::AddChild(BView* child, float weight)
{
	return fSplitLayout->AddView(child, weight);
}

// AddChild
bool
BSplitView::AddChild(int32 index, BView* child, float weight)
{
	return fSplitLayout->AddView(index, child, weight);
}

// AddChild
bool
BSplitView::AddChild(BLayoutItem* child)
{
	return fSplitLayout->AddItem(child);
}

// AddChild
bool
BSplitView::AddChild(BLayoutItem* child, float weight)
{
	return fSplitLayout->AddItem(child, weight);
}

// AddChild
bool
BSplitView::AddChild(int32 index, BLayoutItem* child, float weight)
{
	return fSplitLayout->AddItem(index, child, weight);
}

// Draw
void
BSplitView::Draw(BRect updateRect)
{
	// draw the splitters
	int32 draggedSplitterIndex = fSplitLayout->DraggedSplitter();
	int32 count = fSplitLayout->CountItems();
	for (int32 i = 0; i < count - 1; i++) {
		BRect frame = fSplitLayout->SplitterItemFrame(i);
		DrawSplitter(frame, updateRect, Orientation(),
			draggedSplitterIndex == i);
	}
}

// MouseDown
void
BSplitView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS);

	if (fSplitLayout->StartDraggingSplitter(where))
		Invalidate();
}

// MouseUp
void
BSplitView::MouseUp(BPoint where)
{
	if (fSplitLayout->StopDraggingSplitter()) {
		Relayout();
		Invalidate();
	}
}

// MouseMoved
void
BSplitView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	BCursor cursor(B_CURSOR_SYSTEM_DEFAULT);

	int32 splitterIndex = fSplitLayout->DraggedSplitter();

	if (splitterIndex >= 0 || fSplitLayout->IsAboveSplitter(where)) {
		if (Orientation() == B_VERTICAL)
			cursor = BCursor(kVSplitterCursor);
		else
			cursor = BCursor(kHSplitterCursor);
	}

	if (splitterIndex >= 0) {
		BRect oldFrame = fSplitLayout->SplitterItemFrame(splitterIndex);
		if (fSplitLayout->DragSplitter(where)) {
			Invalidate(oldFrame);
			Invalidate(fSplitLayout->SplitterItemFrame(splitterIndex));
		}
	}

	SetViewCursor(&cursor, true);
}

// SetLayout
void
BSplitView::SetLayout(BLayout* layout)
{
	// not allowed
}

// DrawSplitter
void
BSplitView::DrawSplitter(BRect frame, const BRect& updateRect,
	enum orientation orientation, bool pressed)
{
	_DrawDefaultSplitter(this, frame, updateRect, orientation, pressed);
}

// _DrawDefaultSplitter
void
BSplitView::_DrawDefaultSplitter(BView* view, BRect frame,
	const BRect& updateRect, enum orientation orientation, bool pressed)
{
	uint32 flags = pressed ? BControlLook::B_ACTIVATED : 0;
	be_control_look->DrawSplitter(view, frame, updateRect, view->ViewColor(),
		orientation, flags, 0);
}
