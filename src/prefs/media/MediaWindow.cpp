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
#include "MediaListItem.h"

// Images
#include "devices.h"
#include "mixer.h"
#include "tv.h"

const uint32 ML_AUDIO_SETTINGS = 'MlAS';
const uint32 ML_AUDIO_MIXER = 'MlAM';
const uint32 NEW_MSG = 'nmsg';

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
	BRect TitleRect(0,0,140,16);
	BRect AudioMixerRect(0,30,470,400);
	
	BOutlineListView *outline;
	//MediaListItem    *topmenu;
	//MediaListItem    *submenu;
	BListItem        *topmenu;
	BListItem        *submenu;
	BScrollView      *outlinesv;
	BStringView      *stvAudioSettings;
	BStringView      *stvAudioMixer;

	outline = new BOutlineListView(outlinerect,"audio_list", B_SINGLE_SELECTION_LIST);
	outline->AddItem(topmenu = new BStringItem("Audio Settings"));
	outline->AddUnder(submenu = new BStringItem("Audio Mixer"), topmenu);
	outline->AddUnder(new BStringItem("None Out"), submenu);
	outline->AddUnder(new BStringItem("None In"), submenu);
	outline->AddItem(topmenu = new BStringItem("Video Settings"));
	outline->AddUnder(submenu = new BStringItem("Video Window Consumer"), topmenu);
	
	// Bitmaps
	BRect BitmapFrame(0,0,15,15);
  	BBitmap *DevicesIcon = new BBitmap(BitmapFrame,B_RGB32);
	DevicesIcon->SetBits(devicesicon,768,0,B_RGB32);
	BBitmap *MixerIcon = new BBitmap(BitmapFrame,B_RGB32);
	MixerIcon->SetBits(mixericon,768,0,B_RGB32);
	BBitmap *TVIcon = new BBitmap(BitmapFrame,B_RGB32);
	TVIcon->SetBits(tvicon,768,0,B_RGB32);
	
	/*outline->AddItem(topmenu = new MediaListItem("Audio Settings", MediaListItem::AS, DevicesIcon, new BMessage(ML_AUDIO_SETTINGS)));
	outline->AddItem(submenu = new MediaListItem("Audio Mixer", MediaListItem::AM, MixerIcon, new BMessage(ML_AUDIO_MIXER)));
	outline->AddUnder(new BStringItem("None Out"), submenu);
	outline->AddUnder(new BStringItem("None In"), submenu);
	outline->AddItem(topmenu = new MediaListItem("Video Settings", MediaListItem::VS, DevicesIcon, new BMessage(NEW_MSG)));
	outline->AddItem(submenu = new MediaListItem("Video Window Consumer", MediaListItem::VWC, TVIcon, new BMessage(NEW_MSG)));*/
	
	topmenu->SetHeight(18);
	submenu->SetHeight(18);
			
	// Add ScrollView to Media Menu
	outlinesv = new BScrollView("scroll_audio", outline, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, false, B_FANCY_BORDER);
	
	// StringViews
	rgb_color TitleFontColor = { 0,0,0,0 };	
	stvAudioSettings = new BStringView(TitleRect, "AudioSettings", "Audio Settings", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	stvAudioSettings->SetFont(be_bold_font);
	stvAudioSettings->SetFontSize(12.0);
	stvAudioSettings->SetHighColor(TitleFontColor);
	stvAudioMixer = new BStringView(TitleRect, "AudioMixer", "Audio Mixer", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	stvAudioMixer->SetFont(be_bold_font);
	stvAudioMixer->SetFontSize(12.0);
	stvAudioMixer->SetHighColor(TitleFontColor);
	
	// Setup the OutlineView 
	outline->AllAttached();
	outlinesv->SetBorderHighlighted(true);
	
	// Back and Fore ground Colours
	outline->SetHighColor(0,0,0,0);
	outline->SetLowColor(255,255,255,0);
	
	// Font Settings
	BFont OutlineFont;
	OutlineFont.SetSpacing(B_CHAR_SPACING);
	OutlineFont.SetSize(12);
	outline->SetFont(&OutlineFont,B_FONT_SHEAR | B_FONT_SPACING);
	
	// Create the Views	
	AddChild(ptrMediaView = new MediaView(r));
	
	// Add Child(ren)
	ptrMediaView->AddChild(outlinesv);
	ptrMediaView->AddChild(ptrAvailableViewArea = new AvailableViewArea(AvailableRect));
	ptrAvailableViewArea->AddChild(ptrAudioMixerView = new AudioMixerView(AudioMixerRect));
	
	// Add Child(ren) - Audio Settings View
	ptrAvailableViewArea->AddChild(stvAudioSettings);
	stvAudioSettings->Hide();
		
	// Add Child(ren) - Audio Mixer View
	ptrAvailableViewArea->AddChild(stvAudioMixer);
	//stvAudioMixer->Hide();
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

// ErrorAlert -- Displays a BAlert Box with a Custom Error or Debug Message
void ErrorAlert(char* errorMessage) {
	printf("%s\n", errorMessage);
	BAlert *alert = new BAlert("BAlert", errorMessage, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT); 
	alert->Go();
	exit(1);
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow::MessageReceived -- receives messages
void MediaWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{	
		case ML_AUDIO_SETTINGS:
			exit(1); // debug of course ;)
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// ---------------------------------------------------------------------------------------------------------- //