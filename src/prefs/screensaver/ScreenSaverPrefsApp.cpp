#include <Application.h>
#include <unistd.h>
#include <stdio.h>
#include <Path.h>

#include "ScreenSaverPrefsApp.h"

const char *APP_SIG = "application/x-vnd.OBOS.ScreenSaver";

ScreenSaverPrefsApp::~ScreenSaverPrefsApp(void) {
}

ScreenSaverPrefsApp::ScreenSaverPrefsApp(void) : BApplication(APP_SIG) {
  	m_MainForm = new ScreenSaverWin();
  	m_MainForm->Show();
}

void ScreenSaverPrefsApp::RefsReceived(BMessage *msg) {
	entry_ref ref;
	BEntry e;
	BPath p;
	msg->FindRef("refs", &ref); 
	e.SetTo(&ref, true); 
	e.GetPath(&p);

	char temp[2*B_PATH_NAME_LENGTH];
	sprintf (temp,"cp %s '/boot/home/config/add-ons/Screen Savers/'\n",p.Path());
	system(temp);
	m_MainForm->PostMessage(new BMessage(UPDATELIST));
}

void ScreenSaverPrefsApp::MessageReceived(BMessage *message) {
	switch(message->what) {
		case B_READY_TO_RUN: 
			break;
		default: {
			BApplication::MessageReceived(message);
			}
		}
}

int main(void) {
  ScreenSaverPrefsApp app;
  app.Run();
  return 0;
}
