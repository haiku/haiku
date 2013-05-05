/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <GridView.h>


BGridView::BGridView(float horizontalSpacing, float verticalSpacing)
	:
	BView(NULL, 0, new BGridLayout(horizontalSpacing, verticalSpacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BGridView::BGridView(const char* name, float horizontalSpacing,
	float verticalSpacing)
	:
	BView(name, 0, new BGridLayout(horizontalSpacing, verticalSpacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BGridView::BGridView(BMessage* from)
	:
	BView(from)
{
}


BGridView::~BGridView()
{
}


void
BGridView::SetLayout(BLayout* layout)
{
	// only BGridLayouts are allowed
	if (!dynamic_cast<BGridLayout*>(layout))
		return;

	BView::SetLayout(layout);
}


BGridLayout*
BGridView::GridLayout() const
{
	return dynamic_cast<BGridLayout*>(GetLayout());
}


BArchivable*
BGridView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BGridView"))
		return new BGridView(from);
	return NULL;
}


status_t
BGridView::Perform(perform_code code, void* _data)
{
	return BView::Perform(code, _data);
}


void BGridView::_ReservedGridView1() {}
void BGridView::_ReservedGridView2() {}
void BGridView::_ReservedGridView3() {}
void BGridView::_ReservedGridView4() {}
void BGridView::_ReservedGridView5() {}
void BGridView::_ReservedGridView6() {}
void BGridView::_ReservedGridView7() {}
void BGridView::_ReservedGridView8() {}
void BGridView::_ReservedGridView9() {}
void BGridView::_ReservedGridView10() {}

