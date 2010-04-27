/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DropTargetListView.h"


DropTargetListView::DropTargetListView(const char* name,
		list_view_type type, uint32 flags)
	: BListView(name, type, flags),
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

		SetHighColor(0, 0, 0);
		SetPenSize(2);
		BRect rect = Bounds();
// TODO: this is an incompatibility between R5 and Haiku and should be fixed!
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		rect.left++;
		rect.top++;
#else
		rect.right--;
		rect.bottom--;
#endif
		StrokeRect(rect);

		SetPenSize(1);
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
	// only update the parts affected by the change to reduce flickering
	BRect rect = Bounds();
	rect.right = rect.left + 1;
	Invalidate(rect);
	
	rect = Bounds();
	rect.left = rect.right - 1;
	Invalidate(rect);

	rect = Bounds();
	rect.bottom = rect.top + 1;
	Invalidate(rect);

	rect = Bounds();
	rect.top = rect.bottom - 1;
	Invalidate(rect);
}
