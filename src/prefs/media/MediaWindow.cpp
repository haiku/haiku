/*

Media - MediaWindow by Sikosis

(C)2003

*/


// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TextView.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "Media.h"
#include "MediaViews.h"
#include "MediaWindows.h"

// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow - Constructor
MediaWindow::MediaWindow(BRect frame) : BWindow (frame, "OBOS Media", B_TITLED_WINDOW, B_NORMAL_WINDOW_FEEL , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	Show();
}


// MediaWindow - Destructor
MediaWindow::~MediaWindow()
{
	exit(0);
}


// MediaWindow::InitWindow -- Initialization Commands here
void MediaWindow::InitWindow(void)
{	
	BRect r;
	r = Bounds(); // the whole view
	
	// Create the Views	
	AddChild(ptrMediaView = new MediaView(r));
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow::QuitRequested -- Post a message to the app to quit
bool MediaWindow::QuitRequested()
{
   be_app->PostMessage(B_QUIT_REQUESTED);
   return true;
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow::FrameResized -- When the main frame is resized fix up the other views
void MediaWindow::FrameResized (float width, float height)
{
	// This makes sure our SideBar colours down to the bottom when resized
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow::MessageReceived -- receives messages
void MediaWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// ---------------------------------------------------------------------------------------------------------- //


