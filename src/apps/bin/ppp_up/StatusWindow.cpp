/*
 * Copyright 2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "StatusWindow.h"
#include "StatusView.h"


StatusWindow::StatusWindow(BRect frame, ppp_interface_id id)
	: BWindow(frame, "", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
		B_ALL_WORKSPACES)
{
	SetPulseRate(1000000);
	
	StatusView *view = new StatusView(Bounds(), id);
	AddChild(view);
}


bool
StatusWindow::QuitRequested()
{
	// only the replicant may delete this window!
	Hide();
	return false;
}
