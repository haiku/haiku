/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "PictureTestApp.h"
#include "PictureTestWindow.h"

int main()
{
	PictureTestApp app;
	app.Run();
	return 0;
}

PictureTestApp::PictureTestApp()
	: Inherited(APPLICATION_SIGNATURE)
{
}

void PictureTestApp::ReadyToRun()
{
	PictureTestWindow * window = new PictureTestWindow();
	window->Show();
}
