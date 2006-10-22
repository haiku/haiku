/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg
 */


#include "Time.h"
#include "TimeSettings.h"
#include "TimeMessages.h"

#include <Alert.h>


TimeApplication::TimeApplication()
	: BApplication(HAIKU_APP_SIGNATURE)
{
	fSettings = new TimeSettings();
	fWindow = new TTimeWindow();
}


TimeApplication::~TimeApplication()
{
	delete fSettings;
}


void
TimeApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case ERROR_DETECTED:
			(new BAlert("Error", "Something has gone wrong!", "OK",
				NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT))->Go();
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;			

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
TimeApplication::ReadyToRun()
{
	fWindow->Show();
}


void
TimeApplication::AboutRequested()
{
	(new BAlert("about", "...by Andrew Edward McCall\n...Mike Berg too", "Big Deal"))->Go();
}


void
TimeApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}


//	#pragma mark -


int
main(int, char**)
{
	TimeApplication app;
	app.Run();

	return 0;
}

