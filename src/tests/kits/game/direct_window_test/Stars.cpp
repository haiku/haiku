/*
	
	Stars.cpp
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STAR_WINDOW_H
#include "StarWindow.h"
#endif
#ifndef STARS_H
#include "Stars.h"
#endif

#include <Debug.h>

int main(int, char**)
{	
	StarsApp *myApplication;

	myApplication = new StarsApp();
	if (!myApplication->abort_required)
		myApplication->Run();	
	delete(myApplication);
	return(0);
}

StarsApp::StarsApp() : BApplication("application/x-vnd.Be.StarsDemo")
{
	abort_required = false;
	aWindow = new StarWindow(BRect(120, 150, 540, 420), "Stars");
	// showing the window will also start the direct connection. If you
	// Sync() after the show, the direct connection will be established
	// when the Sync() return (as far as any part of the content area of
	// the window is visible after the show).
	if (!abort_required)
		aWindow->Show();
}
