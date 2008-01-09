/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */


#include "ColorWindow.h"
#include "MenuSettings.h"
#include "msg.h"

#include <Application.h>
#include <Button.h>
#include <ColorControl.h>
#include <Menu.h>	


ColorWindow::ColorWindow(BMessenger owner)
	: BWindow(BRect(150, 150, 350, 200), "Menu Color Scheme", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	fOwner(owner)
{
	fCurrentColor = MenuSettings::GetInstance()->BackgroundColor();
	fPreviousColor = MenuSettings::GetInstance()->PreviousBackgroundColor();
	fDefaultColor = MenuSettings::GetInstance()->DefaultBackgroundColor();

	BView *topView = new BView(Bounds(), "topView", B_FOLLOW_ALL_SIDES, 0);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	fColorControl = new BColorControl(BPoint(10, 10), B_CELLS_32x8, 
		9, "COLOR", new BMessage(MENU_COLOR), true);
	fColorControl->SetValue(fCurrentColor);
	fColorControl->ResizeToPreferred();
	topView->AddChild(fColorControl);

	// Create the buttons and add them to the view
	BRect rect = fColorControl->Frame();
	rect.top = rect.bottom + 8;
	rect.bottom = rect.top + 10;
	fDefaultButton = new BButton(rect, "Default", "Default",
		new BMessage(MENU_COLOR_DEFAULT), B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_NAVIGABLE);
	fDefaultButton->ResizeToPreferred();
	fDefaultButton->SetEnabled(fCurrentColor != fDefaultColor);
	topView->AddChild(fDefaultButton);

	rect.OffsetBy(fDefaultButton->Bounds().Width() + 10, 0);
	fRevertButton = new BButton(rect, "Revert", "Revert",
		new BMessage(MENU_REVERT), B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_NAVIGABLE);
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetEnabled(fCurrentColor != fPreviousColor);
	topView->AddChild(fRevertButton);

	ResizeTo(fColorControl->Bounds().Width() + 20, fRevertButton->Frame().bottom + 10);
}


void
ColorWindow::Quit()
{
	fOwner.SendMessage(COLOR_SCHEME_CLOSED_MSG);
	BWindow::Quit();
}


void
ColorWindow::_UpdateAndPost()
{
	fDefaultButton->SetEnabled(fCurrentColor != fDefaultColor);
	fRevertButton->SetEnabled(fCurrentColor != fPreviousColor);
	
	fCurrentColor = fColorControl->ValueAsColor();
		
	menu_info info;
	get_menu_info(&info);
	info.background_color = fCurrentColor;
	set_menu_info(&info);
	
	be_app->PostMessage(MENU_COLOR);	
}


void
ColorWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MENU_REVERT:
			fColorControl->SetValue(fPreviousColor);
			_UpdateAndPost();
			break;

		case MENU_COLOR_DEFAULT:			
			fColorControl->SetValue(fDefaultColor);
			_UpdateAndPost();
			break;

		case MENU_COLOR:
			_UpdateAndPost();			
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}	
