/*
 * Copyright (c) 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Sikosis, Jérôme Duval
 *		yourpalal, Alex Wilson
 */


#include "MediaWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Debug.h>
#include <Deskbar.h>
#include <LayoutBuilder.h>
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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Media Window"


const uint32 ML_SELECTED_NODE = 'MlSN';
const uint32 ML_INIT_MEDIA = 'MlIM';


class NodeListItemUpdater : public MediaListItem::Visitor {
public:
	typedef void (NodeListItem::*UpdateMethod)(bool);

	NodeListItemUpdater(NodeListItem* target, UpdateMethod action)
		:
		fComparator(target),
		fAction(action)
	{
	}


	virtual	void	Visit(AudioMixerListItem*){}
	virtual	void	Visit(DeviceListItem*){}

	virtual void	Visit(NodeListItem* item)
	{
		item->Accept(fComparator);
		(item->*(fAction))(fComparator.result == 0);
	}

private:

			NodeListItem::Comparator		fComparator;
			UpdateMethod					fAction;
};


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
MediaWindow::SmartNode::SetTo(const dormant_node_info* info)
{
	_FreeNode();
	if (!info)
		return;

	fNode = new media_node();
	BMediaRoster* roster = BMediaRoster::Roster();

	// TODO: error codes
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
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Media"), B_TITLED_WINDOW,
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

	char buffer[512];
	BRect rect = Frame();
	PRINT_OBJECT(rect);
	snprintf(buffer, 512, "# MediaPrefs Settings\n rect = %i,%i,%i,%i\n",
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
MediaWindow::SelectNode(const dormant_node_info* node)
{
	fCurrentNode.SetTo(node);
	_MakeParamView();
	fTitleView->SetLabel(node->name);
}


void
MediaWindow::SelectAudioSettings(const char* title)
{
	fContentLayout->SetVisibleItem(fContentLayout->IndexOfView(fAudioView));
	fTitleView->SetLabel(title);
}


void
MediaWindow::SelectVideoSettings(const char* title)
{
	fContentLayout->SetVisibleItem(fContentLayout->IndexOfView(fVideoView));
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
MediaWindow::UpdateInputListItem(MediaListItem::media_type type,
	const dormant_node_info* node)
{
	NodeListItem compareTo(node, type);
	NodeListItemUpdater updater(&compareTo, &NodeListItem::SetDefaultInput);
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		MediaListItem* item = static_cast<MediaListItem*>(fListView->ItemAt(i));
		item->Accept(updater);
	}
	fListView->Invalidate();
}


void
MediaWindow::UpdateOutputListItem(MediaListItem::media_type type,
	const dormant_node_info* node)
{
	NodeListItem compareTo(node, type);
	NodeListItemUpdater updater(&compareTo, &NodeListItem::SetDefaultOutput);
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		MediaListItem* item = static_cast<MediaListItem*>(fListView->ItemAt(i));
		item->Accept(updater);
	}
	fListView->Invalidate();
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
			node_info[i].name, (int)node_info[i].addon,
			(int)node_info[i].flavor_id));

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
	NodeListItem audioItem(info, MediaListItem::AUDIO_TYPE);
	NodeListItem videoItem(info, MediaListItem::VIDEO_TYPE);

	NodeListItem::Comparator audioComparator(&audioItem);
	NodeListItem::Comparator videoComparator(&videoItem);

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		MediaListItem* item
			= static_cast<MediaListItem*>(fListView->ItemAt(i));
		item->Accept(audioComparator);
		if (audioComparator.result == 0)
			return static_cast<NodeListItem*>(item);

		item->Accept(videoComparator);
		if (videoComparator.result == 0)
			return static_cast<NodeListItem*>(item);
	}
	return NULL;
}


void
MediaWindow::_UpdateListViewMinWidth()
{
	float width = 0;
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		BListItem* item = fListView->ItemAt(i);
		width = max_c(width, item->Width());
	}
	fListView->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
	fListView->InvalidateLayout();
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
	fListView = new BListView("media_list_view");
	fListView->SetSelectionMessage(new BMessage(ML_SELECTED_NODE));

	// Add ScrollView to Media Menu for pretty border
	BScrollView* scrollView = new BScrollView("listscroller",
		fListView, 0, false, false, B_FANCY_BORDER);

	// Create the Views
	fTitleView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fTitleView->SetLabel(B_TRANSLATE("Audio settings"));
	fTitleView->SetFont(be_bold_font);

	fContentLayout = new BCardLayout();
	new BView("content view", 0, fContentLayout);
	fContentLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fAudioView = new AudioSettingsView();
	fContentLayout->AddView(fAudioView);

	fVideoView = new VideoSettingsView();
	fContentLayout->AddView(fVideoView);


	// Layout all views
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(scrollView)
		.AddGroup(B_VERTICAL)
			.SetInsets(0, 0, 0, 0)
			.Add(fTitleView)
			.Add(fContentLayout);

	// Start the window
	fInitCheck = InitMedia(true);
	if (fInitCheck != B_OK) {
		PostMessage(B_QUIT_REQUESTED);
	} else 	if (IsHidden()) {
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
		delete fListView->RemoveItem((int32)0);
	_EmptyNodeLists();

	// Grab Media Info
	_FindNodes();

	// Add video nodes first. They might have an additional audio
	// output or input, but still should be listed as video node.
	_AddNodeItems(fVideoOutputs, MediaListItem::VIDEO_TYPE);
	_AddNodeItems(fVideoInputs, MediaListItem::VIDEO_TYPE);
	_AddNodeItems(fAudioOutputs, MediaListItem::AUDIO_TYPE);
	_AddNodeItems(fAudioInputs, MediaListItem::AUDIO_TYPE);

	fAudioView->AddOutputNodes(fAudioOutputs);
	fAudioView->AddInputNodes(fAudioInputs);
	fVideoView->AddOutputNodes(fVideoOutputs);
	fVideoView->AddInputNodes(fVideoInputs);

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
	_UpdateListViewMinWidth();

	// Set default nodes for our setting views
	media_node default_node;
	dormant_node_info node_info;
	int32 outputID;
	BString outputName;

	if (roster->GetAudioInput(&default_node) == B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		fAudioView->SetDefaultInput(&node_info);
			// this causes our listview to be updated as well
	}

	if (roster->GetAudioOutput(&default_node, &outputID, &outputName)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		fAudioView->SetDefaultOutput(&node_info);
		fAudioView->SetDefaultChannel(outputID);
			// this causes our listview to be updated as well
	}

	if (roster->GetVideoInput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		fVideoView->SetDefaultInput(&node_info);
			// this causes our listview to be updated as well
	}

	if (roster->GetVideoOutput(&default_node)==B_OK) {
		roster->GetDormantNodeFor(default_node, &node_info);
		fVideoView->SetDefaultOutput(&node_info);
			// this causes our listview to be updated as well
	}

	if (first) {
		fListView->Select(fListView->IndexOf(mixer));
	} else {
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
			break;
		}
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
	BLayoutItem* item = fContentLayout->VisibleItem();
	if (!item)
		return;

	BView* view = item->View();
	if (view != fVideoView && view != fAudioView) {
		fContentLayout->RemoveItem(item);
		delete item;
		delete view;
		delete fParamWeb;
		fParamWeb = NULL;
	}
}


void
MediaWindow::_MakeParamView()
{
	if (!fCurrentNode.IsSet())
		return;

	fParamWeb = NULL;
	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster->GetParameterWebFor(fCurrentNode, &fParamWeb) == B_OK) {
		BRect hint(fContentLayout->Frame());
		BView* paramView = BMediaTheme::ViewFor(fParamWeb, &hint);
		if (paramView) {
			fContentLayout->AddView(paramView);
			fContentLayout->SetVisibleItem(fContentLayout->CountItems() - 1);
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
	
	fContentLayout->AddView(stringView);
	fContentLayout->SetVisibleItem(fContentLayout->CountItems() - 1);

	rgb_color panel = stringView->LowColor();
	stringView->SetHighColor(tint_color(panel,
		B_DISABLED_LABEL_TINT));
}

