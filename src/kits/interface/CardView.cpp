/*
 * Copyright 2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <CardLayout.h>
#include <CardView.h>


BCardView::BCardView()
	:
	BView(NULL, 0, new BCardLayout())
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BCardView::BCardView(const char* name)
	:
	BView(name, 0, new BCardLayout())
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BCardView::BCardView(BMessage* from)
	:
	BView(from)
{
}


BCardView::~BCardView()
{
}


void
BCardView::SetLayout(BLayout* layout)
{
	if (dynamic_cast<BCardLayout*>(layout) == NULL)
		return;

	BView::SetLayout(layout);
}


BCardLayout*
BCardView::CardLayout() const
{
	return static_cast<BCardLayout*>(GetLayout());
}


BArchivable*
BCardView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BCardView"))
		return new BCardView(from);
	return NULL;
}


status_t
BCardView::Perform(perform_code d, void* arg)
{
	return BView::Perform(d, arg);
}


void BCardView::_ReservedCardView1() {}
void BCardView::_ReservedCardView2() {}
void BCardView::_ReservedCardView3() {}
void BCardView::_ReservedCardView4() {}
void BCardView::_ReservedCardView5() {}
void BCardView::_ReservedCardView6() {}
void BCardView::_ReservedCardView7() {}
void BCardView::_ReservedCardView8() {}
void BCardView::_ReservedCardView9() {}
void BCardView::_ReservedCardView10() {}
