/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "ConnectionWindow.h"

#include <Application.h>
#include <String.h>


ConnectionWindow::ConnectionWindow(BRect frame, const char *name, ppp_interface_id id,
		thread_id replyThread)
	: BWindow(frame, "", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BString title("Connecting to ");
	title << "\"" << name << "\"...";
	SetTitle(title.String());
	fConnectionView = new ConnectionView(Bounds(), name, id, replyThread);
	AddChild(fConnectionView);
}


bool
ConnectionWindow::QuitRequested()
{
	fConnectionView->CleanUp();
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return true;
}


void
ConnectionWindow::UpdateStatus(BMessage& message)
{
	message.what = MSG_UPDATE;
	PostMessage(&message, fConnectionView);
}


bool
ConnectionWindow::ResponseTest()
{
	// test if we dead-locked
	Lock();
	Unlock();
	return true;
}
