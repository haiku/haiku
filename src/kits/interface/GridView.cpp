/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <GridView.h>


// constructor
BGridView::BGridView(float horizontalSpacing, float verticalSpacing)
	: BView(NULL, 0, new BGridLayout(horizontalSpacing, verticalSpacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// destructor
BGridView::~BGridView()
{
}

// SetLayout
void
BGridView::SetLayout(BLayout* layout)
{
	// only BGridLayouts are allowed
	if (!dynamic_cast<BGridLayout*>(layout))
		return;

	BView::SetLayout(layout);
}

// GridLayout
BGridLayout*
BGridView::GridLayout() const
{
	return dynamic_cast<BGridLayout*>(GetLayout());
}
