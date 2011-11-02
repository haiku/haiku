/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <SplitView.h>

#include <stdio.h>

#include <Archivable.h>
#include <ControlLook.h>
#include <Cursor.h>

#include "SplitLayout.h"


BSplitView::BSplitView(enum orientation orientation, float spacing)
	:
	BView(NULL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_INVALIDATE_AFTER_LAYOUT,
		fSplitLayout = new BSplitLayout(orientation, spacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BSplitView::BSplitView(BMessage* from)
	:
	BView(BUnarchiver::PrepareArchive(from)),
	fSplitLayout(NULL)
{
	BUnarchiver(from).Finish();
}


BSplitView::~BSplitView()
{
}


void
BSplitView::SetInsets(float left, float top, float right, float bottom)
{
	left = BControlLook::ComposeSpacing(left);
	top = BControlLook::ComposeSpacing(top);
	right = BControlLook::ComposeSpacing(right);
	bottom = BControlLook::ComposeSpacing(bottom);

	fSplitLayout->SetInsets(left, top, right, bottom);
}


void
BSplitView::SetInsets(float horizontal, float vertical)
{
	horizontal = BControlLook::ComposeSpacing(horizontal);
	vertical = BControlLook::ComposeSpacing(vertical);
	fSplitLayout->SetInsets(horizontal, vertical, horizontal, vertical);
}


void
BSplitView::SetInsets(float insets)
{
	insets = BControlLook::ComposeSpacing(insets);
	fSplitLayout->SetInsets(insets, insets, insets, insets);
}


void
BSplitView::GetInsets(float* left, float* top, float* right,
	float* bottom) const
{
	fSplitLayout->GetInsets(left, top, right, bottom);
}


float
BSplitView::Spacing() const
{
	return fSplitLayout->Spacing();
}


void
BSplitView::SetSpacing(float spacing)
{
	fSplitLayout->SetSpacing(spacing);
}


orientation
BSplitView::Orientation() const
{
	return fSplitLayout->Orientation();
}


void
BSplitView::SetOrientation(enum orientation orientation)
{
	fSplitLayout->SetOrientation(orientation);
}


float
BSplitView::SplitterSize() const
{
	return fSplitLayout->SplitterSize();
}


void
BSplitView::SetSplitterSize(float size)
{
	fSplitLayout->SetSplitterSize(size);
}


int32
BSplitView::CountItems() const
{
	return fSplitLayout->CountItems();
}


float
BSplitView::ItemWeight(int32 index) const
{
	return fSplitLayout->ItemWeight(index);
}


float
BSplitView::ItemWeight(BLayoutItem* item) const
{
	return fSplitLayout->ItemWeight(item);
}


void
BSplitView::SetItemWeight(int32 index, float weight, bool invalidateLayout)
{
	fSplitLayout->SetItemWeight(index, weight, invalidateLayout);
}


void
BSplitView::SetItemWeight(BLayoutItem* item, float weight)
{
	fSplitLayout->SetItemWeight(item, weight);
}


void
BSplitView::SetCollapsible(bool collapsible)
{
	fSplitLayout->SetCollapsible(collapsible);
}


void
BSplitView::SetCollapsible(int32 index, bool collapsible)
{
	fSplitLayout->SetCollapsible(index, collapsible);
}


void
BSplitView::SetCollapsible(int32 first, int32 last, bool collapsible)
{
	fSplitLayout->SetCollapsible(first, last, collapsible);
}


void
BSplitView::AddChild(BView* child, BView* sibling)
{
	BView::AddChild(child, sibling);
}


bool
BSplitView::AddChild(BView* child, float weight)
{
	return fSplitLayout->AddView(child, weight);
}


bool
BSplitView::AddChild(int32 index, BView* child, float weight)
{
	return fSplitLayout->AddView(index, child, weight);
}


bool
BSplitView::AddChild(BLayoutItem* child)
{
	return fSplitLayout->AddItem(child);
}


bool
BSplitView::AddChild(BLayoutItem* child, float weight)
{
	return fSplitLayout->AddItem(child, weight);
}


bool
BSplitView::AddChild(int32 index, BLayoutItem* child, float weight)
{
	return fSplitLayout->AddItem(index, child, weight);
}


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


void
BSplitView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS);

	if (fSplitLayout->StartDraggingSplitter(where))
		Invalidate();
}


void
BSplitView::MouseUp(BPoint where)
{
	if (fSplitLayout->StopDraggingSplitter()) {
		Relayout();
		Invalidate();
	}
}


void
BSplitView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	BCursor cursor(B_CURSOR_ID_SYSTEM_DEFAULT);

	int32 splitterIndex = fSplitLayout->DraggedSplitter();

	if (splitterIndex >= 0 || fSplitLayout->IsAboveSplitter(where)) {
		if (Orientation() == B_VERTICAL)
			cursor = BCursor(B_CURSOR_ID_RESIZE_NORTH_SOUTH);
		else
			cursor = BCursor(B_CURSOR_ID_RESIZE_EAST_WEST);
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


void
BSplitView::SetLayout(BLayout* layout)
{
	// not allowed
}


status_t
BSplitView::Archive(BMessage* into, bool deep) const
{
	return BView::Archive(into, deep);
}


status_t
BSplitView::AllUnarchived(const BMessage* from)
{
	status_t err = BView::AllUnarchived(from);
	if (err == B_OK) {
		fSplitLayout = dynamic_cast<BSplitLayout*>(GetLayout());
		if (!fSplitLayout && GetLayout())
			return B_BAD_TYPE;
		else if (!fSplitLayout)
			return B_ERROR;
	}
	return err;
}


BArchivable*
BSplitView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BSplitView"))
		return new BSplitView(from);
	return NULL;
}


void
BSplitView::DrawSplitter(BRect frame, const BRect& updateRect,
	enum orientation orientation, bool pressed)
{
	_DrawDefaultSplitter(this, frame, updateRect, orientation, pressed);
}


void
BSplitView::_DrawDefaultSplitter(BView* view, BRect frame,
	const BRect& updateRect, enum orientation orientation, bool pressed)
{
	uint32 flags = pressed ? BControlLook::B_ACTIVATED : 0;
	be_control_look->DrawSplitter(view, frame, updateRect, view->ViewColor(),
		orientation, flags, 0);
}
