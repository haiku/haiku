/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include "FontView.h"
#include "Pref_Utils.h"

FontView::FontView(BRect rect)
	: BView(rect, "Fonts", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect bounds( Bounds().InsetByCopy(5, 5) );
	BRect rect(bounds);
	
	rect.bottom = rect.top + FontHeight(true) *3.5;
	fPlainView = new FontSelectionView(rect, "Plain", 
												PLAIN_FONT_SELECTION_VIEW);
	AddChild(fPlainView);
	
	rect.OffsetBy(0, rect.Height() + 4);
	fBoldView = new FontSelectionView(rect, "Bold", 
												BOLD_FONT_SELECTION_VIEW);
	AddChild(fBoldView);
	
	rect.OffsetBy(0, rect.Height() + 4);
	fFixedView = new FontSelectionView(rect, "Fixed", 
												FIXED_FONT_SELECTION_VIEW);
	AddChild(fFixedView);
}


void
FontView::SetDefaults(void)
{
	fPlainView->SetDefaults();
	fBoldView->SetDefaults();
	fFixedView->SetDefaults();
}

void
FontView::Revert(void)
{
	fPlainView->Revert();
	fBoldView->Revert();
	fFixedView->Revert();
}

void
FontView::RescanFonts(void)
{
	fPlainView->RescanFonts();
	fBoldView->RescanFonts();
	fFixedView->RescanFonts();
}

