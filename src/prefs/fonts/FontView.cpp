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
	plainSelectionView = new FontSelectionView(rect, "Plain", 
												PLAIN_FONT_SELECTION_VIEW);
	AddChild(plainSelectionView);
	
	rect.OffsetBy(0, rect.Height() + 4);
	boldSelectionView = new FontSelectionView(rect, "Bold", 
												BOLD_FONT_SELECTION_VIEW);
	AddChild(boldSelectionView);
	
	rect.OffsetBy(0, rect.Height() + 4);
	fixedSelectionView = new FontSelectionView(rect, "Fixed", 
												FIXED_FONT_SELECTION_VIEW);
	AddChild(fixedSelectionView);
}


void
FontView::AttachedToWindow(void)
{
}

/**
 * Calls each FontSelectionView's resetToDefaults() function to reset
 * the font and size menus to the default font.
 */
void
FontView::resetToDefaults()
{
	plainSelectionView->resetToDefaults();
	boldSelectionView->resetToDefaults();
	fixedSelectionView->resetToDefaults();
}

/**
 * Calls each FontSelectionView's revertToOriginal() function to reset
 * the font and size menus to the original font.
 */
void
FontView::revertToOriginal()
{
	plainSelectionView->revertToOriginal();
	boldSelectionView->revertToOriginal();
	fixedSelectionView->revertToOriginal();
}

