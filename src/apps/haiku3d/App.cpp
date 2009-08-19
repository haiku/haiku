/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include <Alert.h>

#include "App.h"
#include "MainWindow.h"


App::App()
	:
	BApplication("application/x-vnd.Haiku-Haiku3d")
{
	BRect frame(50, 50, 640 + 50, 480 + 50);
	const char *title = "Haiku3d";
	fMainWindow = new MainWindow(frame, title);
}


App::~App()
{
}


void App::AboutRequested()
{
	BAlert* alert;
	alert = new BAlert("About", "A little 3d demo", "ok");
	alert->Go(NULL);
}


void App::MessageReceived(BMessage *message)
{
}


int
main(int argc, char** argv)
{
	App app;
	app.Run();
	return 0;
}
