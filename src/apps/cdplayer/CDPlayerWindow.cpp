/*

CDPlayerWindow

Author: Sikosis

(C)2004 Haiku - http://haiku-os.org/

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <string.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "CDPlayer.h"
#include "CDPlayerWindows.h"
#include "CDPlayerViews.h"
// -------------------------------------------------------------------------------------------------- //

// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());	BPoint	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// -------------------------------------------------------------------------------------------------- //

// CDPlayerWindow - Constructor
CDPlayerWindow::CDPlayerWindow(BRect frame) : BWindow (frame, "CD Player", B_TITLED_WINDOW, B_NORMAL_WINDOW_FEEL , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);

	// Load User Settings 
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("CDPlayer_Settings",true);
	BFile file(path.Path(),B_READ_ONLY);
	BMessage msg;
	msg.Unflatten(&file);
	LoadSettings (&msg);

	Show();
}
// -------------------------------------------------------------------------------------------------- //


// CDPlayerWindow - Destructor
CDPlayerWindow::~CDPlayerWindow()
{
	//exit(0); - this is bad i seem to remember someone telling me ...
}
// -------------------------------------------------------------------------------------------------- //


// CDPlayerWindow::InitWindow -- Initialization Commands here
void CDPlayerWindow::InitWindow(void)
{
	BRect r;
	r = Bounds(); // the whole view
	
	// Create the Views
	AddChild(ptrCDPlayerView = new CDPlayerView(r));
}
// -------------------------------------------------------------------------------------------------- //


// CDPlayerWindow::QuitRequested -- Post a message to the app to quit
bool CDPlayerWindow::QuitRequested()
{
	SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
// -------------------------------------------------------------------------------------------------- //


// CDPlayerWindow::LoadSettings -- Loads your current settings
void CDPlayerWindow::LoadSettings(BMessage *msg)
{
	BRect frame;

	if (B_OK == msg->FindRect("windowframe",&frame)) {
		MoveTo(frame.left,frame.top);
		ResizeTo(frame.right-frame.left,frame.bottom-frame.top);
	}
}
// -------------------------------------------------------------------------------------------------- //


// CDPlayerWindow::SaveSettings -- Saves the Users settings
void CDPlayerWindow::SaveSettings(void)
{
	BMessage msg;
	msg.AddRect("windowframe",Frame());

	BPath path;
	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	if (result == B_OK) {
		path.Append("CDPlayer_Settings",true);
		BFile file(path.Path(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		msg.Flatten(&file);
	}
}
// -------------------------------------------------------------------------------------------------- //


// CDPlayerWindow::MessageReceived -- receives messages
void CDPlayerWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// -------------------------------------------------------------------------------------------------- //

