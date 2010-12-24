/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */

#include "Time.h"
#include "TimeWindow.h"


#include <Alert.h>
#include <Catalog.h>

#include <unistd.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Time"

const char* kAppSignature = "application/x-vnd.Haiku-Time";


TimeApplication::TimeApplication()
	: BApplication(kAppSignature),
	  fWindow(NULL)
{
	fWindow = new TTimeWindow(BRect(100, 100, 570, 327));
}


TimeApplication::~TimeApplication()
{
}


void
TimeApplication::ReadyToRun()
{
	fWindow->Show();
}


void
TimeApplication::AboutRequested()
{
	BAlert* alert = new BAlert(B_TRANSLATE("about"),
		B_TRANSLATE("Time & Date, writen by:\n\n\tAndrew Edward McCall\n\tMike Berg\n\t"
					"Julun\n\tPhilippe Saint-Pierre\n\nCopyright 2004-2008, Haiku."),
		B_TRANSLATE("OK"));
	alert->Go();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	TimeApplication app;
	setuid(0);
	app.Run();

	return 0;
}

