/*
 * Copyright 2004-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		John Scipione, jscipione@gmail.com
 *		Sandor Vroemisse
 */


#include "KeymapApplication.h"


//	#pragma mark - KeymapApplication


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
		case kMsgUpdateModifierKeys:
			fWindow->PostMessage(message);
			break;
	}

	BApplication::MessageReceived(message);
}


void
KeymapApplication::_ShowModifierKeysWindow()
{
	if (fModifierKeysWindow != NULL)
		fModifierKeysWindow->Activate();
	else {
		fModifierKeysWindow = new ModifierKeysWindow();
		fModifierKeysWindow->CenterIn(fWindow->Frame());
		fModifierKeysWindow->Show();
	}
}


//	#pragma mark - main method


int
main(int, char**)
{
	new KeymapApplication;
	be_app->Run();
	delete be_app;
	return B_OK;
}
