// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaWindow.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

// Includes -------------------------------------------------------------------------------------------------- //
#include <MediaTheme.h>
#include <MediaRoster.h>
#include <Alert.h>
#include <ScrollView.h>
#include <Bitmap.h>
#include <stdio.h>
#include <Screen.h>
#include <Application.h>
#include <StorageKit.h>
#include <Deskbar.h>
#include <Button.h>
#include <TextView.h>
#include <Roster.h>
#include <String.h>
#include <Debug.h>
#include <Autolock.h>
#include "MediaWindow.h"

// Images
#include "iconfile.h"

const uint32 ML_SELECTED_NODE = 'MlSN';
const uint32 ML_INIT_MEDIA = 'MlIM';

// MediaWindow - Constructor
MediaWindow::MediaWindow(BRect frame) 
: BWindow (frame, "Media", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	mCurrentNode(NULL),
	mParamWeb(NULL),
	mAlert(NULL)
{
	InitWindow();
}


// MediaWindow - Destructor
MediaWindow::~MediaWindow()
{
	for(int i=0; i<mAudioOutputs.CountItems(); i++)
		delete mAudioOutputs.ItemAt(i);
	for(int i=0; i<mAudioInputs.CountItems(); i++)
		delete mAudioInputs.ItemAt(i);
	for(int i=0; i<mVideoOutputs.CountItems(); i++)
		delete mVideoOutputs.ItemAt(i);
	for(int i=0; i<mVideoInputs.CountItems(); i++)
		delete mVideoInputs.ItemAt(i);
	
	BMediaRoster *roster = BMediaRoster::Roster();
	if(roster && mCurrentNode)
		roster->ReleaseNode(*mCurrentNode);
		
	char buffer[512];
	BRect rect = Frame();
	PRINT_OBJECT(rect);
	sprintf(buffer, "# MediaPrefs Settings\n rect = %i,%i,%i,%i\n", int(rect.left), int(rect.top), int(rect.right), int(rect.bottom));
			
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
		if(file.InitCheck()==B_OK) {
			file.Write(buffer, strlen(buffer));
		}
	}
}

void
MediaWindow::FindNodes(media_type type, uint64 kind, BList &list)
{
	dormant_node_info node_info[64];
	int32 node_info_count = 64;
	media_format format;
	media_format *format1 = NULL, *format2 = NULL;
	BMediaRoster *roster = BMediaRoster::Roster();
	format.type = type;
	
	if(kind & B_PHYSICAL_OUTPUT)
		format1 = &format;
	else if(kind & B_PHYSICAL_INPUT)
		format2 = &format;
	else 
		return;
		
	if(roster->GetDormantNodes(node_info, &node_info_count, format1, format2, NULL, kind)!=B_OK) {
		fprintf(stderr, "error\n");
		return;
	}
	
	for(int32 i=0; i<node_info_count; i++) {
		PRINT(("node : %s, media_addon %i, flavor_id %i\n", 
			node_info[i].name, node_info[i].addon, node_info[i].flavor_id));
		dormant_node_info *info = new dormant_node_info();
		strcpy(info->name, node_info[i].name);
		info->flavor_id = node_info[i].flavor_id;
		info->addon = node_info[i].addon;
		list.AddItem(info);
	}
}

MediaListItem *
MediaWindow::FindMediaListItem(dormant_node_info *info)
{
	for(int32 j=0; j<mListView->CountItems(); j++) {
		MediaListItem *item = static_cast<MediaListItem *>(mListView->ItemAt(j));
		if(item->mInfo && item->mInfo->addon == info->addon && item->mInfo->flavor_id == info->flavor_id) {
			return item;
			break;
		}
	}
	return NULL;
}

void
MediaWindow::AddNodes(BList &list, bool isVideo)
{
	for(int32 i=0; i<list.CountItems(); i++) {
		dormant_node_info *info = static_cast<dormant_node_info *>(list.ItemAt(i));
		if(!FindMediaListItem(info))
			mListView->AddItem(new MediaListItem(info, 1, isVideo, &mIcons));
	}
}

// MediaWindow::InitWindow -- Initialization Commands here
void MediaWindow::InitWindow(void)
{	
	// Bitmaps
	BRect iconRect(0,0,15,15);
  	BBitmap *icon = new BBitmap(iconRect, B_CMAP8);
  	icon->SetBits(kDevicesBits, kDevicesWidth*kDevicesHeight, 0, kDevicesColorSpace);
  	mIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kMixerBits, kMixerWidth*kMixerHeight, 0, kMixerColorSpace);
	mIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kMicBits, kMicWidth*kMicHeight, 0, kMicColorSpace);
	mIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kSpeakerBits, kSpeakerWidth*kSpeakerHeight, 0, kSpeakerColorSpace);
	mIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kCamBits, kCamWidth*kCamHeight, 0, kCamColorSpace);
	mIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kTVBits, kTVWidth*kTVHeight, 0, kTVColorSpace);
	mIcons.AddItem(icon);


	BRect bounds = Bounds(); // the whole view

	// Create the OutlineView
	BRect menuRect(bounds.left+14,bounds.top+14,bounds.left+146,bounds.bottom-14);
	BRect titleRect(menuRect.right+14,menuRect.top,bounds.right-10,menuRect.top+16);
	BRect availableRect(menuRect.right+15,titleRect.bottom+12,bounds.right-14,bounds.bottom-14);
	BRect barRect(titleRect.left,titleRect.bottom+10,titleRect.right-2,titleRect.bottom+11);

	mListView = new BListView(menuRect,"audio_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	mListView->SetSelectionMessage(new BMessage(ML_SELECTED_NODE));
			
	// Add ScrollView to Media Menu
	BScrollView *scrollView = new BScrollView("scroll_audio", mListView, B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM, 0, false, false, B_FANCY_BORDER);
	
	// Create the Views	
	mBox = new BBox(bounds, "mediaView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	
	// Add Child(ren)
	AddChild(mBox);
	mBox->AddChild(scrollView);
	
	// StringViews
	rgb_color titleFontColor = { 0,0,0,0 };	
	mTitleView = new BStringView(titleRect, "AudioSettings", "Audio Settings", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	mTitleView->SetFont(be_bold_font);
	mTitleView->SetFontSize(12.0);
	mTitleView->SetHighColor(titleFontColor);
	
	mBox->AddChild(mTitleView);
	
	mContentView = new BBox(availableRect, "contentView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER);
	mBox->AddChild(mContentView);
	
	BRect settingsRect(0, 0, 442, 330);
	mAudioView = new SettingsView(settingsRect, false);
	mVideoView = new SettingsView(settingsRect, true);
	
	mBar = new BarView(barRect);
	mBox->AddChild(mBar);
		
	if(InitMedia(true)!=B_OK)
		PostMessage(B_QUIT_REQUESTED);
	else {
		if(IsHidden())
			Show();
	}
}
// ---------------------------------------------------------------------------------------------------------- //

status_t
MediaWindow::InitMedia(bool first)
{
	status_t err = B_OK;
	BMediaRoster *roster = BMediaRoster::Roster(&err);
	
	if(first && err != B_OK) {
		BAlert *alert = new BAlert("start_media_server", 
			"Could not connect to the Media Server.\n"
			"Would you like to start it ?", "Quit", "Start Media Server", NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if(alert->Go()==0)
			return B_ERROR;
		
		mAlert = new MediaAlert(BRect(0,0,300,60), 
			"restart_alert", "Restarting Media Services\nStarting Media Server...\n");
		mAlert->Show();
		
		Show();
				
		launch_media_server();
	}
	
	Lock();
	
	bool isVideoSelected = true;
	if(!first && mListView->ItemAt(0) && mListView->ItemAt(0)->IsSelected())
		isVideoSelected = false;
		
	if((!first || (first && err) ) && mAlert) {
		BAutolock locker(mAlert);
		if(locker.IsLocked())
			mAlert->TextView()->SetText("Ready For Use...");
	}
	
	void *listItem;
	while((listItem = mListView->RemoveItem((int32)0)))
		delete listItem;
	while((listItem = mAudioOutputs.RemoveItem((int32)0)))
		delete listItem;
	while((listItem = mAudioInputs.RemoveItem((int32)0)))
		delete listItem;
	while((listItem = mVideoOutputs.RemoveItem((int32)0)))
		delete listItem;
	while((listItem = mVideoInputs.RemoveItem((int32)0)))
		delete listItem;
		
	MediaListItem    *item, *mixer, *audio, *video;

	// Grab Media Info
	FindNodes(B_MEDIA_RAW_AUDIO, B_PHYSICAL_OUTPUT, mAudioOutputs);
	FindNodes(B_MEDIA_RAW_AUDIO, B_PHYSICAL_INPUT, mAudioInputs);
	FindNodes(B_MEDIA_ENCODED_AUDIO, B_PHYSICAL_OUTPUT, mAudioOutputs);
	FindNodes(B_MEDIA_ENCODED_AUDIO, B_PHYSICAL_INPUT, mAudioInputs);
	FindNodes(B_MEDIA_RAW_VIDEO, B_PHYSICAL_OUTPUT, mVideoOutputs);
	FindNodes(B_MEDIA_RAW_VIDEO, B_PHYSICAL_INPUT, mVideoInputs);
	FindNodes(B_MEDIA_ENCODED_VIDEO, B_PHYSICAL_OUTPUT, mVideoOutputs);
	FindNodes(B_MEDIA_ENCODED_VIDEO, B_PHYSICAL_INPUT, mVideoInputs);
	
	AddNodes(mAudioOutputs, false);
	AddNodes(mAudioInputs, false);
	AddNodes(mVideoOutputs, true);
	AddNodes(mVideoInputs, true);
	
	mAudioView->AddNodes(mAudioOutputs, false);
	mAudioView->AddNodes(mAudioInputs, true);
	mVideoView->AddNodes(mVideoOutputs, false);
	mVideoView->AddNodes(mVideoInputs, true);
	
	mListView->AddItem(audio = new MediaListItem("Audio Settings", 0, false, &mIcons));
	mListView->AddItem(video = new MediaListItem("Video Settings", 0, true, &mIcons));
	
	mListView->AddItem(mixer = new MediaListItem("Audio Mixer", 1, false, &mIcons));
	mixer->SetAudioMixer(true);		
		
	mListView->SortItems(&MediaListItem::Compare);
	
	media_node default_node;
	dormant_node_info node_info;
	int32 outputID;
	BString outputName;
		
	if(roster->GetAudioInput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, true);
		mAudioView->SetDefault(node_info, true);
	}
		
	if(roster->GetAudioOutput(&default_node, &outputID, &outputName)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, false);
		mAudioView->SetDefault(node_info, false, outputID);
	}
	
	if(roster->GetVideoInput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, true);
		mVideoView->SetDefault(node_info, true);
	}
	
	if(roster->GetVideoOutput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, false);
		mVideoView->SetDefault(node_info, false);
	}
	
	if(first) {
		mListView->Select(mListView->IndexOf(mixer));
	} else {
		if(!mAudioView->mRestartTextView->IsHidden())
			mAudioView->mRestartTextView->Hide();
		if(!mVideoView->mRestartTextView->IsHidden())
			mVideoView->mRestartTextView->Hide();
	
		if(isVideoSelected)
			mListView->Select(mListView->IndexOf(video));
		else
			mListView->Select(mListView->IndexOf(audio));
	}
	
	if(mAlert) {
		snooze(800000);
		mAlert->PostMessage(B_QUIT_REQUESTED);
	}
	mAlert = NULL;
	
	Unlock();
	
	return B_OK;
}

// MediaWindow::QuitRequested -- Post a message to the app to quit
bool
MediaWindow::QuitRequested()
{
	// stop watching the MediaRoster
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster && mCurrentNode)	{
		roster->StopWatching(this, *mCurrentNode, B_MEDIA_WILDCARD);
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow::FrameResized -- When the main frame is resized fix up the other views
void 
MediaWindow::FrameResized (float width, float height)
{
	// This makes sure our SideBar colours down to the bottom when resized
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //

// ErrorAlert -- Displays a BAlert Box with a Custom Error or Debug Message
void 
ErrorAlert(char* errorMessage) {
	printf("%s\n", errorMessage);
	BAlert *alert = new BAlert("BAlert", errorMessage, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT); 
	alert->Go();
	exit(1);
}
// ---------------------------------------------------------------------------------------------------------- //


// MediaWindow::MessageReceived -- receives messages
void 
MediaWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{	
		case ML_INIT_MEDIA:
			InitMedia(false);
			break;
		case ML_DEFAULTOUTPUT_CHANGE:
			{
				int32 index;
				if(message->FindInt32("index", &index)!=B_OK)
					break;
				Settings2Item *item = static_cast<Settings2Item *>(mAudioView->mMenu3->ItemAt(index));
				
				if(item) {
					BMediaRoster *roster = BMediaRoster::Roster();
					roster->SetAudioOutput(*item->mInput);
					
					if(mAudioView->mRestartTextView->IsHidden())
						mAudioView->mRestartTextView->Show();	
				} else 
					fprintf(stderr, "Settings2Item not found\n");
			}
			break;		
		case ML_DEFAULT_CHANGE:
			{
				bool isVideo = true;
				bool isInput = true;
				if(message->FindBool("isVideo", &isVideo)!=B_OK)
					break;
				if(message->FindBool("isInput", &isInput)!=B_OK)
					break;
				int32 index;
				if(message->FindInt32("index", &index)!=B_OK)
					break;
				SettingsView *settingsView = isVideo ? mVideoView : mAudioView;
				BMenu *menu = isInput ? settingsView->mMenu1 : settingsView->mMenu2;
				SettingsItem *item = static_cast<SettingsItem *>(menu->ItemAt(index));
				
				if(item) {
					PRINT(("isVideo %i isInput %i\n", isVideo, isInput));
					BMediaRoster *roster = BMediaRoster::Roster();
					if(isVideo) {
						if(isInput)
							roster->SetVideoInput(*item->mInfo);
						else
							roster->SetVideoOutput(*item->mInfo);
					} else {
						if(isInput)
							roster->SetAudioInput(*item->mInfo);
						else {
							roster->SetAudioOutput(*item->mInfo);
							mAudioView->SetDefault(*item->mInfo, false, 0);
						}
					}
					
					MediaListItem *oldListItem = NULL;
					for(int32 j=0; j<mListView->CountItems(); j++) {
						oldListItem = static_cast<MediaListItem *>(mListView->ItemAt(j));
						if(oldListItem->mInfo && oldListItem->IsVideo() == isVideo 
							&& oldListItem->IsDefault(isInput))
							break;
					}
					if(oldListItem)
						oldListItem->SetDefault(false, isInput);
					else
						fprintf(stderr, "oldListItem not found\n");
					
					MediaListItem *listItem = FindMediaListItem(item->mInfo);
					if(listItem) {
						listItem->SetDefault(true, isInput);
					} else 
						fprintf(stderr, "MediaListItem not found\n");
					mListView->Invalidate();
					
					if(settingsView->mRestartTextView->IsHidden())
						settingsView->mRestartTextView->Show();
				} else 
					fprintf(stderr, "SettingsItem not found\n");			
			}
			break;
		case ML_RESTART_MEDIA_SERVER:
			{
				thread_id tid;
				tid = spawn_thread(&MediaWindow::RestartMediaServices, "restart_thread", B_NORMAL_PRIORITY, this);
				if(tid<B_OK)
					fprintf(stderr, "couldn't create restart thread\n");
				resume_thread(tid);
			}
			break;
		case ML_SHOW_VOLUME_CONTROL:
			{
				BDeskbar deskbar;
				if(mAudioView->mVolumeCheckBox->Value()==B_CONTROL_ON) {
					BEntry entry("/bin/desklink", true);
					int32 id;
					entry_ref ref;
					entry.GetRef(&ref);
					if(deskbar.AddItem(&ref, &id)!=B_OK)
						fprintf(stderr, "Couldn't add Volume control in Deskbar\n");
				} else {
					if(deskbar.RemoveItem("MediaReplicant")!=B_OK)
						fprintf(stderr, "Couldn't remove Volume control in Deskbar\n");
				}				
			}
			break;
		case ML_ENABLE_REAL_TIME:
			{
				bool isVideo = true;
				if(message->FindBool("isVideo", &isVideo)!=B_OK)
					break;
				SettingsView *settingsView = isVideo ? mVideoView : mAudioView;
				uint32 flags;
				uint32 realtimeFlag = isVideo ? B_MEDIA_REALTIME_VIDEO : B_MEDIA_REALTIME_AUDIO;
				BMediaRoster *roster = BMediaRoster::Roster();
				roster->GetRealtimeFlags(&flags);
				if(settingsView->mRealtimeCheckBox->Value()==B_CONTROL_ON)
					flags |= realtimeFlag;
				else
					flags &= !realtimeFlag;
				roster->SetRealtimeFlags(flags);
			}
			break;
		case B_MEDIA_WEB_CHANGED: 
		case ML_SELECTED_NODE:
			{
				PRINT_OBJECT(*message);
				
				MediaListItem *item = static_cast<MediaListItem *>(mListView->ItemAt(mListView->CurrentSelection()));
				if(!item)
					break;
				BMediaRoster* roster = BMediaRoster::Roster();
				if(mCurrentNode) {
					// stop watching the MediaRoster
					roster->StopWatching(this, *mCurrentNode, B_MEDIA_WILDCARD);
					roster->ReleaseNode(*mCurrentNode);
				}
				mCurrentNode = NULL;
				BView *paramView = mContentView->ChildAt(0);
				if(paramView!=NULL) {
					mContentView->RemoveChild(paramView);
				}
				paramView = NULL;
				if(mParamWeb)
					delete mParamWeb;
				mParamWeb = NULL;
									
				mTitleView->SetText(item->GetLabel());
								
				if(item->OutlineLevel() == 0) {
					if(item->IsVideo())
						mContentView->AddChild(mVideoView);
					else
						mContentView->AddChild(mAudioView);
				} else {
				
					if(!mCurrentNode)
						mCurrentNode = new media_node();
					media_node_id node_id;
					if(item->IsAudioMixer())
						roster->GetAudioMixer(mCurrentNode);
					else if(roster->GetInstancesFor(item->mInfo->addon, item->mInfo->flavor_id, &node_id)!=B_OK) 
						roster->InstantiateDormantNode(*(item->mInfo), mCurrentNode, B_FLAVOR_IS_GLOBAL);
					else
						roster->GetNodeFor(node_id, mCurrentNode);
					
					
					if(roster->GetParameterWebFor(*mCurrentNode, &mParamWeb)==B_OK) {
						BMediaTheme* theme = BMediaTheme::PreferredTheme();
						paramView = theme->ViewFor(mParamWeb);
						mContentView->AddChild(paramView);
						paramView->ResizeTo(mContentView->Bounds().Width(), mContentView->Bounds().Height());
						
						roster->StartWatching(this, *mCurrentNode, B_MEDIA_WILDCARD);				
					} else {
						mParamWeb = NULL;
						BRect bounds = mContentView->Bounds();
						BStringView* stringView = new BStringView(bounds, 
							"noControls", "This hardware has no controls.", B_FOLLOW_V_CENTER | B_FOLLOW_H_CENTER);
						stringView->ResizeToPreferred();
						mContentView->AddChild(stringView);
						stringView->MoveBy((bounds.Width()-stringView->Bounds().Width())/2, 
							(bounds.Height()-stringView->Bounds().Height())/2);
					}
				}
				
				bool barChanged = (item->OutlineLevel() == 0 || mParamWeb == NULL || mParamWeb->CountGroups()<2);
				if(barChanged != mBar->mDisplay) {
					mBar->mDisplay = barChanged;
					mBar->Invalidate();
				}
			}
			break;
		case B_SOME_APP_LAUNCHED:
			{
				PRINT_OBJECT(*message);
				const char *mimeSig;
				if(mAlert && message->FindString("be:signature", &mimeSig)==B_OK
					&& (strcmp(mimeSig, "application/x-vnd.Be.addon-host")==0 
						|| strcmp(mimeSig, "application/x-vnd.Be.media-server")==0)) {
					mAlert->Lock();
					mAlert->TextView()->SetText("Starting Media Server...");
					mAlert->Unlock();
				}
			}			
			break;
		case B_SOME_APP_QUIT:
			{
				PRINT_OBJECT(*message);
				const char *mimeSig;
				if(message->FindString("be:signature", &mimeSig)==B_OK) {
					if(strcmp(mimeSig, "application/x-vnd.Be.addon-host")==0 
						|| strcmp(mimeSig, "application/x-vnd.Be.media-server")==0) {
						BMediaRoster* roster = BMediaRoster::CurrentRoster();
						if(roster&&roster->Lock())
							roster->Quit();
					}
				}
				
			}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

status_t
MediaWindow::RestartMediaServices(void *data) 
{
	MediaWindow *window = (MediaWindow *)data;
	window->mAlert = new MediaAlert(BRect(0,0,300,60), 
		"restart_alert", "Restarting Media Services\nShutting down Media Server\n");
					
	window->mAlert->Show();
	
	shutdown_media_server(B_INFINITE_TIMEOUT, MediaWindow::UpdateProgress, window->mAlert);
	
	{
		BAutolock locker(window->mAlert);
		if(locker.IsLocked())
			window->mAlert->TextView()->SetText("Starting Media Server...");
	}
	launch_media_server();
	
	return window->PostMessage(ML_INIT_MEDIA);
}

bool 
MediaWindow::UpdateProgress(int stage, const char * message, void * cookie)
{
	MediaAlert *alert = static_cast<MediaAlert*>(cookie);
	PRINT(("stage : %i\n", stage));
	char *string = "Unknown stage"; 
	switch (stage) {
		case 10:
			string = "Stopping Media Server...";
			break;
		case 20:
			string = "Telling media_addon_server to quit.";
			break;
		case 40:
			string = "Waiting for media_server to quit.";
			break;
		case 70:
			string = "Cleaning Up.";
			break;
		case 100:
			string = "Done Shutting Down.";
			break;
	}
	
	BAutolock locker(alert);
	if (locker.IsLocked())
		alert->TextView()->SetText(string);
	return true;
}

