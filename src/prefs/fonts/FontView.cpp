/*! \file FontView.cpp
    \brief Header for the FontView class.
    
*/

#ifndef FONT_VIEW_H

	#include "FontView.h"

#endif

/**
 * Constructor.
 * @param rect The size of the view.
 */
FontView::FontView(BRect rect)
	   	   : BView(rect, "FontView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	
	float x;
	float y;
	BRect viewSize = Bounds();
	
	SetViewColor(216, 216, 216, 0);
	
	x = viewSize.Width() / 39;
	y = viewSize.Height() / 25;
	
	plainSelectionView = new FontSelectionView(*(new BRect(x, y, (38.0 * x), (8.0 * y))), "Plain", PLAIN_FONT_SELECTION_VIEW);
	boldSelectionView = new FontSelectionView(*(new BRect(x, (9.0 * y), (38.0 * x), (16.0 * y))), "Bold", BOLD_FONT_SELECTION_VIEW);
	fixedSelectionView = new FontSelectionView(*(new BRect(x, (17.0 * y), (38.0 * x), (24.0 * y))), "Fixed", FIXED_FONT_SELECTION_VIEW);

	AddChild(plainSelectionView);
	AddChild(boldSelectionView);
	AddChild(fixedSelectionView);
	
}

/**
 * Calls each FontSelectionView's buildMenus() function to build the
 * font and size menus.
 */
void FontView::buildMenus(){

	plainSelectionView->buildMenus();
	boldSelectionView->buildMenus();
	fixedSelectionView->buildMenus();

}//buildMenus

/**
 * Calls each FontSelectionView's emptyMenus() function to clear the
 * font and size menus.
 */
void FontView::emptyMenus(){

	plainSelectionView->emptyMenus();
	boldSelectionView->emptyMenus();
	fixedSelectionView->emptyMenus();

}//buildMenus

/**
 * Calls each FontSelectionView's resetToDefaults() function to reset
 * the font and size menus to the default font.
 */
void FontView::resetToDefaults(){

	plainSelectionView->resetToDefaults();
	boldSelectionView->resetToDefaults();
	fixedSelectionView->resetToDefaults();

}//resetToDefaults

/**
 * Calls each FontSelectionView's revertToOriginal() function to reset
 * the font and size menus to the original font.
 */
void FontView::revertToOriginal(){

	plainSelectionView->revertToOriginal();
	boldSelectionView->revertToOriginal();
	fixedSelectionView->revertToOriginal();

}//resetToDefaults

