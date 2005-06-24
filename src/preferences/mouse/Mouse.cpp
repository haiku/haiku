// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			Mouse.cpp
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include <Screen.h>

#include "Mouse.h"
#include "MouseWindow.h"


const char kMouseApplicationSig[] = "application/x-vnd.haiku.Mouse";


MouseApplication::MouseApplication()
	: BApplication(kMouseApplicationSig)					
{
	BRect rect(0, 0, 397, 293);
	MouseWindow *window = new MouseWindow(rect);
	window->Show();
}


void
MouseApplication::AboutRequested(void)
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
