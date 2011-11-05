/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <GroupView.h>


BGroupView::BGroupView(enum orientation orientation, float spacing)
	:
	BView(NULL, 0, new BGroupLayout(orientation, spacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BGroupView::BGroupView(const char* name, enum orientation orientation,
	float spacing)
	:
	BView(name, 0, new BGroupLayout(orientation, spacing))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BGroupView::BGroupView(BMessage* from)
	:
	BView(from)
{
}


BGroupView::~BGroupView()
{
}


void
BGroupView::SetLayout(BLayout* layout)
{
	// only BGroupLayouts are allowed
	if (!dynamic_cast<BGroupLayout*>(layout))
		return;

	BView::SetLayout(layout);
}


BArchivable*
BGroupView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BGroupView"))
		return new BGroupView(from);
	return NULL;
}


BGroupLayout*
BGroupView::GroupLayout() const
{
	return dynamic_cast<BGroupLayout*>(GetLayout());
}


status_t
BGroupView::Perform(perform_code code, void* _data)
{
	return BView::Perform(code, _data);
}


void BGroupView::_ReservedGroupView1() {}
void BGroupView::_ReservedGroupView2() {}
void BGroupView::_ReservedGroupView3() {}
void BGroupView::_ReservedGroupView4() {}
void BGroupView::_ReservedGroupView5() {}
void BGroupView::_ReservedGroupView6() {}
void BGroupView::_ReservedGroupView7() {}
void BGroupView::_ReservedGroupView8() {}
void BGroupView::_ReservedGroupView9() {}
void BGroupView::_ReservedGroupView10() {}

