/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */


#include "ColorWindow.h"
#include "MenuApp.h"
#include "MenuBar.h"
#include "MenuSettings.h"
#include "MenuWindow.h"
#include "msg.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <MenuItem.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>

	
MenuWindow::MenuWindow(BRect rect)
	: BWindow(rect, "Menu", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	fColorWindow = NULL;
	fRevert = false;

	BView* topView = new BView(Bounds(), "menuView", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	fMenuBar = new MenuBar();
	topView->AddChild(fMenuBar);

	// resize the window according to the size of fMenuBar
	ResizeTo(fMenuBar->Frame().right + 40, fMenuBar->Frame().bottom + 45);

	BRect menuBarFrame = fMenuBar->Frame();
	BRect buttonFrame(menuBarFrame.left, menuBarFrame.bottom + 10, 
		menuBarFrame.left + 75, menuBarFrame.bottom + 30);

	BButton* defaultButton = new BButton(buttonFrame, "Default", "Defaults",
		new BMessage(MENU_DEFAULT), B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE);
	topView->AddChild(defaultButton);

	buttonFrame.OffsetBy(buttonFrame.Width() + 20, 0);
	fRevertButton = new BButton(buttonFrame, "Revert", "Revert", new BMessage(MENU_REVERT),
		B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	fRevertButton->SetEnabled(false);
	topView->AddChild(fRevertButton);

	topView->MakeFocus();

	Update();	
}


void
MenuWindow::MessageReceived(BMessage *msg)
{
	MenuSettings *settings = MenuSettings::GetInstance();
	menu_info info;

	switch (msg->what) {
		case MENU_REVERT:
			fRevert = false;
			settings->Revert();
			Update();
			break;

		case MENU_DEFAULT:
			fRevert = true;
			settings->ResetToDefaults();
			Update();
			break;

		case UPDATE_WINDOW:
			Update();
			break;

		case MENU_FONT_FAMILY:
		case MENU_FONT_STYLE:
		{
			fRevert = true;
			const font_family *family;
			msg->FindString("family", (const char **)&family);
			const font_style *style;
			msg->FindString("style", (const char **)&style);

			settings->Get(info);
			strlcpy(info.f_family, (const char *)family, B_FONT_FAMILY_LENGTH);
			strlcpy(info.f_style, (const char *)style, B_FONT_STYLE_LENGTH);
			settings->Set(info);
			Update();
			break;
		}

		case MENU_FONT_SIZE:
			fRevert = true;
			settings->Get(info);
			msg->FindFloat("size", &info.font_size);
			settings->Set(info);
			Update();
			break;

		case ALLWAYS_TRIGGERS_MSG:
			fRevert = true;
			settings->Get(info);
			info.triggers_always_shown = !info.triggers_always_shown;
			settings->Set(info);
			fMenuBar->UpdateMenu();
			Update();
			break;

		case CTL_MARKED_MSG:
			fRevert = true;
			// This might not be the same for all keyboards
			set_modifier_key(B_LEFT_COMMAND_KEY, 0x5c);
			set_modifier_key(B_RIGHT_COMMAND_KEY, 0x60);
			set_modifier_key(B_LEFT_CONTROL_KEY, 0x5d);
			set_modifier_key(B_RIGHT_OPTION_KEY, 0x5f);
			be_roster->Broadcast(new BMessage(B_MODIFIERS_CHANGED));
			Update();
			break;

		case ALT_MARKED_MSG:
			fRevert = true;
			// This might not be the same for all keyboards
			set_modifier_key(B_LEFT_COMMAND_KEY, 0x5d);
			set_modifier_key(B_RIGHT_COMMAND_KEY, 0x5f);
			set_modifier_key(B_LEFT_CONTROL_KEY, 0x5c);
			set_modifier_key(B_RIGHT_OPTION_KEY, 0x60);

			be_roster->Broadcast(new BMessage(B_MODIFIERS_CHANGED));
			Update();
			break;

		case COLOR_SCHEME_OPEN_MSG:
			if (fColorWindow == NULL) {
				fColorWindow = new ColorWindow(this);
				fColorWindow->Show();
			} else
				fColorWindow->Activate();
			break;

		case COLOR_SCHEME_CLOSED_MSG:
			fColorWindow = NULL;
			break;

		case MENU_COLOR:
			fRevert = true;
			Update();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
MenuWindow::QuitRequested()
{
	if (fColorWindow != NULL && fColorWindow->Lock()) {
		fColorWindow->Quit();
		fColorWindow = NULL;
	}

	return true;
}


void
MenuWindow::Update()
{
	fRevertButton->SetEnabled(fRevert);

	// alert the rest of the application to update	
	fMenuBar->Update();
}

