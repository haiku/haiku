/*
	
	cl_wind.cpp
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "cl_wind.h"
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Debug.h>

TClockWindow::TClockWindow(BRect r, const char* t)
			 :BWindow(r, t, B_TITLED_WINDOW,
			 	B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AVOID_FRONT,
				B_ALL_WORKSPACES)
{
	SetPulseRate(500000);		// half second pulse rate
}

bool TClockWindow::QuitRequested( void )
{
	int			ref;
	BPoint		lefttop;
	short		face;
	bool		seconds;
	BPath		path;
//	int			len;

	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append("Clock_settings");
		ref = creat(path.Path(), 0777);
		if (ref >= 0) {
			lefttop = Frame().LeftTop();
			write(ref, (char *)&lefttop, sizeof(BPoint));
			face = theOnscreenView->ReturnFace();
			write(ref, (char *)&face, sizeof(short));
			seconds = theOnscreenView->ReturnSeconds();
			write(ref, (char *)&seconds, sizeof(bool));
			close(ref);
		}
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}
