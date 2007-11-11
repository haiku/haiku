/*
  -----------------------------------------------------------------------------

	File:	GLMovApp.cpp
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
*/

#include <Alert.h>
#include "GLMovApp.h"
#include "GLMovStrings.h"
#include "GLMovWindow.h"

GLMovApp::GLMovApp()
	: BApplication("application/x-vnd.Be-3DM2")
{
}

GLMovApp::~GLMovApp()
{
}

void GLMovApp::ReadyToRun()
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

int main(int argc, char **argv)
{
	GLMovApp app;
	app.Run();
	return 0;
}
