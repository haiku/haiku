/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Andrew Bachmann
 *		Sergei Panteleev
 */


#include "ScreenApplication.h"
#include "ScreenWindow.h"
#include "ScreenSettings.h"
#include "Constants.h"

#include <Alert.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screen"


static const char* kAppSignature = "application/x-vnd.Haiku-Screen";


ScreenApplication::ScreenApplication()
	:	BApplication(kAppSignature),
	fScreenWindow(new ScreenWindow(new ScreenSettings()))
{
	fScreenWindow->Show();
}


void
ScreenApplication::AboutRequested()
{
	BAlert *aboutAlert = new BAlert(B_TRANSLATE("About"),
		B_TRANSLATE("Screen preferences by the Haiku team"), B_TRANSLATE("OK"),
		NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
	aboutAlert->SetFlags(aboutAlert->Flags() | B_CLOSE_ON_ESCAPE);
	aboutAlert->Go();
}


void
ScreenApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SET_CUSTOM_REFRESH_MSG:
		case MAKE_INITIAL_MSG:
		case UPDATE_DESKTOP_COLOR_MSG:
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
