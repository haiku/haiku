#ifndef SCREEN_SAVER_H
#include "ScreenSaverApp.h"
#endif
#include <stdio.h>
#include "Screen.h"
#include "image.h"
#include "StorageDefs.h"
#include "FindDirectory.h"
#include "File.h"
#include "Path.h"
#include "string.h"

extern int32 screenSaverThread(void *);
class BScreenSaver;

// Yes, these are all global variables. They are shared between the thread
// and this app. 
SSstates nextAction; 
int32 frame;
BView *view;
BScreenSaver *saver;
sem_id ssSem;
SSAwindow *win;

extern void callSaverDelete (void);

/* General note on how this works:
 * The semaphore (ssSem) is used to start and stop the render thread.
 * When it is time to go blank, the ScreenSaver app thread releases
 * the semaphore. The render thread was blocked waiting for it.
 * When it is time to stop, the shared variable (nextAction) is set to STOP. 
 * The thread stops and acquires the semaphore, so that it waits...
*/

// Start the server application. Set pulse to fire once per second.
// Run until the application quits.
int main(int, char**)
{	
	ScreenSaverApp myApplication;
	myApplication.SetPulseRate(1000000); // Fire once per second.
	ssSem=create_sem(0,"ScreenSaverSemaphore");
	nextAction=STOP;
	myApplication.Run();
	return(0);
}

// Construct the server app. Doesn't do much, at this point.
ScreenSaverApp::ScreenSaverApp() : BApplication("application/x-vnd.OBOS-ScreenSaverApp")
{
	addon_image=0;	
	currentTime=0;
	win=NULL;
}


// This is the main message loop. It spawns a thread that actually runs the screen saver.
void ScreenSaverApp::DispatchMessage(BMessage *msg,BHandler *target)
{
	switch (msg->what)
		{
		case B_READY_TO_RUN:
			{
			threadID=spawn_thread(screenSaverThread,"Screen Saver Thread",0,NULL);
			LoadSettings();
			// If everything works OK, create a BDirectWindow and start the render thread.
			BScreen theScreen(B_MAIN_SCREEN_ID);
			win=new SSAwindow(theScreen.Frame());
			resume_thread(threadID);
			}
			break;
		case 'TOPL':
		case 'TOPR':
		case 'BOTL':
		case 'BOTR':
		case 'ENDC':
			{
			if (cornerNow==msg->what)
				goBlank();
			else if (cornerNever==msg->what)
				disabled=true;
			else // We are in the middle or some other corner
				{
				disabled=false;
				unBlank();
				}
			}
		case 'INPT':
			unBlank();
			break;
		case NEWDURATION:
			int32 dur;
			msg->FindInt32("duration",&dur);
			setNewDuration(dur);
			break;
		case B_PULSE:
			if (++currentTime==blankTime)
				goBlank();
			break;
		case B_QUIT_REQUESTED:
			nextAction=EXIT;
			status_t dummy; // The thread has no return value
			wait_for_thread(threadID,&dummy);
			BApplication::QuitRequested();
			break;
		default:
			printf ("Unknown message! %d\n",msg->what);
			break;
		}
}

// Load the flattened settings BMessage from disk and parse it.
void ScreenSaverApp::LoadSettings(void)
{
	bool ok=false;
	char pathAndFile[1024]; // Yes, this is very long...
	BPath path;

	status_t found=find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	if (found==B_OK)
		{
		strncpy(pathAndFile,path.Path(),1023);
		strncat(pathAndFile,"/ScreenSaver_settings",1023);
		BFile ssSettings(pathAndFile,B_READ_ONLY);
		if (B_OK==ssSettings.InitCheck())
			{ // File exists. Unflatten the message and call the settings parser.
			BMessage settings;
			settings.Unflatten(&ssSettings);
			parseSettings (&settings);
			ok=true;
			}
		}
	if (!ok)// If we can not find a settings file, post quit so that the app will quit right away...
		Quit();
}

void ScreenSaverApp::LoadAddOn(BMessage *msg)
{
	BScreenSaver *(*instantiate)(BMessage *, image_id );
	image_id addon_image;
	char temp[2048]; // Yes, this is a lot...
	sprintf (temp,"/boot/beos/system/add-ons/Screen Savers/%s",moduleName);
	if (addon_image) // This is a new set of preferences. Free up what we did have
		{
		unload_add_on(addon_image);
		callSaverDelete();
		}
	addon_image = load_add_on(temp);
	if (addon_image<0)
		{
		blankTime=-1;
		printf ("Unable to open the add-on\n");
		printf ("add on image = %x!\n",addon_image);
		}
	else
		// Look for the one C function that should exist.
		if (B_OK != get_image_symbol(addon_image, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate))
			{
			blankTime=-1;
			printf ("Unable to find the instantiator\n");
			printf ("Error = %d\n",get_image_symbol(addon_image, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate));
			}
		else
			saver=instantiate(msg,addon_image);
}

// Blank the screen.
void ScreenSaverApp::goBlank(void)
{
	if (win && (blankTime> -1) && !disabled)
		{
		win->SetFullScreen(true);
		win->Show(); 
		win->Sync(); 
		HideCursor();
		nextAction=DIRECTDRAW;
		release_sem(ssSem); // Tell the drawing thread to go and draw.
		}
}

// Unblank. Only works for direct draws.
void ScreenSaverApp::unBlank(void)
{ 
	if (passwordTime > 0)
		{

		}
	resetTimer();
	if (win  && (nextAction == DIRECTDRAW))
		{
		win->SetFullScreen(false);
		win->Hide(); 
		win->Sync(); 
		ShowCursor();
		nextAction=STOP;
		}
}

// Reset the timer. Keyboard or mouse event.
void ScreenSaverApp::resetTimer(void)
{
	currentTime=0;
}

// Set a new duration for blanking
void ScreenSaverApp::setNewDuration(int newTimer)
{
	blankTime=newTimer;
}

void setOnValue(BMessage *msg, char *name, int &result)
{
	int32 value;
	if (B_OK == msg->FindInt32(name,&value)) // If screen saving is even enabled
		result=value;
	// printf ("Read parameter %s, setting it to:%d\n",name,result);
}

void ScreenSaverApp::parseSettings (BMessage *msg)
{
	int temp;
	const char *strPtr,*passwordPointer;
	char pathAndFile[1024]; // Yes, this is very long...
	BPath path;

	passwordPointer=password;
	setOnValue(msg,"timeflags",temp);
	if (temp) // If screen saver is enabled, set blanktime. 
		setOnValue(msg,"timefade",blankTime);
	else
		blankTime=-1;
	// timestandby is not used, for now.
	setOnValue(msg,"cornernow",cornerNow);
	setOnValue(msg,"cornernever",cornerNever);
	passwordTime=-1; // This is pessimistic - assume that password is off OR that a settings load will fail.
	setOnValue(msg,"lockenable",temp);
	if (temp && (B_OK == msg->FindString("lockmethod",&strPtr)))
		{ // Get the password. Either from the settings file (that's secure) or the networking file (also secure).
		if (strcmp(strPtr,"network")) // Not network, therefore from the settings file
			if (B_OK == msg->FindString("lockpassword",&passwordPointer))
				setOnValue(msg,"lockdelay",passwordTime);
		else // Get from the network file
			{
			status_t found=find_directory(B_USER_SETTINGS_DIRECTORY,&path);
			if (found==B_OK)
				{
				FILE *networkFile=NULL;
				char buffer[512],*start;
				strncpy(pathAndFile,path.Path(),1023);
				strncat(pathAndFile,"/network",1023);
				// This ugly piece opens the networking file and reads the password, if it exists.
				if (networkFile=fopen(pathAndFile,"r"))
					while (buffer==fgets(buffer,512,networkFile))
						if (start=strstr(buffer,"PASSWORD ="))
							strncpy(password, start+10,strlen(start-11));
				}
			}
		setOnValue(msg,"lockdelay",passwordTime);
		}
	if (B_OK != msg->FindString("modulename",&strPtr)) 
		blankTime=-1; // If the module doesn't exist, never blank.
	strcpy(moduleName,strPtr);
	LoadAddOn(msg);
}
