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
#include "Alert.h"


// Start the server application. Set pulse to fire once per second.
// Run until the application quits.
int main(int, char**) {	
	ScreenSaverApp myApplication;
	myApplication.Run();
	return(0);
}

// Construct the server app. Doesn't do much, at this point.
ScreenSaverApp::ScreenSaverApp() : BApplication("application/x-vnd.OBOS-ScreenSaverApp"),addon_image(0),win(NULL) {
}

void ScreenSaverApp::ReadyToRun(void) {
	if (!pref.LoadSettings() || (!LoadAddOn()))
		exit(1);
	else {	// If everything works OK, create a BDirectWindow and start the render thread.
		BScreen theScreen(B_MAIN_SCREEN_ID);
		win=new SSAwindow(theScreen.Frame(),saver);
		thrd=new ScreenSaverThread(saver,win,win->view,&pref);
		threadID=spawn_thread(threadFunc,"ScreenSaverRenderer",0,thrd);
		resume_thread(threadID);
		HideCursor();
		}
}

bool ScreenSaverApp::QuitRequested(void) {
	kill_thread(threadID);
	delete thrd;
	return true;
}

bool ScreenSaverApp::LoadAddOn() {
	BScreenSaver *(*instantiate)(BMessage *, image_id );
	if (addon_image) { // This is a new set of preferences. Free up what we did have 
		unload_add_on(addon_image);
		if (saver)
			delete saver;
		}
	char temp[B_PATH_NAME_LENGTH]; // Yes, this is a lot...
	if (B_OK==find_directory(B_BEOS_ADDONS_DIRECTORY,NULL,false,temp,B_PATH_NAME_LENGTH)) { 
		sprintf (temp,"%s/Screen Savers/%s",temp,pref.ModuleName());
		//printf ("Trying to open add-on: %s\n",temp);
		addon_image = load_add_on(temp);
		}
	if (addon_image<0)  {
		//printf ("Unable to open add-on: %s\n",temp);
		sprintf (temp,"%s/Screen Savers/%s",temp,pref.ModuleName());
		if (B_OK==find_directory(B_COMMON_ADDONS_DIRECTORY,NULL,false,temp,B_PATH_NAME_LENGTH)) { 
			sprintf (temp,"%s/Screen Savers/%s",temp,pref.ModuleName());
			//printf ("Trying to open add-on: %s\n",temp);
			addon_image = load_add_on(temp);
			}
		}
	if (addon_image<0)  {
		//printf ("Unable to open add-on: %s\n",temp);
		if (B_OK==find_directory(B_USER_ADDONS_DIRECTORY,NULL,false,temp,B_PATH_NAME_LENGTH)) { 
			sprintf (temp,"%s/Screen Savers/%s",temp,pref.ModuleName());
			//printf ("Trying to open add-on: %s\n",temp);
			addon_image = load_add_on(temp);
			}
		}
	if (addon_image<0) {
		printf ("Unable to open add-on: %s\n",temp);
		printf ("add on image = %ld!\n",addon_image);
		return false;
		}
	else {
		// Look for the one C function that should exist.
		status_t retVal;
		if (B_OK != (retVal=get_image_symbol(addon_image, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate))) {
			printf ("Unable to find the instantiator\n");
			printf ("Error = %ld\n",retVal);
			return false;
			}
		else
			saver=instantiate(pref.GetState(),addon_image);
			if (B_OK!=saver->InitCheck()) {
				unload_add_on(addon_image);
				delete saver;
				saver=NULL;
				return false;
				}
		}
	return true;
}

