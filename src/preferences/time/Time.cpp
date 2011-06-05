/*
 * Copyright 2002-2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */


#include "Time.h"

#include <stdio.h>
#include <unistd.h>

#include <Alert.h>
#include <Catalog.h>

#include "NetworkTimeView.h"
#include "TimeWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Time"


const char* kAppSignature = "application/x-vnd.Haiku-Time";


TimeApplication::TimeApplication()
	: 
	BApplication(kAppSignature),
	fWindow(NULL)
{
	fWindow = new TTimeWindow();
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
		B_TRANSLATE(
		"Time & Date, writen by:\n\n\tAndrew Edward McCall\n\tMike Berg\n\t"
		"Julun\n\tPhilippe Saint-Pierre\n\nCopyright 2004-2008, Haiku."),
		B_TRANSLATE("OK"));
	alert->Go();
}


int
main(int argc, char** argv)
{
	if (argc > 1) {
		if (strcmp(argv[1], "--update") != 0)
			return 0;
		
		Settings settings;
		if (!settings.GetSynchronizeAtBoot())
			return 0;

		const char* errorString = NULL;
		int32 errorCode = 0;
		if (update_time(settings, &errorString, &errorCode) == B_OK)
			printf("Synchronization successful\r\n");
		else if (errorCode != 0)
			printf("The following error occured "
				"while synchronizing:\r\n%s: %s\r\n",
				errorString, strerror(errorCode));
		else
			printf("The following error occured "
				"while synchronizing:\r\n%s\r\n",
				errorString);
	}
	else {
		TimeApplication app;
		setuid(0);
		app.Run();
	}

	return 0;
}

