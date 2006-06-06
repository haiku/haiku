/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mikael Konradson, mikael.konradson@gmail.com
 */


#include "FontDemo.h"
#include "FontDemoView.h"
#include "ControlView.h"

#include <Window.h>


FontDemo::FontDemo()
	: BApplication("application/x-vnd.Haiku-FontDemo")
{
}


FontDemo::~FontDemo()
{
}


void
FontDemo::ReadyToRun()
{
	// Create the demo window where we draw the string
	BWindow* demoWindow = new BWindow(BRect(80, 30, 490, 300), "FontDemo",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	FontDemoView* demoView = new FontDemoView(demoWindow->Bounds());
	demoWindow->AddChild(demoView);

	BWindow* controlWindow = new BWindow(BRect(500,30, 700, 352), "Controls",
		B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS);

	ControlView* controlView = new ControlView(controlWindow->Bounds());
	controlWindow->AddChild(controlView);

	controlView->SetTarget(demoView);

	demoWindow->Show();	
	controlWindow->Show();								
}


//	#pragma mark -


int
main()
{
	FontDemo fontdemo;
	fontdemo.Run();
	return 0;
}

