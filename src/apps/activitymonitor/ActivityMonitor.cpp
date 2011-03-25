/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ActivityMonitor.h"

#include <stdlib.h>

#include <AboutWindow.h>
#include <Application.h>

#include "ActivityWindow.h"

const char* kSignature = "application/x-vnd.Haiku-ActivityMonitor";


ActivityMonitor::ActivityMonitor()
	: BApplication(kSignature)
{
}


ActivityMonitor::~ActivityMonitor()
{
}


void
ActivityMonitor::ReadyToRun()
{
	fWindow = new ActivityWindow();
	fWindow->Show();
}


void
ActivityMonitor::RefsReceived(BMessage* message)
{
	fWindow->PostMessage(message);
}


void
ActivityMonitor::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
}


void
ActivityMonitor::AboutRequested()
{
	ShowAbout();
}


/*static*/ void
ActivityMonitor::ShowAbout()
{
	const char* kAuthors[] = {
		"Axel Dörfler",
		NULL
	};

	BAboutWindow aboutWindow(B_TRANSLATE_SYSTEM_NAME("ActivityMonitor"), 2008, kAuthors);
	aboutWindow.Show();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	ActivityMonitor app;
	app.Run();

	return 0;
}
