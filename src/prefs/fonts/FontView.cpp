/*! \file FontView.cpp
    \brief Header for the FontView class.
    
*/

#ifndef FONT_VIEW_H

	#include "FontView.h"

#endif

#include "Pref_Utils.h"

/**
 * Constructor.
 * @param rect The size of the view.
 */
FontView::FontView(BRect rect)
	:BView(rect, "FontView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect bounds(Bounds().InsetByCopy(5, 5));

	BRect rect(bounds);
	rect.bottom = rect.top +FontHeight(true) *3.5;
	plainSelectionView = new FontSelectionView(rect, "Plain", PLAIN_FONT_SELECTION_VIEW);
	rect.OffsetBy(0, rect.Height()+4);
	boldSelectionView = new FontSelectionView(rect, "Bold", BOLD_FONT_SELECTION_VIEW);
	rect.OffsetBy(0, rect.Height()+4);
	fixedSelectionView = new FontSelectionView(rect, "Fixed", FIXED_FONT_SELECTION_VIEW);

	AddChild(plainSelectionView);
	AddChild(boldSelectionView);
	AddChild(fixedSelectionView);
	
}


void
FontView::AttachedToWindow(void)
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(219, 219, 219, 255);
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

