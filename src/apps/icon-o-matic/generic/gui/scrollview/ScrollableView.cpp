/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "ScrollableView.h"

#include <View.h>

// constructor
ScrollableView::ScrollableView()
	: Scrollable()
{
}

// destructor
ScrollableView::~ScrollableView()
{
}

// ScrollOffsetChanged
void
ScrollableView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	if (BView* view = dynamic_cast<BView*>(this)) {
		// We keep it simple: The part of the data rect we shall show now
		// has existed before as well (even if partially or completely
		// obscured), so we let CopyBits() do the messy details.
		BRect bounds(view->Bounds());
		view->CopyBits(bounds.OffsetByCopy(newOffset - oldOffset), bounds);
		// move our children
		for (int32 i = 0; BView* child = view->ChildAt(i); i++)
			child->MoveTo(child->Frame().LeftTop() + oldOffset - newOffset);
	}
}

