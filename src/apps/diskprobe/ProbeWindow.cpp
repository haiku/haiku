/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


/*!	Abstract base class for probe windows. It only provides the following
	functionality:
		- Access to the basic entry_ref
		- Common BWindow flags
		- Stores size in settings on QuitRequested()
		- Redirects drops to BApplication
		- Notifies BApplication about closed window
		- Forwards mouse wheel to the DataView
		- Contains() checks whether or not the ref/attribute is what this
		  window contains
*/


#include "ProbeWindow.h"

#include <Application.h>
#include <View.h>

#include "DiskProbe.h"


ProbeWindow::ProbeWindow(BRect rect, entry_ref* ref)
	:
	BWindow(rect, ref->name, B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fRef(*ref)
{
}


ProbeWindow::~ProbeWindow()
{
}


void 
ProbeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
			if (BView* view = FindView("dataView"))
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

