/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "PPPStatusWindow.h"


PPPStatusWindow::PPPStatusWindow(BRect frame, ppp_interface_id id)
	: BWindow(frame, "", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
		B_ALL_WORKSPACES)
{
	SetPulseRate(1000000);
	
	fStatusView = new PPPStatusView(Bounds(), id);
	AddChild(fStatusView);
}


bool
PPPStatusWindow::QuitRequested()
{
	// only the replicant may delete this window!
	Hide();
	return false;
}
