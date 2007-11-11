/* 
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */

#include <Alert.h>
#include "GLMovApp.h"
#include "GLMovStrings.h"
#include "GLMovWindow.h"

GLMovApp::GLMovApp()
	: BApplication("application/x-vnd.Haiku-3DMov")
{
}

GLMovApp::~GLMovApp()
{
}

void
GLMovApp::ReadyToRun()
{
	GLMovWindow *win = GLMovWindow::MakeWindow(OBJ_CUBE);
}

void GLMovApp::AboutRequested()
{
	BAlert *alert;
	alert = new BAlert("about", STR_ABOUT_TEXT, STR_OK);
	alert->Go(NULL);
}

void GLMovApp::MessageReceived(BMessage *message)
{
}

int
main(int argc, char **argv)
{
	GLMovApp app;
	app.Run();
	return 0;
}
