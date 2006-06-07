/*
 * Copyright (c) 2003-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Sikosis, Jérôme Duval
 */


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
	fCurrentNode(NULL),
	fParamWeb(NULL),
	fAlert(NULL),
	fInitCheck(B_OK)
{
	InitWindow();
}


status_t
MediaWindow::InitCheck()
{
	return fInitCheck;
}


// MediaWindow - Destructor
MediaWindow::~MediaWindow()
{
	for(int i=0; i<fAudioOutputs.CountItems(); i++)
		delete static_cast<dormant_node_info *>(fAudioOutputs.ItemAt(i));
	for(int i=0; i<fAudioInputs.CountItems(); i++)
		delete static_cast<dormant_node_info *>(fAudioInputs.ItemAt(i));
	for(int i=0; i<fVideoOutputs.CountItems(); i++)
		delete static_cast<dormant_node_info *>(fVideoOutputs.ItemAt(i));
	for(int i=0; i<fVideoInputs.CountItems(); i++)
		delete static_cast<dormant_node_info *>(fVideoInputs.ItemAt(i));
	
	BMediaRoster *roster = BMediaRoster::Roster();
	if(roster && fCurrentNode)
		roster->ReleaseNode(*fCurrentNode);
		
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
	for(int32 j=0; j<fListView->CountItems(); j++) {
		MediaListItem *item = static_cast<MediaListItem *>(fListView->ItemAt(j));
		if(item->fInfo && item->fInfo->addon == info->addon && item->fInfo->flavor_id == info->flavor_id) {
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
			fListView->AddItem(new MediaListItem(info, 1, isVideo, &fIcons));
	}
}


// MediaWindow::InitWindow -- Initialization Commands here
void 
MediaWindow::InitWindow(void)
{	
	// Bitmaps
	BRect iconRect(0,0,15,15);
  	BBitmap *icon = new BBitmap(iconRect, B_CMAP8);
  	icon->SetBits(kDevicesBits, kDevicesWidth*kDevicesHeight, 0, kDevicesColorSpace);
  	fIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kMixerBits, kMixerWidth*kMixerHeight, 0, kMixerColorSpace);
	fIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kMicBits, kMicWidth*kMicHeight, 0, kMicColorSpace);
	fIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kSpeakerBits, kSpeakerWidth*kSpeakerHeight, 0, kSpeakerColorSpace);
	fIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kCamBits, kCamWidth*kCamHeight, 0, kCamColorSpace);
	fIcons.AddItem(icon);
	icon = new BBitmap(iconRect, B_CMAP8);
	icon->SetBits(kTVBits, kTVWidth*kTVHeight, 0, kTVColorSpace);
	fIcons.AddItem(icon);


	BRect bounds = Bounds(); // the whole view

	// Create the OutlineView
	BRect menuRect(bounds.left+14,bounds.top+14,bounds.left+146,bounds.bottom-14);
	BRect titleRect(menuRect.right+14,menuRect.top,bounds.right-10,menuRect.top+16);
	BRect availableRect(menuRect.right+15,titleRect.bottom+12,bounds.right-14,bounds.bottom-4);
	BRect barRect(titleRect.left,titleRect.bottom+10,titleRect.right-2,titleRect.bottom+11);

	fListView = new BListView(menuRect, "media_list_view", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	fListView->SetSelectionMessage(new BMessage(ML_SELECTED_NODE));
			
	// Add ScrollView to Media Menu
	BScrollView *scrollView = new BScrollView("listscroller", fListView, B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM, 0, false, false, B_FANCY_BORDER);
	
	// Create the Views	
	fBox = new BBox(bounds, "background", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	
	// Add Child(ren)
	AddChild(fBox);
	fBox->AddChild(scrollView);
	
	// StringViews
	rgb_color titleFontColor = { 0,0,0,0 };	
	fTitleView = new BStringView(titleRect, "AudioSettings", "Audio Settings", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fTitleView->SetFont(be_bold_font);
	fTitleView->SetFontSize(12.0);
	fTitleView->SetHighColor(titleFontColor);
	
	fBox->AddChild(fTitleView);
	
	fContentView = new BBox(availableRect, "contentView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER);
	fBox->AddChild(fContentView);
	
	BRect settingsRect(0, 0, 442, 335);
	fAudioView = new SettingsView(settingsRect, false);
	fVideoView = new SettingsView(settingsRect, true);
	
	fBar = new BarView(barRect);
	fBox->AddChild(fBar);
	
	fInitCheck = InitMedia(true);
	if (fInitCheck != B_OK) {
		PostMessage(B_QUIT_REQUESTED);
	} else {
		if (IsHidden())
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
		
		fAlert = new MediaAlert(BRect(0,0,300,60), 
			"restart_alert", "Restarting Media Services\nStarting Media Server...\n");
		fAlert->Show();
		
		Show();
				
		launch_media_server();
	}
	
	Lock();
	
	bool isVideoSelected = true;
	if(!first && fListView->ItemAt(0) && fListView->ItemAt(0)->IsSelected())
		isVideoSelected = false;
		
	if((!first || (first && err) ) && fAlert) {
		BAutolock locker(fAlert);
		if(locker.IsLocked())
			fAlert->TextView()->SetText("Ready For Use...");
	}
	
	void *listItem;
	while((listItem = fListView->RemoveItem((int32)0)))
		delete static_cast<MediaListItem *>(listItem);
	while((listItem = fAudioOutputs.RemoveItem((int32)0)))
		delete static_cast<dormant_node_info *>(listItem);
	while((listItem = fAudioInputs.RemoveItem((int32)0)))
		delete static_cast<dormant_node_info *>(listItem);
	while((listItem = fVideoOutputs.RemoveItem((int32)0)))
		delete static_cast<dormant_node_info *>(listItem);
	while((listItem = fVideoInputs.RemoveItem((int32)0)))
		delete static_cast<dormant_node_info *>(listItem);
		
	MediaListItem    *item, *mixer, *audio, *video;

	// Grab Media Info
	FindNodes(B_MEDIA_RAW_AUDIO, B_PHYSICAL_OUTPUT, fAudioOutputs);
	FindNodes(B_MEDIA_RAW_AUDIO, B_PHYSICAL_INPUT, fAudioInputs);
	FindNodes(B_MEDIA_ENCODED_AUDIO, B_PHYSICAL_OUTPUT, fAudioOutputs);
	FindNodes(B_MEDIA_ENCODED_AUDIO, B_PHYSICAL_INPUT, fAudioInputs);
	FindNodes(B_MEDIA_RAW_VIDEO, B_PHYSICAL_OUTPUT, fVideoOutputs);
	FindNodes(B_MEDIA_RAW_VIDEO, B_PHYSICAL_INPUT, fVideoInputs);
	FindNodes(B_MEDIA_ENCODED_VIDEO, B_PHYSICAL_OUTPUT, fVideoOutputs);
	FindNodes(B_MEDIA_ENCODED_VIDEO, B_PHYSICAL_INPUT, fVideoInputs);

	// Add video nodes first. They might have an additional audio
	// output or input, but still should be listed as video node.
	AddNodes(fVideoOutputs, true);
	AddNodes(fVideoInputs, true);
	AddNodes(fAudioOutputs, false);
	AddNodes(fAudioInputs, false);
	
	fAudioView->AddNodes(fAudioOutputs, false);
	fAudioView->AddNodes(fAudioInputs, true);
	fVideoView->AddNodes(fVideoOutputs, false);
	fVideoView->AddNodes(fVideoInputs, true);
	
	fListView->AddItem(audio = new MediaListItem("Audio Settings", 0, false, &fIcons));
	fListView->AddItem(video = new MediaListItem("Video Settings", 0, true, &fIcons));
	
	fListView->AddItem(mixer = new MediaListItem("Audio Mixer", 1, false, &fIcons));
	mixer->SetAudioMixer(true);		
		
	fListView->SortItems(&MediaListItem::Compare);
	
	media_node default_node;
	dormant_node_info node_info;
	int32 outputID;
	BString outputName;
		
	if(roster->GetAudioInput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, true);
		fAudioView->SetDefault(node_info, true);
	}
		
	if(roster->GetAudioOutput(&default_node, &outputID, &outputName)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, false);
		fAudioView->SetDefault(node_info, false, outputID);
	}
	
	if(roster->GetVideoInput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, true);
		fVideoView->SetDefault(node_info, true);
	}
	
	if(roster->GetVideoOutput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		item = FindMediaListItem(&node_info);
		if(item)
			item->SetDefault(true, false);
		fVideoView->SetDefault(node_info, false);
	}
	
	if(first) {
		fListView->Select(fListView->IndexOf(mixer));
	} else {
		if(!fAudioView->fRestartView->IsHidden())
			fAudioView->fRestartView->Hide();
		if(!fVideoView->fRestartView->IsHidden())
			fVideoView->fRestartView->Hide();
	
		if(isVideoSelected)
			fListView->Select(fListView->IndexOf(video));
		else
			fListView->Select(fListView->IndexOf(audio));
	}
	
	if(fAlert) {
		snooze(800000);
		fAlert->PostMessage(B_QUIT_REQUESTED);
	}
	fAlert = NULL;
	
	Unlock();
	
	return B_OK;
}


// MediaWindow::QuitRequested -- Post a message to the app to quit
bool
MediaWindow::QuitRequested()
{
	// stop watching the MediaRoster
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster && fCurrentNode)	{
		roster->StopWatching(this, *fCurrentNode, B_MEDIA_WILDCARD);
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
				Settings2Item *item = static_cast<Settings2Item *>(fAudioView->fMenu3->ItemAt(index));
				
				if(item) {
					BMediaRoster *roster = BMediaRoster::Roster();
					roster->SetAudioOutput(*item->fInput);
					
					if(fAudioView->fRestartView->IsHidden())
						fAudioView->fRestartView->Show();	
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
				SettingsView *settingsView = isVideo ? fVideoView : fAudioView;
				BMenu *menu = isInput ? settingsView->fMenu1 : settingsView->fMenu2;
				SettingsItem *item = static_cast<SettingsItem *>(menu->ItemAt(index));
				
				if(item) {
					PRINT(("isVideo %i isInput %i\n", isVideo, isInput));
					BMediaRoster *roster = BMediaRoster::Roster();
					if(isVideo) {
						if(isInput)
							roster->SetVideoInput(*item->fInfo);
						else
							roster->SetVideoOutput(*item->fInfo);
					} else {
						if(isInput)
							roster->SetAudioInput(*item->fInfo);
						else {
							roster->SetAudioOutput(*item->fInfo);
							fAudioView->SetDefault(*item->fInfo, false, 0);
						}
					}
					
					MediaListItem *oldListItem = NULL;
					for(int32 j=0; j<fListView->CountItems(); j++) {
						oldListItem = static_cast<MediaListItem *>(fListView->ItemAt(j));
						if(oldListItem->fInfo && oldListItem->IsVideo() == isVideo 
							&& oldListItem->IsDefault(isInput))
							break;
					}
					if(oldListItem)
						oldListItem->SetDefault(false, isInput);
					else
						fprintf(stderr, "oldListItem not found\n");
					
					MediaListItem *listItem = FindMediaListItem(item->fInfo);
					if(listItem) {
						listItem->SetDefault(true, isInput);
					} else 
						fprintf(stderr, "MediaListItem not found\n");
					fListView->Invalidate();
					
					if(settingsView->fRestartView->IsHidden())
						settingsView->fRestartView->Show();
				} else 
					fprintf(stderr, "SettingsItem not found\n");			
			}
			break;
		case ML_RESTART_MEDIA_SERVER:
		{
			thread_id thread = spawn_thread(&MediaWindow::RestartMediaServices,
				"restart_thread", B_NORMAL_PRIORITY, this);
			if (thread < B_OK)
				fprintf(stderr, "couldn't create restart thread\n");
			else
				resume_thread(thread);
			break;
		}
		case ML_SHOW_VOLUME_CONTROL:
		{
			BDeskbar deskbar;
			if (fAudioView->fVolumeCheckBox->Value() == B_CONTROL_ON) {
				BEntry entry("/bin/desklink", true);
				int32 id;
				entry_ref ref;
				status_t status = entry.GetRef(&ref);
				if (status == B_OK)
					status = deskbar.AddItem(&ref, &id);

				if (status != B_OK) {
					fprintf(stderr, "Couldn't add Volume control in Deskbar: %s\n",
						strerror(status));
				}
			} else {
				status_t status = deskbar.RemoveItem("MediaReplicant");
				if (status != B_OK) {
					fprintf(stderr, "Couldn't remove Volume control in Deskbar: %s\n",
						strerror(status));
				}
			}
			break;
		}
		case ML_ENABLE_REAL_TIME:
			{
				bool isVideo = true;
				if(message->FindBool("isVideo", &isVideo)!=B_OK)
					break;
				SettingsView *settingsView = isVideo ? fVideoView : fAudioView;
				uint32 flags;
				uint32 realtimeFlag = isVideo ? B_MEDIA_REALTIME_VIDEO : B_MEDIA_REALTIME_AUDIO;
				BMediaRoster *roster = BMediaRoster::Roster();
				roster->GetRealtimeFlags(&flags);
				if(settingsView->fRealtimeCheckBox->Value()==B_CONTROL_ON)
					flags |= realtimeFlag;
				else
					flags &= ~realtimeFlag;
				roster->SetRealtimeFlags(flags);
			}
			break;
		case B_MEDIA_WEB_CHANGED: 
		case ML_SELECTED_NODE:
			{
				PRINT_OBJECT(*message);
				
				MediaListItem *item = static_cast<MediaListItem *>(fListView->ItemAt(fListView->CurrentSelection()));
				if(!item)
					break;
				BMediaRoster* roster = BMediaRoster::Roster();
				if(fCurrentNode) {
					// stop watching the MediaRoster
					roster->StopWatching(this, *fCurrentNode, B_MEDIA_WILDCARD);
					roster->ReleaseNode(*fCurrentNode);
				}
				fCurrentNode = NULL;
				BView *paramView = fContentView->ChildAt(0);
				if(paramView!=NULL) {
					fContentView->RemoveChild(paramView);
				}
				paramView = NULL;
				if(fParamWeb)
					delete fParamWeb;
				fParamWeb = NULL;
									
				fTitleView->SetText(item->GetLabel());
								
				if(item->OutlineLevel() == 0) {
					if(item->IsVideo())
						fContentView->AddChild(fVideoView);
					else
						fContentView->AddChild(fAudioView);
				} else {
				
					if(!fCurrentNode)
						fCurrentNode = new media_node();
					media_node_id node_id;
					if(item->IsAudioMixer())
						roster->GetAudioMixer(fCurrentNode);
					else if(roster->GetInstancesFor(item->fInfo->addon, item->fInfo->flavor_id, &node_id)!=B_OK) 
						roster->InstantiateDormantNode(*(item->fInfo), fCurrentNode, B_FLAVOR_IS_GLOBAL);
					else
						roster->GetNodeFor(node_id, fCurrentNode);
					
					
					if(roster->GetParameterWebFor(*fCurrentNode, &fParamWeb)==B_OK) {
						BMediaTheme* theme = BMediaTheme::PreferredTheme();
						paramView = theme->ViewFor(fParamWeb);
						fContentView->AddChild(paramView);
						paramView->ResizeTo(fContentView->Bounds().Width(), fContentView->Bounds().Height() - 10);
						
						roster->StartWatching(this, *fCurrentNode, B_MEDIA_WILDCARD);				
					} else {
						fParamWeb = NULL;
						BRect bounds = fContentView->Bounds();
						BStringView* stringView = new BStringView(bounds, 
							"noControls", "This hardware has no controls.", B_FOLLOW_V_CENTER | B_FOLLOW_H_CENTER);
						stringView->ResizeToPreferred();
						fContentView->AddChild(stringView);
						stringView->MoveBy((bounds.Width()-stringView->Bounds().Width())/2, 
							(bounds.Height()-stringView->Bounds().Height())/2);
					}
				}
				
				bool barChanged = (item->OutlineLevel() == 0 || fParamWeb == NULL || fParamWeb->CountGroups()<2);
				if(barChanged != fBar->fDisplay) {
					fBar->fDisplay = barChanged;
					fBar->Invalidate();
				}
			}
			break;
		case B_SOME_APP_LAUNCHED:
			{
				PRINT_OBJECT(*message);
				const char *mimeSig;
				if(fAlert && message->FindString("be:signature", &mimeSig)==B_OK
					&& (strcmp(mimeSig, "application/x-vnd.Be.addon-host")==0 
						|| strcmp(mimeSig, "application/x-vnd.Be.media-server")==0)) {
					fAlert->Lock();
					fAlert->TextView()->SetText("Starting Media Server...");
					fAlert->Unlock();
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
	window->fAlert = new MediaAlert(BRect(0,0,300,60), 
		"restart_alert", "Restarting Media Services\nShutting down Media Server\n");
					
	window->fAlert->Show();
	
	shutdown_media_server(B_INFINITE_TIMEOUT, MediaWindow::UpdateProgress, window->fAlert);
	
	{
		BAutolock locker(window->fAlert);
		if(locker.IsLocked())
			window->fAlert->TextView()->SetText("Starting Media Server...");
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

