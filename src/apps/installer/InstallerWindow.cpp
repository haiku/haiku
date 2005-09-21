/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Box.h>
#include "DrawButton.h"
#include "InstallerWindow.h"

const uint32 BEGIN_MESSAGE = 'iBMG';
const uint32 SHOW_BOTTOM_MESSAGE = 'iSMG';

InstallerWindow::InstallerWindow(BRect frame_rect)
	: BWindow(frame_rect, "Installer", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{

	BRect bounds = Bounds();
	bounds.bottom += 1;
	bounds.right += 1;
	fBackBox = new BBox(bounds, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	AddChild(fBackBox);

	fBeginButton = new BButton(BRect(bounds.right-90, bounds.bottom-34, bounds.right-10, bounds.bottom-10), 
		"begin_button", "Begin", new BMessage(BEGIN_MESSAGE));
	fBeginButton->MakeDefault(true);
	fBackBox->AddChild(fBeginButton);

	DrawButton *drawButton = new DrawButton(BRect(bounds.left+12, bounds.bottom-33, bounds.left+100, bounds.bottom-20),
		"options_button", "Fewer options", "More options", new BMessage(SHOW_BOTTOM_MESSAGE));
	fBackBox->AddChild(drawButton);
	
	// finish creating window
	Show();
}

InstallerWindow::~InstallerWindow()
{
}


void
InstallerWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case BEGIN_MESSAGE:
			break;
		case SHOW_BOTTOM_MESSAGE:
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
InstallerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void 
InstallerWindow::ShowBottom(bool show)
{
	if (show) {

	} else {

	}
}

