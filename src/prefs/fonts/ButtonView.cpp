/*! \file ButtonView.cpp
 *  \brief Code for the ButtonView.
 */
 
#ifndef BUTTON_VIEW_H

	#include "ButtonView.h"

#endif

/**
 * Constructor
 * @param rect The size of the view.
 */
ButtonView::ButtonView(BRect rect)
	:BView(rect, "ButtonView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(216,216,216,0);
	
	BRect btnRect(0, 0, 75, 25);
	btnRect.OffsetBy(10, 8);
	BButton *rescanButton = new BButton(btnRect,
				"rescanButton", "Rescan", 
				new BMessage(RESCAN_FONTS_MSG),
				B_FOLLOW_LEFT, B_WILL_DRAW);
	
	btnRect.OffsetBy(96, 0);
	BButton *defaultsButton = new BButton(btnRect,
				"defaultsButton", "Defaults",
				new BMessage(RESET_FONTS_MSG),
				B_FOLLOW_LEFT, B_WILL_DRAW);
				
	btnRect.OffsetBy(85, 0);
	revertButton = new BButton(btnRect,
				"revertButton", "Revert",
				new BMessage(REVERT_MSG),
				B_FOLLOW_LEFT, B_WILL_DRAW);
							  
	revertButton->SetEnabled(false);
	
	AddChild(rescanButton);
	AddChild(defaultsButton);
	AddChild(revertButton);
	
}


void
ButtonView::Draw(BRect update)
{
	rgb_color dark = {100, 100, 100, 255};
	rgb_color light = {255, 255, 255, 255};
	BRect bounds(Bounds());
	BeginLineArray(6);
		AddLine(bounds.LeftTop(), bounds.RightTop(), light);
		AddLine(bounds.LeftTop(), bounds.LeftBottom(), light);
		AddLine(bounds.RightTop(), bounds.RightBottom(), dark);
		AddLine(bounds.RightBottom(), bounds.LeftBottom(), dark);
		AddLine(BPoint(95, 9), BPoint(95, 34), dark);
		AddLine(BPoint(96, 9), BPoint(96, 34), light);
	EndLineArray();
}


/**
 * Returns the state of the revert button.
 */
bool ButtonView::RevertState(){

	return revertButton->IsEnabled();
	
}//RevertState

/**
 * Sets the state of the revert button.
 */
void ButtonView::SetRevertState(bool b){

	revertButton->SetEnabled(b);
	
}//SetRevertState
