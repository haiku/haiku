/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Switcher.h"

#include <stdlib.h>

#include <Application.h>
#include <Catalog.h>

#include "CaptureWindow.h"
#include "PanelWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Switcher"


const char* kSignature = "application/x-vnd.Haiku-Switcher";


Switcher::Switcher()
	:
	BApplication(kSignature),
	fOccupiedLocations(0)
{
}


Switcher::~Switcher()
{
}


void
Switcher::ReadyToRun()
{
	CaptureWindow* window = new CaptureWindow();
	window->Run();

	fCaptureMessenger = window;
}


void
Switcher::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgLocationTrigger:
		{
			uint32 location = (uint32)message->FindInt32("location");
			if ((location & fOccupiedLocations) == 0) {
				// TODO: make function configurable
				uint32 which = kShowApplicationWindows;
				if ((location & (kTopEdge | kBottomEdge)) != 0)
					which = kShowApplications;

				new PanelWindow(location, which,
					(team_id)message->FindInt32("team"));
				fOccupiedLocations |= location;
			}
			break;
		}

		case kMsgLocationFree:
		{
			uint32 location;
			if (message->FindInt32("location", (int32*)&location) == B_OK)
				fOccupiedLocations &= ~location;
			break;
		}

		case kMsgHideWhenMouseMovedOut:
			fCaptureMessenger.SendMessage(message);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	Switcher app;
	app.Run();

	return 0;
}
