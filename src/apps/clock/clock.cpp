/*
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 */

#include "clock.h"
#include "cl_wind.h"


const char *kAppSignature = "application/x-vnd.Haiku-Clock";


THelloApplication::THelloApplication()
		  :BApplication(kAppSignature)
{
	BRect windowRect(100, 100, 182, 182);
	myWindow = new TClockWindow(windowRect, "Clock");
	myWindow->Show();
}


THelloApplication::~THelloApplication()
{
}


void
THelloApplication::MessageReceived(BMessage *msg)
{
	BApplication::MessageReceived(msg);
}


//	#pragma mark -


int
main(int argc, char* argv[])
{
	THelloApplication app;
	app.Run();

	return 0;
}

