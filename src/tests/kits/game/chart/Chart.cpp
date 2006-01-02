/*
	
	Chart.cpp
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef CHART_WINDOW_H
#include "ChartWindow.h"
#endif
#ifndef CHART_H
#include "Chart.h"
#endif

#include <Debug.h>

int
main()
{	
	ChartApp *myApplication;

	myApplication = new ChartApp();
	myApplication->Run();
	
	delete(myApplication);
	return(0);
}

ChartApp::ChartApp() : BApplication("application/x-vnd.Be.ChartDemo")
{
	aWindow = new ChartWindow(BRect(120, 150, 629, 557), "Charts");
	
	// showing the window will also start the direct connection. If you
	// Sync() after the show, the direct connection will be established
	// when the Sync() return (as far as any part of the content area of
	// the window is visible after the show).
	aWindow->Show();
}
