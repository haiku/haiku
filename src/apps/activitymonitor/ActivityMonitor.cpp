/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ActivityMonitor.h"

#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <TextView.h>

#include "ActivityWindow.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ActivityMonitor"

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
	BString	text;
	text << kAppName << "\n"
		<< B_TRANSLATE("\twritten by Axel Dörfler\n"
		"\tCopyright 2008, Haiku Inc.\n");

	BAlert *alert = new BAlert(B_TRANSLATE("About"),
		text.String(), B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, strlen(kAppName), &font);

	alert->Go();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	ActivityMonitor app;
	app.Run();

	return 0;
}
