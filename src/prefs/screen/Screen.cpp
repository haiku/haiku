/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Andrew Bachmann
 *		Sergei Panteleev
 */


#include <Application.h>
#include <Alert.h>

#include <cstdio>
#include <cstring>

#include "Constants.h"
#include "ScreenApplication.h"
#include "ScreenWindow.h"
#include "ScreenSettings.h"


ScreenApplication::ScreenApplication()
	:	BApplication(kAppSignature),
		fScreenWindow(new ScreenWindow(new ScreenSettings()))
{
	fScreenWindow->Show();
}


void
ScreenApplication::AboutRequested()
{
	BAlert *aboutAlert = new BAlert("About", "Screen preferences by the Haiku team",
		"Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
	aboutAlert->SetShortcut(0, B_OK);
	aboutAlert->Go();
}


void
ScreenApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SET_INITIAL_MODE_MSG:
		case SET_CUSTOM_REFRESH_MSG:
		case MAKE_INITIAL_MSG:
			fScreenWindow->PostMessage(message);	
			break;

		default:
			BApplication::MessageReceived(message);			
			break;
	}
}


//	#pragma mark -


int
main()
{
	ScreenApplication app;
	app.Run();

	return 0;
}
