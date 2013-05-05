/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 */


#include "Mouse.h"
#include "MouseWindow.h"

#include <Alert.h>
#include <Screen.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MouseApplication"

const char* kSignature = "application/x-vnd.Haiku-Mouse";


MouseApplication::MouseApplication()
	:
	BApplication(kSignature)
{
	BRect rect(0, 0, 397, 293);
	MouseWindow *window = new MouseWindow(rect);
	window->Show();
}


void
MouseApplication::AboutRequested()
{
	BAlert* alert = new BAlert("about",
		B_TRANSLATE("...by Andrew Edward McCall"), B_TRANSLATE("Dig Deal"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


//	#pragma mark -


int
main(int /*argc*/, char ** /*argv*/)
{
	MouseApplication app;
	app.Run();

	return 0;
}
