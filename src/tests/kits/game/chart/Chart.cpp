/*

	Chart.cpp

	by Pierre Raynaud-Richard.

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ChartWindow.h"
#include "Chart.h"

#include <Catalog.h>
#include <Debug.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Chart"

int
main()
{
	ChartApp *app = new ChartApp();
	app->Run();

	delete app;
	return 0;
}


ChartApp::ChartApp() : BApplication("application/x-vnd.Be.ChartDemo")
{
	fWindow = new ChartWindow(BRect(120, 150, 629, 591),
		B_TRANSLATE_SYSTEM_NAME("Chart"));

	// showing the window will also start the direct connection. If you
	// Sync() after the show, the direct connection will be established
	// when the Sync() return (as far as any part of the content area of
	// the window is visible after the show).
	fWindow->Show();
}
