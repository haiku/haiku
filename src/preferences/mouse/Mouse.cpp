/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Jérôme Duval
 *		Axel Dörfler (axeld@pinc-software.de)
 */


#include "Mouse.h"
#include "MouseWindow.h"

#include <Alert.h>
#include <Screen.h>


const char* kSignature = "application/x-vnd.Haiku-Mouse";


MouseApplication::MouseApplication()
	: BApplication(kSignature)					
{
	BRect rect(0, 0, 397, 293);
	MouseWindow *window = new MouseWindow(rect);
	window->Show();
}


void
MouseApplication::AboutRequested()
{
	(new BAlert("about", "...by Andrew Edward McCall", "Dig Deal"))->Go();
}


//	#pragma mark -


int
main(int /*argc*/, char **/*argv*/)
{
	MouseApplication app;
	app.Run();

	return 0;
}
