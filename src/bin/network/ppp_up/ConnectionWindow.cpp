/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "ConnectionWindow.h"

#include <Application.h>
#include <String.h>


ConnectionWindow::ConnectionWindow(BRect frame, const BString& interfaceName)
	: BWindow(frame, "", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
		B_ALL_WORKSPACES)
{
	BString title("Connecting to ");
	title << "\"" << interfaceName << "\"...";
	SetTitle(title.String());
	fConnectionView = new ConnectionView(Bounds(), interfaceName);
	AddChild(fConnectionView);
}


bool
ConnectionWindow::QuitRequested()
{
	fConnectionView->CleanUp();
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return true;
}
