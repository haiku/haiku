#include <Application.h>

#include "ScreenSaverPrefsApp.h"

const char *APP_SIG = "application/x-vnd.OBOS.ScreenSaver";

ScreenSaverPrefsApp::~ScreenSaverPrefsApp(void) {
}

ScreenSaverPrefsApp::ScreenSaverPrefsApp(void) : BApplication(APP_SIG) {
  	m_MainForm = new ScreenSaverWin();
  	m_MainForm->Show();
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
