#ifndef SCREEN_SAVER_H
#include "ScreenSaverApp.h"
#endif
#include <stdio.h>
#include "Screen.h"
#include "image.h"
#include "StorageDefs.h"
#include "FindDirectory.h"
#include "SupportDefs.h"
#include "File.h"
#include "Path.h"
#include "string.h"
#include "Beep.h"


// Start the server application. Set pulse to fire once per second.
// Run until the application quits.
int main(int, char**) {	
	ScreenSaverApp myApplication;
	myApplication.Run();
	return(0);
}

// Construct the server app. Doesn't do much, at this point.
ScreenSaverApp::ScreenSaverApp() : BApplication("application/x-vnd.OBOS-ScreenSaverApp"),win(NULL) {
	blankTime=real_time_clock();
}

void ScreenSaverApp::ReadyToRun(void) {
	if (!pref.LoadSettings()) 
		exit(1);
	else {	// If everything works OK, create a BDirectWindow and start the render thread.
		BScreen theScreen(B_MAIN_SCREEN_ID);
		win=new SSAwindow(theScreen.Frame());
		pww=new pwWindow();
		thrd=new ScreenSaverThread(win,win->view,&pref);

		saver=thrd->LoadAddOn();
		if (!saver)
			exit(1);
		win->SetSaver(saver);
		win->SetFullScreen(true);
		win->Show();
		threadID=spawn_thread(threadFunc,"ScreenSaverRenderer",0,thrd);
		resume_thread(threadID);
		HideCursor();
		}
}

void ScreenSaverApp::ShowPW(void) {
		win->Lock();
		suspend_thread(threadID);
		if (B_OK==win->SetFullScreen(false)) {
			win->Sync();
			ShowCursor();
			pww->Show();
			pww->Sync();
			}
		win->Unlock();
}

void ScreenSaverApp::MessageReceived(BMessage *message) {
  switch(message->what) {
    case 'DONE':
		if (strcmp(pww->GetPassword(),pref.Password())) {
			beep();
			pww->Hide();
			win->SetFullScreen(true);
			if (threadID)
				resume_thread(threadID);
			}
			else  {
				printf ("Quitting!\n");
				Quit();
			}
		break;
    case 'MOO1':
		if (real_time_clock()-blankTime>pref.PasswordTime())
			ShowPW();
		else 
			Quit();
		break;
    default:
      	BApplication::MessageReceived(message);
      	break;
  }
}
bool ScreenSaverApp::QuitRequested(void) {
	if (threadID)
		kill_thread(threadID);
	if (thrd)
		delete thrd;
	return true;
}

