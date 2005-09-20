/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Box.h>
#include "InstallerWindow.h"

InstallerWindow::InstallerWindow(BRect frame_rect)
	: BWindow(frame_rect, "Installer", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	
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

