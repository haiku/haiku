/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <GroupView.h>


// constructor
BGroupView::BGroupView(enum orientation orientation, float spacing)
	: BView(NULL, 0, new BGroupLayout(orientation, spacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// destructor
BGroupView::~BGroupView()
{
}

// SetLayout
void
BGroupView::SetLayout(BLayout* layout)
{
	// only BGroupLayouts are allowed
	if (!dynamic_cast<BGroupLayout*>(layout))
		return;

	BView::SetLayout(layout);
}

// GroupLayout
BGroupLayout*
BGroupView::GroupLayout() const
{
	return dynamic_cast<BGroupLayout*>(GetLayout());
}
