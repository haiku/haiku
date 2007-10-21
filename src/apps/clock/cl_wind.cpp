/*
 *	Copyright 1999, Be Incorporated. All Rights Reserved.
 *	This file may be used under the terms of the Be Sample Code License.
 */

#include "cl_wind.h"
#include "cl_view.h"

#include <Application.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>


#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


TClockWindow::TClockWindow(BRect frame, const char* title)
	: BWindow(frame, title, B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AVOID_FRONT, B_ALL_WORKSPACES),
	  fOnScreenView(NULL)
{
	_InitWindow();
}


TClockWindow::~TClockWindow()
{
}


bool
TClockWindow::QuitRequested()
{
	BPath path;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append("Clock_settings");
		int ref = creat(path.Path(), 0777);
		if (ref >= 0) {
			BPoint lefttop = Frame().LeftTop();
			write(ref, (char *)&lefttop, sizeof(BPoint));
			short face = fOnScreenView->ReturnFace();
			write(ref, (char *)&face, sizeof(short));
			bool seconds = fOnScreenView->ReturnSeconds();
			write(ref, (char *)&seconds, sizeof(bool));
			close(ref);
		}
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TClockWindow::_InitWindow()
{
	// half second pulse rate
	SetPulseRate(500000);
	
	fOnScreenView = new TOnscreenView(BRect(0, 0, 82, 82), "Clock", 22, 15, 41);
	AddChild(fOnScreenView);

	int ref;
	BPath path;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Clock_settings");
		ref = open(path.Path(), O_RDONLY);
		if (ref >= 0) {
			BPoint leftTop;
			read(ref, (char*)&leftTop, sizeof(leftTop));

			short face;
			read(ref, (char *)&face, sizeof(short));
			fOnScreenView->UseFace(face);

			bool secs;
			read(ref, (char *)&secs, sizeof(bool));
			fOnScreenView->ShowSecs(secs);

			close(ref);
			
			MoveTo(leftTop);
 
			BRect frame = Frame();
			frame.InsetBy(-4, -4);
			// it's not visible so reposition. I'm not going to get
			// fancy here, just place in the default location
			if (!frame.Intersects(BScreen(this).Frame()))
				MoveTo(100, 100);
		}
	}
}

