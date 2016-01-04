/*
 * Copyright 2010-2015 Haiku, Inc. All rights reserved.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 *
 * Distributed under the terms of the MIT License.
 */


#include <GroupView.h>


BGroupView::BGroupView(orientation orientation, float spacing)
	:
	BView(NULL, 0, new BGroupLayout(orientation, spacing))
{
	AdoptSystemColors();
}


BGroupView::BGroupView(const char* name, orientation orientation,
	float spacing)
	:
	BView(name, 0, new BGroupLayout(orientation, spacing))
{
	AdoptSystemColors();
}


BGroupView::BGroupView(BMessage* from)
	:
	BView(from)
{
	AdoptSystemColors();
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
