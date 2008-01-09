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

#include <stdio.h>
#include <string.h>

	
MenuWindow::MenuWindow(BRect rect)
	: BWindow(rect, "Menu", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	fColorWindow = NULL;
	
	fSettings = MenuSettings::GetInstance();

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

	fDefaultsButton = new BButton(buttonFrame, "Default", "Defaults",
		new BMessage(MENU_DEFAULT), B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE);
	fDefaultsButton->SetEnabled(false);
	topView->AddChild(fDefaultsButton);

	buttonFrame.OffsetBy(buttonFrame.Width() + 20, 0);
	fRevertButton = new BButton(buttonFrame, "Revert", "Revert", new BMessage(MENU_REVERT),
		B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	fRevertButton->SetEnabled(false);
	topView->AddChild(fRevertButton);

	topView->MakeFocus();
	
	_UpdateAll();
}


void
MenuWindow::MessageReceived(BMessage *msg)
{
	menu_info info;

	switch (msg->what) {
		case MENU_REVERT:
			fSettings->Revert();
			_UpdateAll();
			break;

		case MENU_DEFAULT:
			fSettings->ResetToDefaults();
			_UpdateAll();
			break;

		case UPDATE_WINDOW:
			_UpdateAll();
			break;

		case MENU_FONT_FAMILY:
		case MENU_FONT_STYLE:
		{
			const font_family *family;
			msg->FindString("family", (const char **)&family);
			const font_style *style;
			msg->FindString("style", (const char **)&style);

			fSettings->Get(info);
			strlcpy(info.f_family, (const char *)family, B_FONT_FAMILY_LENGTH);
			strlcpy(info.f_style, (const char *)style, B_FONT_STYLE_LENGTH);
			fSettings->Set(info);

			_UpdateAll();
			break;
		}

		case MENU_FONT_SIZE:
			fSettings->Get(info);
			msg->FindFloat("size", &info.font_size);
			fSettings->Set(info);

			_UpdateAll();
			break;

		case ALLWAYS_TRIGGERS_MSG:
			fSettings->Get(info);
			info.triggers_always_shown = !info.triggers_always_shown;
			fSettings->Set(info);

			_UpdateAll();
			break;

		case CTL_MARKED_MSG:			
			fSettings->SetAltAsShortcut(false);
			_UpdateAll();
			break;

		case ALT_MARKED_MSG:
			fSettings->SetAltAsShortcut(true);			
			_UpdateAll();
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
			_UpdateAll();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
MenuWindow::_UpdateAll()
{
	fRevertButton->SetEnabled(fSettings->IsRevertable());
	fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
	fMenuBar->Update();
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

