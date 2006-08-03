/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#include <Alert.h>

#include "Keyboard.h"
#include "KeyboardWindow.h"
#include "KeyboardMessages.h"


int main(int, char**)
{
	KeyboardApplication	myApplication;

	myApplication.Run();

	return(0);
}

KeyboardApplication::KeyboardApplication()
 :	BApplication("application/x-vnd.Haiku-KeyboardPrefs")
{
	new KeyboardWindow();
}

void
KeyboardApplication::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case ERROR_DETECTED: {
				BAlert *errorAlert = new BAlert("Error", 
												"Something has gone wrong!",
												"OK",NULL,NULL,
												B_WIDTH_AS_USUAL,
												B_OFFSET_SPACING,
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
KeyboardApplication::AboutRequested(void)
{
	(new BAlert("about", "Written by Andrew Edward McCall", "OK"))->Go();
}
