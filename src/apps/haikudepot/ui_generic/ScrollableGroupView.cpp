/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ScrollableGroupView.h"

#include <ScrollBar.h>
#include <SpaceLayoutItem.h>


ScrollableGroupView::ScrollableGroupView()
	:
	BGroupView(B_VERTICAL, 0.0)
{
	AddChild(BSpaceLayoutItem::CreateGlue());
}


BSize
ScrollableGroupView::MinSize()
{
	BSize minSize = BGroupView::MinSize();
	return BSize(minSize.width, 80);
}


void
ScrollableGroupView::DoLayout()
{
	BGroupView::DoLayout();

	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);

	if (scrollBar == NULL)
		return;

	BRect layoutArea = GroupLayout()->LayoutArea();
	float layoutHeight = layoutArea.Height();
	// Min size is not reliable with HasHeightForWidth() children,
	// since it does not reflect how those children are currently
	// laid out, but what their theoretical minimum size would be.

	BLayoutItem* lastItem = GroupLayout()->ItemAt(
		GroupLayout()->CountItems() - 1);
	if (lastItem != NULL)
		layoutHeight = lastItem->Frame().bottom;

	float viewHeight = Bounds().Height();

	float max = layoutHeight- viewHeight;
	scrollBar->SetRange(0, max);
	if (layoutHeight > 0)
		scrollBar->SetProportion(viewHeight / layoutHeight);
	else
		scrollBar->SetProportion(1);
}
