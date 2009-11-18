/*
 * Copyright 2004-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Jérôme Duval
 *		Marcus Overhagen
 */


#include "Keyboard.h"
#include "KeyboardWindow.h"
#include "KeyboardMessages.h"

#include <Alert.h>

#undef TR_CONTEXT
#define TR_CONTEXT "KeyboardApplication"

KeyboardApplication::KeyboardApplication()
	: BApplication("application/x-vnd.Haiku-Keyboard")
{
	be_locale->GetAppCatalog(&fCatalog);
	new KeyboardWindow();
}


void
KeyboardApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ERROR_DETECTED:
		{
			BAlert* errorAlert = new BAlert("Error", TR("Something has gone wrong!"),
				TR("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT);
			errorAlert->Go();
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;			
		}
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
KeyboardApplication::AboutRequested()
{
	(new BAlert("about", TR("Written by Andrew Edward McCall"), TR("OK")))->Go();
}


//	#pragma mark -


int
main(int, char**)
{
	KeyboardApplication	app;
	app.Run();

	return 0;
}

