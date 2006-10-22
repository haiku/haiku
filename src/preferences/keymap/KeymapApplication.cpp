/*
 * Copyright 2004-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Sandor Vroemisse
 *		Jérôme Duval
 */


#include "KeymapApplication.h"


KeymapApplication::KeymapApplication()
	: BApplication("application/x-vnd.Haiku-Keymap")
{
	// create the window
	fWindow = new KeymapWindow();
	fWindow->Show();
}


void
KeymapApplication::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
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
