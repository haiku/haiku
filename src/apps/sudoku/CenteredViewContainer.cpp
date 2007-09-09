/*
 * Copyright 2007, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CenteredViewContainer.h"

CenteredViewContainer::CenteredViewContainer(BView *target, BRect frame, const char* name, uint32 resizingMode)
	: BView(frame, name, resizingMode, B_WILL_DRAW | B_FRAME_EVENTS)
	, fTarget(target)
{
	SetViewColor(B_TRANSPARENT_COLOR);
		// to avoid flickering
	AddChild(fTarget);
	_CenterTarget(frame.Width(), frame.Height());
}

CenteredViewContainer::~CenteredViewContainer()
{
}

void 
CenteredViewContainer::Draw(BRect updateRect)
{
	FillRect(updateRect);
}

void 
CenteredViewContainer::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_CenterTarget(width, height);
}

void CenteredViewContainer::_CenterTarget(float width, float height)
{
	float size = width < height ? width : height;
	float left = floor((width - size) / 2);
	float top = floor((height - size) / 2);
	fTarget->MoveTo(left, top);
	fTarget->ResizeTo(size, size);
	fTarget->FrameResized(size, size);
		// in BeOS R5 BView::FrameResized is not (always) called automatically
		// after ResizeTo() 
}
