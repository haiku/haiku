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
int main(int, char**) 
{	
	ScreenSaverApp myApplication;
	myApplication.Run();
	return(0);
}

// Construct the server app. Doesn't do much, at this point.
ScreenSaverApp::ScreenSaverApp() : BApplication("application/x-vnd.OBOS-ScreenSaverApp"),fWin(NULL) 
{
	fBlankTime=real_time_clock();
}


void 
ScreenSaverApp::ReadyToRun(void) 
{
	if (!fPref.LoadSettings()) 
		exit(1);
	else {	// If everything works OK, create a BDirectWindow and start the render thread.
		BScreen theScreen(B_MAIN_SCREEN_ID);
		fWin=new SSAwindow(theScreen.Frame());
		fPww=new pwWindow();
		fThrd=new ScreenSaverThread(fWin,fWin->fView,&fPref);

		fSaver=fThrd->LoadAddOn();
		if (!fSaver)
			exit(1);
		fWin->SetSaver(fSaver);
		fWin->SetFullScreen(true);
		fWin->Show();
		fThreadID=spawn_thread(threadFunc,"ScreenSaverRenderer",0,fThrd);
		resume_thread(fThreadID);
		HideCursor();
		}
}


void 
ScreenSaverApp::ShowPW(void) 
{
		fWin->Lock();
		suspend_thread(fThreadID);
		if (B_OK==fWin->SetFullScreen(false)) {
			fWin->Sync();
			ShowCursor();
			fPww->Show();
			fPww->Sync();
			}
		fWin->Unlock();
}


void 
ScreenSaverApp::MessageReceived(BMessage *message) 
{
  switch(message->what) {
    case 'DONE':
		if (strcmp(fPww->GetPassword(),fPref.Password())) {
			beep();
			fPww->Hide();
			fWin->SetFullScreen(true);
			if (fThreadID)
				resume_thread(fThreadID);
			}
			else  {
				printf ("Quitting!\n");
				Quit();
			}
		break;
    case 'MOO1':
		if (real_time_clock()-fBlankTime>fPref.PasswordTime())
			ShowPW();
		else 
			Quit();
		break;
    default:
      	BApplication::MessageReceived(message);
      	break;
  }
}


bool 
ScreenSaverApp::QuitRequested(void) 
{
	if (fThreadID)
		kill_thread(fThreadID);
	if (fThrd)
		delete fThrd;
	return true;
}
