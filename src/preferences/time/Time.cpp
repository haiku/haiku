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

#include <locale.h>
#include <stdio.h>
#include <unistd.h>

#include <Alert.h>
#include <Catalog.h>
#include <LocaleRoster.h>

#include "NetworkTimeView.h"
#include "TimeMessages.h"
#include "TimeWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"


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
		"Time & Date, written by:\n\n\tAndrew Edward McCall\n\tMike Berg\n\t"
		"Julun\n\tPhilippe Saint-Pierre\n\nCopyright 2004-2012, Haiku."),
		B_TRANSLATE("OK"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


void
TimeApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSelectClockTab:
		case kShowHideTime:
		case B_LOCALE_CHANGED:
			fWindow->PostMessage(message);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


int
main(int argc, char** argv)
{
	if (argc > 1) {
		if (strcmp(argv[1], "--update") != 0)
			return 0;

		Settings settings;
		const char* errorString = NULL;
		int32 errorCode = 0;
		if (update_time(settings, &errorString, &errorCode) == B_OK) {
			printf("Synchronization successful\n");
		} else if (errorCode != 0) {
			printf("The following error occured "
					"while synchronizing:\n%s: %s\n",
				errorString, strerror(errorCode));
		} else {
			printf("The following error occured while synchronizing:\n%s\n",
				errorString);
		}
	} else {
		setlocale(LC_ALL, "");

		TimeApplication app;
		setuid(0);
		app.Run();
	}

	return 0;
}

