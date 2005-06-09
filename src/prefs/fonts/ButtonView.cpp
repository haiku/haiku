/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include "ButtonView.h"
#include "MainWindow.h"

ButtonView::ButtonView(BRect rect)
	: BView(rect, "ButtonView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect btnRect(0, 0, 75, 25);
	btnRect.OffsetBy(10, 8);
	BButton *rescanButton = new BButton(btnRect, "rescanButton", "Rescan", 
										new BMessage(M_RESCAN_FONTS),
										B_FOLLOW_LEFT, B_WILL_DRAW);
	AddChild(rescanButton);
	
	btnRect.OffsetBy(96, 0);
	BButton *defaultsButton = new BButton(btnRect, "defaultsButton",
											"Defaults",
											new BMessage(M_SET_DEFAULTS),
											B_FOLLOW_LEFT, B_WILL_DRAW);
	AddChild(defaultsButton);
				
	btnRect.OffsetBy(85, 0);
	fRevertButton = new BButton(btnRect, "fRevertButton", "Revert",
								new BMessage(M_REVERT),
								B_FOLLOW_LEFT, B_WILL_DRAW);
	AddChild(fRevertButton);
	
	fRevertButton->SetEnabled(false);
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


bool
ButtonView::RevertState(void) const
{
	return fRevertButton->IsEnabled();
}

void ButtonView::SetRevertState(bool value)
{
	fRevertButton->SetEnabled(value);
}
