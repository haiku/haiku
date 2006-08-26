/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <SplitView.h>

#include "SplitLayout.h"


// constructor
BSplitView::BSplitView(enum orientation orientation, float spacing)
	: BView(NULL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_INVALIDATE_AFTER_LAYOUT,
		fSplitLayout = new BSplitLayout(orientation, spacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// destructor
BSplitView::~BSplitView()
{
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
		DrawSplitter(frame, Orientation(), draggedSplitterIndex == i);
	}
}

// MouseDown
void
BSplitView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

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
	int32 splitterIndex = fSplitLayout->DraggedSplitter();
	if (splitterIndex >= 0) {
		BRect oldFrame = fSplitLayout->SplitterItemFrame(splitterIndex);
		if (fSplitLayout->DragSplitter(where)) {
			Invalidate(oldFrame);
			Invalidate(fSplitLayout->SplitterItemFrame(splitterIndex));
		}
	}
}

// SetLayout
void
BSplitView::SetLayout(BLayout* layout)
{
	// not allowed
}

// DrawSplitter
void
BSplitView::DrawSplitter(BRect frame, enum orientation orientation,
	bool pressed)
{
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color white = { 255, 255, 255, 255 };

	SetHighColor(pressed ? white : black);

	FillRect(frame);
}
