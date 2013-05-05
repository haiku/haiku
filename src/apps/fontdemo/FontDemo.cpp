/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mikael Konradson, mikael.konradson@gmail.com
 */


#include "ControlView.h"
#include "FontDemo.h"
#include "FontDemoView.h"

#include <Catalog.h>
#include <Window.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FontDemo"

const BString APP_NAME = B_TRANSLATE_SYSTEM_NAME("FontDemo");

FontDemo::FontDemo()
	: BApplication("application/x-vnd.Haiku-FontDemo")
{
	// Create the demo window where we draw the string
	BWindow* demoWindow = new BWindow(BRect(80, 30, 490, 300), APP_NAME,
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	FontDemoView* demoView = new FontDemoView(demoWindow->Bounds());
	demoWindow->AddChild(demoView);

	BWindow* controlWindow = new BWindow(BRect(500, 30, 700, 402), B_TRANSLATE("Controls"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS);

	ControlView* controlView = new ControlView(controlWindow->Bounds());
	controlWindow->AddChild(controlView);

	controlView->SetTarget(demoView);

	demoWindow->Show();
	controlWindow->Show();
}


FontDemo::~FontDemo()
{
}


void
FontDemo::ReadyToRun()
{

}


//	#pragma mark -


int
main()
{
	FontDemo fontdemo;
	fontdemo.Run();
	return 0;
}

