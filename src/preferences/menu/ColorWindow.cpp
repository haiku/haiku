/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */


#include "ColorWindow.h"
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
	// Set and collect the variables for revert
	get_menu_info(&fInfo);
	get_menu_info(&fRevertInfo);

	BView *topView = new BView(Bounds(), "topView", B_FOLLOW_ALL_SIDES, 0);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	fColorControl = new BColorControl(BPoint(10, 10), B_CELLS_32x8, 
		9, "COLOR", new BMessage(MENU_COLOR), true);
	fColorControl->SetValue(fInfo.background_color);
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
	fDefaultButton->SetEnabled(false);
	topView->AddChild(fDefaultButton);

	rect.OffsetBy(fDefaultButton->Bounds().Width() + 10, 0);
	fRevertButton = new BButton(rect, "Revert", "Revert",
		new BMessage(MENU_REVERT), B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_NAVIGABLE);
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetEnabled(false);
	topView->AddChild(fRevertButton);

	ResizeTo(fColorControl->Bounds().Width() + 20, fRevertButton->Frame().bottom + 10);
}


ColorWindow::~ColorWindow()
{
}


void
ColorWindow::Quit()
{
	fOwner.SendMessage(COLOR_SCHEME_CLOSED_MSG);
	BWindow::Quit();
}


void
ColorWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MENU_REVERT:
			fColorControl->SetValue(fRevertInfo.background_color);
			fInfo.background_color = fColorControl->ValueAsColor();
			set_menu_info(&fInfo);
			be_app->PostMessage(UPDATE_WINDOW);
			fRevertButton->SetEnabled(false);
			break;

		case MENU_COLOR_DEFAULT:
			// change to system color for system wide
			// compatability
			rgb_color color;
			color.red = 216;
			color.blue = 216;
			color.green = 216;
			color.alpha = 255;
			fColorControl->SetValue(color);
			fDefaultButton->SetEnabled(false);
			get_menu_info(&fInfo);
			fInfo.background_color = fColorControl->ValueAsColor();
			set_menu_info(&fInfo);
			be_app->PostMessage(MENU_COLOR);
			break;

		case MENU_COLOR:
			get_menu_info(&fInfo);
			fRevertInfo.background_color = fInfo.background_color;
			fInfo.background_color = fColorControl->ValueAsColor();
			set_menu_info(&fInfo);
			be_app->PostMessage(MENU_COLOR);
			fDefaultButton->SetEnabled(true);
			fRevertButton->SetEnabled(true);
			break;

		default:
			be_app->PostMessage(UPDATE_WINDOW);
			BWindow::MessageReceived(msg);
			break;
	}
}	
