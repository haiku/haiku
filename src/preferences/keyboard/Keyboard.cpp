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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "KeyboardApplication"

KeyboardApplication::KeyboardApplication()
	: BApplication("application/x-vnd.Haiku-Keyboard")
{
	new KeyboardWindow();
}


void
KeyboardApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ERROR_DETECTED:
		{
			BAlert* errorAlert = new BAlert("Error",
				B_TRANSLATE("Something has gone wrong!"),
				B_TRANSLATE("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
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
	(new BAlert("about", B_TRANSLATE("Written by Andrew Edward McCall"),
		B_TRANSLATE("OK")))->Go();
}


//	#pragma mark -


int
main(int, char**)
{
	KeyboardApplication	app;
	app.Run();

	return 0;
}

