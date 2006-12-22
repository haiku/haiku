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

	BView *colView = new BView(BRect(0,0,1000,100), "menuView",
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	
	fColorControl = new BColorControl(BPoint(10,10), B_CELLS_32x8, 
		9, "COLOR", new BMessage(MENU_COLOR), true);
	fColorControl->SetValue(fInfo.background_color);
	colView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	colView->AddChild(fColorControl);
	AddChild(colView);

	ResizeTo(383, 130);

	// Create the buttons and add them to the view
	fDefaultButton = new BButton(BRect(10,100,85,110), "Default", "Default",
		new BMessage(MENU_COLOR_DEFAULT), B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_NAVIGABLE);
	fRevertButton = new BButton(BRect(95,100,175,20), "REVERT", "Revert",
		new BMessage(MENU_REVERT), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, 
		B_WILL_DRAW | B_NAVIGABLE);

	colView->AddChild(fDefaultButton);
	colView->AddChild(fRevertButton);

	fDefaultButton->SetEnabled(false);
	fRevertButton->SetEnabled(false);
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
