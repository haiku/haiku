/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Group.h"

#include <stdio.h>

// constructor
Group::Group(BRect frame, const char* name, orientation direction)
	: BView(frame, name, B_FOLLOW_ALL, B_FRAME_EVENTS),
	  fOrientation(direction),
	  fInset(4.0),
	  fSpacing(0.0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// destructor
Group::~Group()
{
}

// AttachedToWindow
void
Group::AttachedToWindow()
{
	// trigger a layout
	FrameResized(Bounds().Width(), Bounds().Height());
}

// FrameResized
void
Group::FrameResized(float width, float height)
{
	// layout controls
	BRect r(Bounds());
	r.InsetBy(fInset, fInset);
	_LayoutControls(r);
}

// GetPreferredSize
void
Group::GetPreferredSize(float* width, float* height)
{
	BRect r(_MinFrame());

	if (width)
		*width = r.Width();
	if (height)
		*height = r.Height();
}

// #pragma mark -

void
Group::SetSpacing(float insetFromBorder, float childSpacing)
{
	if (fSpacing == childSpacing && fInset == insetFromBorder)
		return;

	fInset = insetFromBorder;
	fSpacing = childSpacing;
	// trigger a layout
	FrameResized(Bounds().Width(), Bounds().Height());
}

// #pragma mark -

// _LayoutControls
void
Group::_LayoutControls(BRect r) const
{
	if (fOrientation == B_HORIZONTAL) {
		r.left -= fSpacing;
		for (int32 i = 0; BView* child = ChildAt(i); i++) {
			r.right = r.left + child->Bounds().Width() + 2 * fSpacing;
			_LayoutControl(child, r);
			r.left = r.right + 1.0 - fSpacing;
		}
	} else {
		r.top -= fSpacing;
		for (int32 i = 0; BView* child = ChildAt(i); i++) {
			r.bottom = r.top + child->Bounds().Height() + 2 * fSpacing;
			_LayoutControl(child, r);
			r.top = r.bottom + 1.0 - fSpacing;
		}
	}
}

// _MinFrame
BRect           
Group::_MinFrame() const
{
	float minWidth = fInset * 2.0;
	float minHeight = fInset * 2.0;

	if (fOrientation == B_HORIZONTAL) {
		minWidth += fSpacing;
		for (int32 i = 0; BView* child = ChildAt(i); i++) {
			minWidth += child->Bounds().Width() + 1 + fSpacing;
			minHeight = max_c(minHeight,
							  child->Bounds().Height() + 1 + fInset * 2.0);
		}
	} else {
		minHeight += fSpacing;
		for (int32 i = 0; BView* child = ChildAt(i); i++) {
			minHeight += child->Bounds().Height() + 1 + fSpacing;
			minWidth = max_c(minWidth,
							 child->Bounds().Width() + 1 + fInset * 2.0);
		}
	}

	return BRect(0.0, 0.0, minWidth - 1.0, minHeight - 1.0);
}

// _LayoutControl
void
Group::_LayoutControl(BView* view, BRect frame,
					  bool resizeWidth, bool resizeHeight) const
{
	if (!resizeHeight)
		// center vertically
		frame.top = (frame.top + frame.bottom) / 2.0 - view->Bounds().Height() / 2.0;
	if (!resizeWidth)
		// center horizontally
		frame.left = (frame.left + frame.right) / 2.0 - view->Bounds().Width() / 2.0;
	view->MoveTo(frame.LeftTop());
	float width = resizeWidth ? frame.Width() : view->Bounds().Width();
	float height = resizeHeight ? frame.Height() : view->Bounds().Height();
	if (resizeWidth || resizeHeight)
		view->ResizeTo(width, height);
}

