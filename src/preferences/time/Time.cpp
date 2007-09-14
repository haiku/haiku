/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@digitalparadise.co.uk>
 *		Mike Berg <mike@agamemnon.homelinux.net>
 *		Julun <host.haiku@gmx.de>
 */

#include "Time.h"
#include "TimeMessages.h"
#include "TimeSettings.h"
#include "TimeWindow.h"


#include <Alert.h>
#include <Message.h>


TimeApplication::TimeApplication()
	: BApplication(HAIKU_APP_SIGNATURE),
	  fWindow(NULL)
{
	BPoint pt = TimeSettings().LeftTop();
	fWindow = new TTimeWindow(pt);
}


TimeApplication::~TimeApplication()
{
}


void
TimeApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case UPDATE_SETTINGS:
		{
			BPoint pt;
			if (message->FindPoint("LeftTop", &pt) == B_OK)
				TimeSettings().SetLeftTop(pt);
		}	break;			

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
	BAlert alert("about", "Time & Date, by\n\nAndrew Edward McCall\nMike Berg", "OK");
	alert.Go();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	TimeApplication app;
	app.Run();

	return 0;
}

