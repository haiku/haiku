/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "FontView.h"


FontView::FontView(BRect _rect)
	: BView(_rect, "Fonts", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect rect(Bounds());

	float labelWidth = StringWidth("Fixed Font:") + 8;

	fPlainView = new FontSelectionView(rect, "plain", "Plain Font:", *be_plain_font);
	fPlainView->SetDivider(labelWidth);
	fPlainView->ResizeToPreferred();
	AddChild(fPlainView);

	rect.OffsetBy(0, fPlainView->Bounds().Height() + 10);
	fBoldView = new FontSelectionView(rect, "bold", "Bold Font:", *be_bold_font);
	fBoldView->SetDivider(labelWidth);
	fBoldView->ResizeToPreferred();
	AddChild(fBoldView);

	rect.OffsetBy(0, fPlainView->Bounds().Height() + 10);
	fFixedView = new FontSelectionView(rect, "fixed", "Fixed Font:", *be_fixed_font);
	fFixedView->SetDivider(labelWidth);
	fFixedView->ResizeToPreferred();
	AddChild(fFixedView);
}


void
FontView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = fPlainView->Bounds().Width();

	if (_height)
		*_height = fPlainView->Bounds().Height() * 3 + 20;
}


void
FontView::SetDefaults()
{
	for (int32 i = 0; i < CountChildren(); i++) {
		FontSelectionView* view = dynamic_cast<FontSelectionView *>(ChildAt(i));
		if (view == NULL)
			continue;

		view->SetDefaults();
	}
}


void
FontView::Revert()
{
	for (int32 i = 0; i < CountChildren(); i++) {
		FontSelectionView* view = dynamic_cast<FontSelectionView *>(ChildAt(i));
		if (view == NULL)
			continue;

		view->Revert();
	}
}


void
FontView::UpdateFonts()
{
	fPlainView->UpdateFontsMenu();
	fBoldView->UpdateFontsMenu();
	fFixedView->UpdateFontsMenu();
}


void
FontView::RelayoutIfNeeded()
{
	fPlainView->RelayoutIfNeeded();
	fBoldView->RelayoutIfNeeded();
	fFixedView->RelayoutIfNeeded();
}


bool
FontView::IsRevertable()
{
	return fPlainView->IsRevertable()
		|| fBoldView->IsRevertable()
		|| fFixedView->IsRevertable();
}

