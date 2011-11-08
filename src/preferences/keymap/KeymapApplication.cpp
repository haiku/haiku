/*
 * Copyright 2004-2006 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 */


#include "KeymapApplication.h"


KeymapApplication::KeymapApplication()
	:
	BApplication("application/x-vnd.Haiku-Keymap"),
	fModifierKeysWindow(NULL)
{
	// create the window
	fWindow = new KeymapWindow();
	fWindow->Show();
}


void
KeymapApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgShowModifierKeysWindow:
			_ShowModifierKeysWindow();
			break;
		case kMsgCloseModifierKeysWindow:
			fModifierKeysWindow = NULL;
			break;
		case kMsgUpdateModifiers:
			fWindow->PostMessage(message);
			break;
	}

	BApplication::MessageReceived(message);
}


void
KeymapApplication::_ShowModifierKeysWindow()
{
	if (fModifierKeysWindow)
		fModifierKeysWindow->Activate();
	else {
		fModifierKeysWindow = new ModifierKeysWindow();
		fModifierKeysWindow->Show();
	}
}


//	#pragma mark -


int
main(int, char**)
{
	new KeymapApplication;
	be_app->Run();
	delete be_app;
	return B_OK;
}
