/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ProbeWindow.h"
#include "DiskProbe.h"

#include <Application.h>
#include <View.h>


ProbeWindow::ProbeWindow(BRect rect, entry_ref *ref)
	: BWindow(rect, ref->name, B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fRef(*ref)
{
}


ProbeWindow::~ProbeWindow()
{
}


void 
ProbeWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
			if (BView *view = FindView("dataView"))
				view->MessageReceived(message);
			break;

		case B_SIMPLE_DATA:
		{
			BMessage refsReceived(*message);
			refsReceived.what = B_REFS_RECEIVED;
			be_app_messenger.SendMessage(&refsReceived);
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool 
ProbeWindow::QuitRequested()
{
	BMessage update(kMsgSettingsChanged);
	update.AddRect("window_frame", Frame());
	be_app_messenger.SendMessage(&update);

	be_app_messenger.SendMessage(kMsgWindowClosed);
	return true;
}

