/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WrapperView.h"

#include <LayoutUtils.h>
#include <View.h>


WrapperView::WrapperView(BView* view)
	: View(),
	  fView(view),
	  fInsets(1, 1, 1, 1)
{
	SetViewColor((rgb_color){255, 0, 0, 255});
}


BView*
WrapperView::GetView() const
{
	return fView;
}


BSize
WrapperView::MinSize()
{
	return _FromViewSize(fView->MinSize());
}


BSize
WrapperView::MaxSize()
{
	return _FromViewSize(fView->MaxSize());
}


BSize
WrapperView::PreferredSize()
{
	return _FromViewSize(fView->PreferredSize());
}


void
WrapperView::AddedToContainer()
{
	_UpdateViewFrame();

	Container()->AddChild(fView);
}


void
WrapperView::RemovingFromContainer()
{
	Container()->RemoveChild(fView);
}


void
WrapperView::FrameChanged(BRect oldFrame, BRect newFrame)
{
	_UpdateViewFrame();
}


void
WrapperView::_UpdateViewFrame()
{
	BRect frame(_ViewFrameInContainer());
	fView->MoveTo(frame.LeftTop());
	fView->ResizeTo(frame.Width(), frame.Height());
}


BRect
WrapperView::_ViewFrame() const
{
	BRect viewFrame(Bounds());
	viewFrame.left += fInsets.left;
	viewFrame.top += fInsets.top;
	viewFrame.right -= fInsets.right;
	viewFrame.bottom -= fInsets.bottom;

	return viewFrame;
}


BRect
WrapperView::_ViewFrameInContainer() const
{
	return ConvertToContainer(_ViewFrame());
}


BSize
WrapperView::_FromViewSize(BSize size) const
{
	float horizontalInsets = fInsets.left + fInsets.right - 1;
	float verticalInsets = fInsets.top + fInsets.bottom - 1;
	return BSize(BLayoutUtils::AddDistances(size.width, horizontalInsets),
		BLayoutUtils::AddDistances(size.height, verticalInsets));
}
