/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Atsushi Takamatsu
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 */


#include "HApp.h"
#include "HWindow.h"

#include <Alert.h>


HApp::HApp()
	: BApplication("application/x-vnd.Haiku-Sounds")
{
	BRect rect;
	rect.Set(200, 150, 500, 450);

	HWindow *win = new HWindow(rect, "Sounds");
	win->Show();
}


HApp::~HApp()
{
}


void
HApp::AboutRequested()
{
	(new BAlert("About Sounds", "Sounds\n"
			    "  Brought to you by :\n"
			    "	Oliver Ruiz Dorantes\n"
			    "	Jérôme DUVAL.\n"
			    "  Original work from Atsushi Takamatsu.\n"
			    "Copyright " B_UTF8_COPYRIGHT "2003-2006 Haiku","OK"))->Go();
}


//	#pragma mark -


int
main(int, char**)
{
	HApp app;
	app.Run();

	return 0;
}

