/*

Media - MediaWindow by Sikosis

(C)2003

*/


// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Font.h>
#include <ListView.h>
#include <math.h>
#include <OutlineListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <StringView.h>
#include <TextView.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

// Media Includes
#include <ParameterWeb.h>
#include <TimeSource.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <MultiChannelControl.h>

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
	
	// Grab Media Info
	media_node mixerNode;
	BMediaRoster* roster = BMediaRoster::Roster();
	roster->GetAudioMixer(&mixerNode);
	roster->GetParameterWebFor(mixerNode, &mParamWeb);

	// we look up the global BMediaRoster object, 
	// ask it for a reference to the global system mixer's node, 
	// then get a copy of the node's BParameterWeb.
	BMediaTheme* theme = BMediaTheme::PreferredTheme();
	BView* paramView = theme->ViewFor(mParamWeb);
		
	// Create the OutlineView
	BRect outlinerect(r.left+12,r.top+12,r.left+146,r.bottom-10);
	BRect AvailableRect(r.left+160,r.top+12,r.right-10,r.bottom-10);
	BRect AudioMixerRect(r.left+160,r.top+41,r.right-10,r.bottom-10);
	BOutlineListView *outline;
	BListItem 		 *topmenu;
	BListItem 		 *submenu;
	BScrollView 	 *outlinesv;

	outline = new BOutlineListView(outlinerect,"audio_list", B_SINGLE_SELECTION_LIST);
	outline->AddItem(topmenu = new BStringItem("Audio Settings"));
	outline->AddUnder(submenu = new BStringItem("Audio Mixer"), topmenu);
	outline->AddUnder(new BStringItem("None Out"), submenu);
	outline->AddUnder(new BStringItem("None In"), submenu);
	outline->AddItem(topmenu = new BStringItem("Video Settings"));
	outline->AddUnder(submenu = new BStringItem("Video Window Consumer"), topmenu);
	outlinesv = new BScrollView("scroll_audio", outline, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, false, B_FANCY_BORDER);
	
	BRect IconRect(0,0,16,16);
	ptrIconView = new IconView(IconRect);
	
	topmenu->DrawItem(ptrIconView,IconRect,false);
	
	// Setup the OutlineView 
	outlinesv->SetBorderHighlighted(true);
	
	outline->SetHighColor(0,0,0,0);
	outline->SetLowColor(255,255,255,0);
	
	BFont OutlineFont;
	OutlineFont.SetSpacing(B_CHAR_SPACING);
	OutlineFont.SetSize(12);
	outline->SetFont(&OutlineFont,B_FONT_SHEAR | B_FONT_SPACING);
	
	// Create the Views	
	AddChild(ptrMediaView = new MediaView(r));
	
	// Add Child(ren)
	ptrMediaView->AddChild(outlinesv);
	//AddChild(ptrAudioSettingsView = mew AudioSettingsView(AvailableRect));
	ptrMediaView->AddChild(ptrAudioMixerView = new AudioMixerView(AudioMixerRect));
	ptrAudioMixerView->AddChild(paramView);
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


