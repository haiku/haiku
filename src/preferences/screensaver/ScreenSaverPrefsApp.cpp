/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Michael Phipps
 *              Jérôme Duval, jerome.duval@free.fr
 */

#include <Application.h>
#include <Entry.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <Path.h>

#include "ScreenSaverPrefsApp.h"

const char *APP_SIG = "application/x-vnd.haiku.ScreenSaver";

ScreenSaverPrefsApp::ScreenSaverPrefsApp() : BApplication(APP_SIG) 
{
  	fScreenSaverWin = new ScreenSaverWin();
  	fScreenSaverWin->Show();
}


void 
ScreenSaverPrefsApp::RefsReceived(BMessage *msg) 
{
	entry_ref ref;
	BEntry e;
	BPath p;
	msg->FindRef("refs", &ref); 
	e.SetTo(&ref, true); 
	e.GetPath(&p);

	char temp[2*B_PATH_NAME_LENGTH];
	sprintf (temp,"cp %s '/boot/home/config/add-ons/Screen Savers/'\n",p.Path());
	system(temp);
	fScreenSaverWin->PostMessage(kUpdatelist);
}


int main(void) {
  ScreenSaverPrefsApp app;
  app.Run();
  return 0;
}
