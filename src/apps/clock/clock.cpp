/*
	
	Clock.cpp
	
	13 apr 94	elr	new today

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>

#include "clock.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Path.h>
#include <Screen.h>
#include <FindDirectory.h>
#include <Dragger.h>

int main(int argc, char* argv[])
{	
	THelloApplication *myApplication;

	myApplication = new THelloApplication();
	myApplication->Run();
	
	delete myApplication;
	return 0;
}

const char *app_signature = "application/x-vnd.Be-simpleclock";

THelloApplication::THelloApplication()
		  :BApplication(app_signature)
{
	BRect			windowRect, viewRect;
	BPoint			wind_loc;
	int				ref;
	short			face;
	bool			secs;
	BPath			path;

	viewRect.Set(0, 0, 82, 82);
	myView = new TOnscreenView(viewRect, "Clock",22,15,41);
	windowRect.Set(100, 100, 182, 182);
	myWindow = new TClockWindow(windowRect, "Clock");
	face = 1;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Clock_settings");
		ref = open(path.Path(), O_RDONLY);
		if (ref >= 0) {
			read(ref, (char *)&wind_loc, sizeof(wind_loc));
			read(ref, (char *)&face, sizeof(short));
			read(ref, (char *)&secs, sizeof(bool));
			close(ref);
			myWindow->MoveTo(wind_loc);

			BRect frame = myWindow->Frame();
			frame.InsetBy(-4, -4);
			if (!frame.Intersects(BScreen(myWindow).Frame())) {
				// it's not visible so reposition. I'm not going to get
				// fancy here, just place in the default location
				myWindow->MoveTo(100, 100);
			}
		}
	}

//+	BPopUpMenu	*menu = MainMenu();
//+	BMenuItem	*item = new BMenuItem("Show Seconds", new BMessage(SHOW_SECONDS));
//+	item->SetTarget(this);
//+	item->SetMarked(secs);
//+
//+	menu->AddItem(new BSeparatorItem(), 1);
//+	menu->AddItem(item, 2);
//+	menu->AddItem(new BSeparatorItem(), 3);

	myWindow->Lock();
	myWindow->AddChild(myView);
	myWindow->theOnscreenView = myView;

//+	BRect r = myView->Frame();
//+	r.top = r.bottom - 7;
//+	r.left = r.right - 7;
//+	BDragger *dw = new BDragger(r, myView, 0);
//+	myWindow->AddChild(dw);

	myView->UseFace( face );
	myView->ShowSecs( secs );
	myView->Pulse();		// Force update
	myWindow->Show();
	myWindow->Unlock();
}

void THelloApplication::MessageReceived(BMessage *msg)
{
	if (msg->what == SHOW_SECONDS) {
//+		if (myWindow->Lock()) {
//+			BMenuItem *item = MainMenu()->FindItem(SHOW_SECONDS);
//+			item->SetMarked(!item->IsMarked());
//+//+			myView->ShowSecs(item->IsMarked());
//+			myWindow->Unlock();
//+		}
	} else {
		BApplication::MessageReceived(msg);
	}
}
