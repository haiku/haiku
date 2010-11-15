/*
 * Copyright (c) 2003-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Sikosis, Jérôme Duval
 */


#include "MediaWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <Deskbar.h>
#include <GroupView.h>
#include <Locale.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StorageKit.h>
#include <String.h>
#include <TextView.h>

#include "MediaIcons.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Media Window"


const uint32 ML_SELECTED_NODE = 'MlSN';
const uint32 ML_INIT_MEDIA = 'MlIM';


MediaWindow::SmartNode::SmartNode(const BMessenger& notifyHandler)
	:
	fNode(NULL),
	fMessenger(notifyHandler)
{
}


MediaWindow::SmartNode::~SmartNode()
{
	_FreeNode();
}


void
MediaWindow::SmartNode::SetTo(dormant_node_info* info)
{
	_FreeNode();
	if (!info)
		return;

	fNode = new media_node();

	// TODO: check error codes
	BMediaRoster* roster = BMediaRoster::Roster();

	media_node_id node_id;
	if (roster->GetInstancesFor(info->addon, info->flavor_id, &node_id) == B_OK)
		roster->GetNodeFor(node_id, fNode);
	else
		roster->InstantiateDormantNode(*info, fNode, B_FLAVOR_IS_GLOBAL);
	roster->StartWatching(fMessenger, *fNode, B_MEDIA_WILDCARD);
}


void
MediaWindow::SmartNode::SetTo(const media_node& node)
{
	_FreeNode();
	fNode = new media_node(node);
	BMediaRoster* roster = BMediaRoster::Roster();
	roster->StartWatching(fMessenger, *fNode, B_MEDIA_WILDCARD);
}


bool
MediaWindow::SmartNode::IsSet()
{
	return fNode != NULL;
}


MediaWindow::SmartNode::operator media_node()
{
	if (fNode)
		return *fNode;
	media_node node;
	return node;
}


void
MediaWindow::SmartNode::_FreeNode()
{
	if (!IsSet())
		return;
	// TODO: check error codes
	BMediaRoster* roster = BMediaRoster::Roster();
	roster->StopWatching(fMessenger, *fNode, B_MEDIA_WILDCARD);
	roster->ReleaseNode(*fNode);
	delete fNode;
	fNode = NULL;
}


// MediaWindow - Constructor
MediaWindow::MediaWindow(BRect frame)
	:
	BWindow (frame, B_TRANSLATE("Media"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fCurrentNode(BMessenger(this)),
	fParamWeb(NULL),
	fAudioInputs(5, true),
	fAudioOutputs(5, true),
	fVideoInputs(5, true),
	fVideoOutputs(5, true),
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


MediaWindow::~MediaWindow()
{
	_EmptyNodeLists();
	_ClearParamView();
	delete fVideoView;
	delete fAudioView;

	char buffer[512];
	BRect rect = Frame();
	PRINT_OBJECT(rect);
	sprintf(buffer, "# MediaPrefs Settings\n rect = %i,%i,%i,%i\n",
		int(rect.left), int(rect.top), int(rect.right), int(rect.bottom));

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(), B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
		if (file.InitCheck()==B_OK) {
			file.Write(buffer, strlen(buffer));
		}
	}
}


void
MediaWindow::SelectNode(dormant_node_info* node)
{
	fCurrentNode.SetTo(node);
	_MakeParamView();
	fTitleView->SetLabel(node->name);
}


void
MediaWindow::SelectAudioSettings(const char* title)
{
	fContentView->AddChild(fAudioView);
	fTitleView->SetLabel(title);
}


void
MediaWindow::SelectVideoSettings(const char* title)
{
	fContentView->AddChild(fVideoView);
	fTitleView->SetLabel(title);
}


void
MediaWindow::SelectAudioMixer(const char* title)
{
	media_node mixerNode;
	BMediaRoster* roster = BMediaRoster::Roster();
	roster->GetAudioMixer(&mixerNode);
	fCurrentNode.SetTo(mixerNode);
	_MakeParamView();
	fTitleView->SetLabel(title);
}


void
MediaWindow::_FindNodes()
{
	_FindNodes(B_MEDIA_RAW_AUDIO, B_PHYSICAL_OUTPUT, fAudioOutputs);
	_FindNodes(B_MEDIA_RAW_AUDIO, B_PHYSICAL_INPUT, fAudioInputs);
	_FindNodes(B_MEDIA_ENCODED_AUDIO, B_PHYSICAL_OUTPUT, fAudioOutputs);
	_FindNodes(B_MEDIA_ENCODED_AUDIO, B_PHYSICAL_INPUT, fAudioInputs);
	_FindNodes(B_MEDIA_RAW_VIDEO, B_PHYSICAL_OUTPUT, fVideoOutputs);
	_FindNodes(B_MEDIA_RAW_VIDEO, B_PHYSICAL_INPUT, fVideoInputs);
	_FindNodes(B_MEDIA_ENCODED_VIDEO, B_PHYSICAL_OUTPUT, fVideoOutputs);
	_FindNodes(B_MEDIA_ENCODED_VIDEO, B_PHYSICAL_INPUT, fVideoInputs);
}


void
MediaWindow::_FindNodes(media_type type, uint64 kind, NodeList& into)
{
	dormant_node_info node_info[64];
	int32 node_info_count = 64;

	media_format format;
	media_format* nodeInputFormat = NULL, *nodeOutputFormat = NULL;
	format.type = type;

	// output nodes must be BBufferConsumers => they have an input format
	// input nodes must be BBufferProducers => they have an output format
	if (kind & B_PHYSICAL_OUTPUT)
		nodeInputFormat = &format;
	else if (kind & B_PHYSICAL_INPUT)
		nodeOutputFormat = &format;
	else
		return;

	BMediaRoster* roster = BMediaRoster::Roster();

	if (roster->GetDormantNodes(node_info, &node_info_count, nodeInputFormat,
		nodeOutputFormat, NULL, kind) != B_OK) {
		// TODO: better error reporting!
		fprintf(stderr, "error\n");
		return;
	}

	for (int32 i = 0; i < node_info_count; i++) {
		PRINT(("node : %s, media_addon %i, flavor_id %i\n",
			node_info[i].name, node_info[i].addon, node_info[i].flavor_id));

		dormant_node_info* info = new dormant_node_info();
		strncpy(info->name, node_info[i].name, B_MEDIA_NAME_LENGTH);
		info->flavor_id = node_info[i].flavor_id;
		info->addon = node_info[i].addon;
		into.AddItem(info);
	}
}


NodeListItem*
MediaWindow::_FindNodeListItem(dormant_node_info* info)
{
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		MediaListItem* item
			= static_cast<MediaListItem*>(fListView->ItemAt(i));
		dormant_node_info* itemInfo = item->NodeInfo();
		if (itemInfo && itemInfo->addon == info->addon
				&& itemInfo->flavor_id == info->flavor_id) {
			return dynamic_cast<NodeListItem*>(item);
		}
	}
	return NULL;
}


void
MediaWindow::_AddNodeItems(NodeList &list, MediaListItem::media_type type)
{
	int32 count = list.CountItems();
	for (int32 i = 0; i < count; i++) {
		dormant_node_info* info = list.ItemAt(i);
		if (!_FindNodeListItem(info))
			fListView->AddItem(new NodeListItem(info, type));
	}
}


void
MediaWindow::_EmptyNodeLists()
{
	fAudioOutputs.MakeEmpty();
	fAudioInputs.MakeEmpty();
	fVideoOutputs.MakeEmpty();
	fVideoInputs.MakeEmpty();
}


void
MediaWindow::InitWindow()
{
	const float scrollWidth = 9 * be_plain_font->Size() + 30;

	fListView = new BListView("media_list_view");
	fListView->SetSelectionMessage(new BMessage(ML_SELECTED_NODE));

	// Add ScrollView to Media Menu
	BScrollView* scrollView = new BScrollView("listscroller",
		fListView, 0, false, false, B_FANCY_BORDER);
	scrollView->SetExplicitMinSize(BSize(scrollWidth, B_SIZE_UNSET));
	scrollView->SetExplicitMaxSize(BSize(scrollWidth, B_SIZE_UNSET));

	// Create the Views
	fBox = new BBox("background", B_WILL_DRAW | B_FRAME_EVENTS,
		B_PLAIN_BORDER);
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	GetLayout()->AddView(fBox);

	// StringViews
	fTitleView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fTitleView->SetLabel(B_TRANSLATE("Audio settings"));
	fTitleView->SetFont(be_bold_font);

	fContentView = new BBox("contentView", B_WILL_DRAW | B_FRAME_EVENTS,
		B_NO_BORDER);

	fAudioView = new SettingsView(false);
	fVideoView = new SettingsView(true);


	// Layout all views
	BGroupView* rightView = new BGroupView(B_VERTICAL, 5);
	rightView->GroupLayout()->SetInsets(14, 0, 0, 0);
	rightView->GroupLayout()->AddView(fTitleView);
	rightView->GroupLayout()->AddView(fContentView);

	BGroupLayout* rootLayout = new BGroupLayout(B_HORIZONTAL);
	rootLayout->SetInsets(14, 14, 14, 14);
	fBox->SetLayout(rootLayout);

	rootLayout->AddView(scrollView);

	rootLayout->AddView(rightView);

	// Start the window
	fInitCheck = InitMedia(true);
	if (fInitCheck != B_OK) {
		PostMessage(B_QUIT_REQUESTED);
	} else {
		if (IsHidden())
			Show();
	}
}


// #pragma mark -


status_t
MediaWindow::InitMedia(bool first)
{
	status_t err = B_OK;
	BMediaRoster* roster = BMediaRoster::Roster(&err);

	if (first && err != B_OK) {
		BAlert* alert = new BAlert("start_media_server",
			B_TRANSLATE("Could not connect to the media server.\n"
			"Would you like to start it ?"),
			B_TRANSLATE("Quit"),
			B_TRANSLATE("Start media server"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert->Go()==0)
			return B_ERROR;

		fAlert = new MediaAlert(BRect(0, 0, 300, 60),
			"restart_alert", B_TRANSLATE(
				"Restarting media services\nStarting media server"
				B_UTF8_ELLIPSIS "\n"));
		fAlert->Show();

		Show();

		launch_media_server();
	}

	Lock();

	bool isVideoSelected = true;
	if (!first && fListView->ItemAt(0) && fListView->ItemAt(0)->IsSelected())
		isVideoSelected = false;

	if ((!first || (first && err) ) && fAlert) {
		BAutolock locker(fAlert);
		if (locker.IsLocked())
			fAlert->TextView()->SetText(
				B_TRANSLATE("Ready for use" B_UTF8_ELLIPSIS));
	}

	while (fListView->CountItems() > 0)
		delete static_cast<MediaListItem*>(fListView->RemoveItem((int32)0));
	_EmptyNodeLists();

	// Grab Media Info
	_FindNodes();

	// Add video nodes first. They might have an additional audio
	// output or input, but still should be listed as video node.
	_AddNodeItems(fVideoOutputs, MediaListItem::VIDEO_TYPE);
	_AddNodeItems(fVideoInputs, MediaListItem::VIDEO_TYPE);
	_AddNodeItems(fAudioOutputs, MediaListItem::AUDIO_TYPE);
	_AddNodeItems(fAudioInputs, MediaListItem::AUDIO_TYPE);

	fAudioView->AddNodes(fAudioOutputs, false);
	fAudioView->AddNodes(fAudioInputs, true);
	fVideoView->AddNodes(fVideoOutputs, false);
	fVideoView->AddNodes(fVideoInputs, true);

	// build our list view
	DeviceListItem* audio = new DeviceListItem(B_TRANSLATE("Audio settings"),
		MediaListItem::AUDIO_TYPE);
	fListView->AddItem(audio);

	MediaListItem* video = new DeviceListItem(B_TRANSLATE("Video settings"),
		MediaListItem::VIDEO_TYPE);
	fListView->AddItem(video);

	MediaListItem* mixer = new AudioMixerListItem(B_TRANSLATE("Audio mixer"));
	fListView->AddItem(mixer);

	fListView->SortItems(&MediaListItem::Compare);

	// Set default nodes for our setting views
	media_node default_node;
	dormant_node_info node_info;
	int32 outputID;
	BString outputName;

	if (roster->GetAudioInput(&default_node) == B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		NodeListItem* item = _FindNodeListItem(&node_info);
		if (item)
			item->SetDefaultInput(true);
		fAudioView->SetDefault(node_info, true);
	}

	if (roster->GetAudioOutput(&default_node, &outputID, &outputName)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		NodeListItem* item = _FindNodeListItem(&node_info);
		if (item)
			item->SetDefaultOutput(true);
		fAudioView->SetDefault(node_info, false, outputID);
	}

	if (roster->GetVideoInput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		NodeListItem* item = _FindNodeListItem(&node_info);
		if (item)
			item->SetDefaultInput(true);
		fVideoView->SetDefault(node_info, true);
	}

	if (roster->GetVideoOutput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		NodeListItem* item = _FindNodeListItem(&node_info);
		if (item)
			item->SetDefaultOutput(true);
		fVideoView->SetDefault(node_info, false);
	}

	if (first) {
		fListView->Select(fListView->IndexOf(mixer));
	} else {
		if (!fAudioView->fRestartView->IsHidden())
			fAudioView->fRestartView->Hide();
		if (!fVideoView->fRestartView->IsHidden())
			fVideoView->fRestartView->Hide();

		if (isVideoSelected)
			fListView->Select(fListView->IndexOf(video));
		else
			fListView->Select(fListView->IndexOf(audio));
	}

	if (fAlert) {
		snooze(800000);
		fAlert->PostMessage(B_QUIT_REQUESTED);
	}
	fAlert = NULL;

	Unlock();

	return B_OK;
}


bool
MediaWindow::QuitRequested()
{
	// stop watching the MediaRoster
	fCurrentNode.SetTo(NULL);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MediaWindow::FrameResized (float width, float height)
{
	// This makes sure our SideBar colours down to the bottom when resized
	BRect r;
	r = Bounds();
}


// ErrorAlert -- Displays a BAlert Box with a Custom Error or Debug Message
void
ErrorAlert(char* errorMessage) {
	printf("%s\n", errorMessage);
	BAlert* alert = new BAlert("BAlert", errorMessage, B_TRANSLATE("OK"),
		NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
	exit(1);
}


void
MediaWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case ML_INIT_MEDIA:
			InitMedia(false);
			break;
		case ML_DEFAULTOUTPUT_CHANGE:
			{
				int32 index;
				if (message->FindInt32("index", &index)!=B_OK)
					break;
				Settings2Item* item = static_cast<Settings2Item*>(
					fAudioView->fChannelMenu->ItemAt(index));

				if (item) {
					BMediaRoster* roster = BMediaRoster::Roster();
					roster->SetAudioOutput(*item->fInput);

					if (fAudioView->fRestartView->IsHidden())
						fAudioView->fRestartView->Show();
				} else
					fprintf(stderr, "Settings2Item not found\n");
			}
			break;
		case ML_DEFAULT_CHANGE:
			{
				bool isVideo = true;
				bool isInput = true;
				if (message->FindBool("isVideo", &isVideo)!=B_OK)
					break;
				if (message->FindBool("isInput", &isInput)!=B_OK)
					break;
				int32 index;
				if (message->FindInt32("index", &index)!=B_OK)
					break;
				SettingsView* settingsView = isVideo ? fVideoView : fAudioView;
				BMenu* menu = isInput ? settingsView->fInputMenu
					: settingsView->fOutputMenu;
				SettingsItem* item = static_cast<SettingsItem*>(
					menu->ItemAt(index));

				if (item) {
					PRINT(("isVideo %i isInput %i\n", isVideo, isInput));
					BMediaRoster* roster = BMediaRoster::Roster();
					if (isVideo) {
						if (isInput)
							roster->SetVideoInput(*item->fInfo);
						else
							roster->SetVideoOutput(*item->fInfo);
					} else {
						if (isInput)
							roster->SetAudioInput(*item->fInfo);
						else {
							roster->SetAudioOutput(*item->fInfo);
							fAudioView->SetDefault(*item->fInfo, false, 0);
						}
					}

					NodeListItem* oldListItem = NULL;
					MediaListItem::media_type mediaType = isVideo ?
						MediaListItem::VIDEO_TYPE : MediaListItem::AUDIO_TYPE;
					for (int32 j = 0; j < fListView->CountItems(); j++) {
						NodeListItem* item = dynamic_cast<NodeListItem*>(
							fListView->ItemAt(j));
						if (!oldListItem)
							continue;
						if (oldListItem->Type() != mediaType)
							continue;
						if (isInput && !oldListItem->IsDefaultInput()) {
							oldListItem = item;
							break;
						} else if (!isInput && oldListItem->IsDefaultOutput()) {
							oldListItem = item;
							break;
						}
					}
					if (oldListItem && isInput)
						oldListItem->SetDefaultInput(false);
					else if (oldListItem && !isInput)
						oldListItem->SetDefaultOutput(false);
					else
						fprintf(stderr, "oldListItem not found\n");

					NodeListItem* listItem = _FindNodeListItem(item->fInfo);
					if (listItem && isInput)
						listItem->SetDefaultInput(true);
					else if (listItem && !isInput)
						listItem->SetDefaultOutput(true);
					else
						fprintf(stderr, "MediaListItem not found\n");
					fListView->Invalidate();

					if (settingsView->fRestartView->IsHidden())
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
					fprintf(stderr, B_TRANSLATE(
						"Couldn't add volume control in Deskbar: %s\n"),
						strerror(status));
				}
			} else {
				status_t status = deskbar.RemoveItem("MediaReplicant");
				if (status != B_OK) {
					fprintf(stderr, B_TRANSLATE(
						"Couldn't remove volume control in Deskbar: %s\n"),
						strerror(status));
				}
			}
			break;
		}
		case ML_ENABLE_REAL_TIME:
			{
				bool isVideo = true;
				if (message->FindBool("isVideo", &isVideo) != B_OK)
					break;
				SettingsView* settingsView = isVideo ? fVideoView : fAudioView;
				uint32 flags;
				uint32 realtimeFlag = isVideo
					? B_MEDIA_REALTIME_VIDEO : B_MEDIA_REALTIME_AUDIO;
				BMediaRoster* roster = BMediaRoster::Roster();
				roster->GetRealtimeFlags(&flags);
				if (settingsView->fRealtimeCheckBox->Value() == B_CONTROL_ON)
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

				MediaListItem* item = static_cast<MediaListItem*>(
						fListView->ItemAt(fListView->CurrentSelection()));
				if (!item)
					break;

				fCurrentNode.SetTo(NULL);
				_ClearParamView();
				item->AlterWindow(this);
			}
			break;
		case B_SOME_APP_LAUNCHED:
			{
				PRINT_OBJECT(*message);
				if (!fAlert)
					break;

				BString mimeSig;
				if (message->FindString("be:signature", &mimeSig) == B_OK
					&& (mimeSig == "application/x-vnd.Be.addon-host"
						|| mimeSig == "application/x-vnd.Be.media-server")) {
					fAlert->Lock();
					fAlert->TextView()->SetText(
						B_TRANSLATE("Starting media server" B_UTF8_ELLIPSIS));
					fAlert->Unlock();
				}
			}
			break;
		case B_SOME_APP_QUIT:
			{
				PRINT_OBJECT(*message);
				BString mimeSig;
				if (message->FindString("be:signature", &mimeSig) == B_OK) {
					if (mimeSig == "application/x-vnd.Be.addon-host"
						|| mimeSig == "application/x-vnd.Be.media-server") {
						BMediaRoster* roster = BMediaRoster::CurrentRoster();
						if (roster && roster->Lock())
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
MediaWindow::RestartMediaServices(void* data)
{
	MediaWindow* window = (MediaWindow*)data;
	window->fAlert = new MediaAlert(BRect(0, 0, 300, 60),
		"restart_alert", B_TRANSLATE(
		"Restarting media services\nShutting down media server\n"));

	window->fAlert->Show();

	shutdown_media_server(B_INFINITE_TIMEOUT, MediaWindow::UpdateProgress,
		window->fAlert);

	{
		BAutolock locker(window->fAlert);
		if (locker.IsLocked())
			window->fAlert->TextView()->SetText(
				B_TRANSLATE("Starting media server" B_UTF8_ELLIPSIS));
	}
	launch_media_server();

	return window->PostMessage(ML_INIT_MEDIA);
}


bool
MediaWindow::UpdateProgress(int stage, const char * message, void * cookie)
{
	MediaAlert* alert = static_cast<MediaAlert*>(cookie);
	PRINT(("stage : %i\n", stage));
	const char* string = "Unknown stage";
	switch (stage) {
		case 10:
			string = B_TRANSLATE("Stopping media server" B_UTF8_ELLIPSIS);
			break;
		case 20:
			string = B_TRANSLATE("Telling media_addon_server to quit.");
			break;
		case 40:
			string = B_TRANSLATE("Waiting for media_server to quit.");
			break;
		case 70:
			string = B_TRANSLATE("Cleaning up.");
			break;
		case 100:
			string = B_TRANSLATE("Done shutting down.");
			break;
	}

	BAutolock locker(alert);
	if (locker.IsLocked())
		alert->TextView()->SetText(string);
	return true;
}


void
MediaWindow::_ClearParamView()
{
	BView* child = fContentView->ChildAt(0);
	if (child)
		child->RemoveSelf();
	if (child != fVideoView && child != fAudioView)
		delete child;
	delete fParamWeb;
	fParamWeb = NULL;
}


void
MediaWindow::_MakeParamView()
{
	if (!fCurrentNode.IsSet())
		return;

	fParamWeb = NULL;
	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster->GetParameterWebFor(fCurrentNode, &fParamWeb) == B_OK) {
		BView* paramView = BMediaTheme::ViewFor(fParamWeb);
		if (paramView) {
			fContentView->AddChild(paramView);
			paramView->ResizeTo(1, 1);
				// make sure we don't get stuck with a very large
				// paramView
			return;
		}
	}
	_MakeEmptyParamView();
	
}


void
MediaWindow::_MakeEmptyParamView()
{
	fParamWeb = NULL;

	BStringView* stringView = new BStringView("noControls",
		B_TRANSLATE("This hardware has no controls."));

	BSize unlimited(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	stringView->SetExplicitMaxSize(unlimited);

	BAlignment centered(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_VERTICAL_CENTER);
	stringView->SetExplicitAlignment(centered);
	stringView->SetAlignment(B_ALIGN_CENTER);
	
	fContentView->AddChild(stringView);

	rgb_color panel = stringView->LowColor();
	stringView->SetHighColor(tint_color(panel,
		B_DISABLED_LABEL_TINT));
}

