/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DropTargetListView.h"


DropTargetListView::DropTargetListView(BRect frame, const char* name,
		list_view_type type, uint32 resizeMask, uint32 flags)
	: BListView(frame, name, type, resizeMask, flags),
	fDropTarget(false)
{
}


DropTargetListView::~DropTargetListView()
{
}


void
DropTargetListView::Draw(BRect updateRect)
{
	BListView::Draw(updateRect);

	if (fDropTarget) {
		// mark this view as a drop target
		rgb_color color = HighColor();
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(Bounds());
		SetHighColor(color);
	}
}


void
DropTargetListView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (dragMessage != NULL && AcceptsDrag(dragMessage)) {
		bool dropTarget = transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW;
		if (dropTarget != fDropTarget) {
			fDropTarget = dropTarget;
			_InvalidateFrame();
		}
	} else if (fDropTarget) {
		fDropTarget = false;
		_InvalidateFrame();
	}
}


bool
DropTargetListView::AcceptsDrag(const BMessage* /*message*/)
{
	return true;
}


void
DropTargetListView::_InvalidateFrame()
{
	Invalidate();
}
