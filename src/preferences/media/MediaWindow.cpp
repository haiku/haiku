/*
 * Copyright 2003-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Sikosis, Jérôme Duval
 *		yourpalal, Alex Wilson
 */


#include "MediaWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Autolock.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Debug.h>
#include <Deskbar.h>
#include <IconUtils.h>
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

#include "Media.h"
#include "MediaIcons.h"
#include "MidiSettingsView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Media Window"


const uint32 ML_SELECTED_NODE = 'MlSN';
const uint32 ML_RESTART_THREAD_FINISHED = 'MlRF';


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
	virtual	void	Visit(MidiListItem*){}
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

	status_t status = B_OK;
	media_node_id node_id;
	if (roster->GetInstancesFor(info->addon, info->flavor_id, &node_id) == B_OK)
		status = roster->GetNodeFor(node_id, fNode);
	else
		status = roster->InstantiateDormantNode(*info, fNode, B_FLAVOR_IS_GLOBAL);

	if (status != B_OK) {
		fprintf(stderr, "SmartNode::SetTo error with node %" B_PRId32
			": %s\n", fNode->node, strerror(status));
	}

	status = roster->StartWatching(fMessenger, *fNode, B_MEDIA_WILDCARD);
	if (status != B_OK) {
		fprintf(stderr, "SmartNode::SetTo can't start watching for"
			" node %" B_PRId32 "\n", fNode->node);
	}
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

	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster != NULL) {
		status_t status = roster->StopWatching(fMessenger,
			*fNode, B_MEDIA_WILDCARD);
		if (status != B_OK) {
			fprintf(stderr, "SmartNode::_FreeNode can't unwatch"
				" media services for node %" B_PRId32 "\n", fNode->node);
		}

		roster->ReleaseNode(*fNode);
		if (status != B_OK) {
			fprintf(stderr, "SmartNode::_FreeNode can't release"
				" node %" B_PRId32 "\n", fNode->node);
		}
	}
	delete fNode;
	fNode = NULL;
}


// #pragma mark -


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
	fInitCheck(B_OK),
	fRestartThread(-1),
	fRestartAlert(NULL)
{
	_InitWindow();

	BMediaRoster* roster = BMediaRoster::Roster();
	roster->StartWatching(BMessenger(this, this),
		B_MEDIA_SERVER_STARTED);
	roster->StartWatching(BMessenger(this, this),
		B_MEDIA_SERVER_QUIT);
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
		BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			file.Write(buffer, strlen(buffer));
	}

	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	roster->StopWatching(BMessenger(this, this),
		B_MEDIA_SERVER_STARTED);
	roster->StartWatching(BMessenger(this, this),
		B_MEDIA_SERVER_QUIT);
}


status_t
MediaWindow::InitCheck()
{
	return fInitCheck;
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
MediaWindow::SelectMidiSettings(const char* title)
{
	fContentLayout->SetVisibleItem(fContentLayout->IndexOfView(fMidiView));
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


bool
MediaWindow::QuitRequested()
{
	if (fRestartThread > 0) {
		BString text(B_TRANSLATE("Quitting Media now will stop the "
			"restarting of the media services. Flaky or unavailable media "
			"functionality is the likely result."));

		fRestartAlert = new BAlert(B_TRANSLATE("Warning!"), text,
			B_TRANSLATE("Quit anyway"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

		fRestartAlert->Go();
	}
	// Stop watching the MediaRoster
	fCurrentNode.SetTo(NULL);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MediaWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ML_RESTART_THREAD_FINISHED:
			fRestartThread = -1;
			_InitMedia(false);
			break;

		case ML_RESTART_MEDIA_SERVER:
		{
			fRestartThread = spawn_thread(&MediaWindow::_RestartMediaServices,
				"restart_thread", B_NORMAL_PRIORITY, this);
			if (fRestartThread < 0)
				fprintf(stderr, "couldn't create restart thread\n");
			else
				resume_thread(fRestartThread);
			break;
		}

		case B_MEDIA_WEB_CHANGED:
		case ML_SELECTED_NODE:
		{
			PRINT_OBJECT(*message);

			MediaListItem* item = static_cast<MediaListItem*>(
					fListView->ItemAt(fListView->CurrentSelection()));
			if (item == NULL)
				break;

			fCurrentNode.SetTo(NULL);
			_ClearParamView();
			item->AlterWindow(this);
			break;
		}

		case B_MEDIA_SERVER_STARTED:
		case B_MEDIA_SERVER_QUIT:
		{
			PRINT_OBJECT(*message);
			_InitMedia(false);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


// #pragma mark - private


void
MediaWindow::_InitWindow()
{
	fListView = new BListView("media_list_view");
	fListView->SetSelectionMessage(new BMessage(ML_SELECTED_NODE));
	fListView->SetExplicitMinSize(BSize(140, B_SIZE_UNSET));

	// Add ScrollView to Media Menu for pretty border
	BScrollView* scrollView = new BScrollView("listscroller",
		fListView, 0, false, false, B_FANCY_BORDER);

	// Create the Views
	fTitleView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fTitleView->SetLabel(B_TRANSLATE("Audio settings"));
	fTitleView->SetFont(be_bold_font);

	fContentLayout = new BCardLayout();
	new BView("content view", 0, fContentLayout);
	fContentLayout->Owner()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fContentLayout->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fAudioView = new AudioSettingsView();
	fContentLayout->AddView(fAudioView);

	fVideoView = new VideoSettingsView();
	fContentLayout->AddView(fVideoView);

	fMidiView = new MidiSettingsView();
	fContentLayout->AddView(fMidiView);

	// Layout all views
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(scrollView, 0.0f)
		.AddGroup(B_VERTICAL)
			.SetInsets(0, 0, 0, 0)
			.Add(fTitleView)
			.Add(fContentLayout);

	// Start the window
	fInitCheck = _InitMedia(true);
	if (fInitCheck != B_OK)
		PostMessage(B_QUIT_REQUESTED);
	else if (IsHidden())
		Show();
}


status_t
MediaWindow::_InitMedia(bool first)
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
		alert->SetShortcut(0, B_ESCAPE);
		if (alert->Go() == 0)
			return B_ERROR;

		Show();

		launch_media_server();
	}

	Lock();

	bool isVideoSelected = true;
	if (!first && fListView->ItemAt(0) != NULL
		&& fListView->ItemAt(0)->IsSelected())
		isVideoSelected = false;

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

	MidiListItem* midi = new MidiListItem(B_TRANSLATE("MIDI Settings"));
	fListView->AddItem(midi);

	MediaListItem* video = new DeviceListItem(B_TRANSLATE("Video settings"),
		MediaListItem::VIDEO_TYPE);
	fListView->AddItem(video);

	MediaListItem* mixer = new AudioMixerListItem(B_TRANSLATE("Audio mixer"));
	fListView->AddItem(mixer);

	fListView->SortItems(&MediaListItem::Compare);
	_UpdateListViewMinWidth();

	// Set default nodes for our setting views
	media_node defaultNode;
	dormant_node_info nodeInfo;
	int32 outputID;
	BString outputName;

	if (roster->GetAudioInput(&defaultNode) == B_OK) {
		roster->GetDormantNodeFor(defaultNode, &nodeInfo);
		fAudioView->SetDefaultInput(&nodeInfo);
			// this causes our listview to be updated as well
	}

	if (roster->GetAudioOutput(&defaultNode, &outputID, &outputName) == B_OK) {
		roster->GetDormantNodeFor(defaultNode, &nodeInfo);
		fAudioView->SetDefaultOutput(&nodeInfo);
		fAudioView->SetDefaultChannel(outputID);
			// this causes our listview to be updated as well
	}

	if (roster->GetVideoInput(&defaultNode) == B_OK) {
		roster->GetDormantNodeFor(defaultNode, &nodeInfo);
		fVideoView->SetDefaultInput(&nodeInfo);
			// this causes our listview to be updated as well
	}

	if (roster->GetVideoOutput(&defaultNode) == B_OK) {
		roster->GetDormantNodeFor(defaultNode, &nodeInfo);
		fVideoView->SetDefaultOutput(&nodeInfo);
			// this causes our listview to be updated as well
	}

	if (first)
		fListView->Select(fListView->IndexOf(mixer));
	else if (isVideoSelected)
		fListView->Select(fListView->IndexOf(video));
	else
		fListView->Select(fListView->IndexOf(audio));

	Unlock();

	return B_OK;
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
	dormant_node_info nodeInfo[64];
	int32 nodeInfoCount = 64;

	media_format format;
	media_format* nodeInputFormat = NULL;
	media_format* nodeOutputFormat = NULL;
	format.type = type;

	// output nodes must be BBufferConsumers => they have an input format
	// input nodes must be BBufferProducers => they have an output format
	if ((kind & B_PHYSICAL_OUTPUT) != 0)
		nodeInputFormat = &format;
	else if ((kind & B_PHYSICAL_INPUT) != 0)
		nodeOutputFormat = &format;
	else
		return;

	BMediaRoster* roster = BMediaRoster::Roster();

	if (roster->GetDormantNodes(nodeInfo, &nodeInfoCount, nodeInputFormat,
			nodeOutputFormat, NULL, kind) != B_OK) {
		// TODO: better error reporting!
		fprintf(stderr, "error\n");
		return;
	}

	for (int32 i = 0; i < nodeInfoCount; i++) {
		PRINT(("node : %s, media_addon %i, flavor_id %i\n",
			nodeInfo[i].name, (int)nodeInfo[i].addon,
			(int)nodeInfo[i].flavor_id));

		dormant_node_info* info = new dormant_node_info();
		strlcpy(info->name, nodeInfo[i].name, B_MEDIA_NAME_LENGTH);
		info->flavor_id = nodeInfo[i].flavor_id;
		info->addon = nodeInfo[i].addon;
		into.AddItem(info);
	}
}


void
MediaWindow::_AddNodeItems(NodeList& list, MediaListItem::media_type type)
{
	int32 count = list.CountItems();
	for (int32 i = 0; i < count; i++) {
		dormant_node_info* info = list.ItemAt(i);
		if (_FindNodeListItem(info) == NULL)
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


NodeListItem*
MediaWindow::_FindNodeListItem(dormant_node_info* info)
{
	NodeListItem audioItem(info, MediaListItem::AUDIO_TYPE);
	NodeListItem videoItem(info, MediaListItem::VIDEO_TYPE);

	NodeListItem::Comparator audioComparator(&audioItem);
	NodeListItem::Comparator videoComparator(&videoItem);

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		MediaListItem* item = static_cast<MediaListItem*>(fListView->ItemAt(i));
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


status_t
MediaWindow::_RestartMediaServices(void* data)
{
	MediaWindow* window = (MediaWindow*)data;

	shutdown_media_server();

	if (window->fRestartAlert != NULL
			&& window->fRestartAlert->Lock()) {
		window->fRestartAlert->Quit();
	}

	return window->PostMessage(ML_RESTART_THREAD_FINISHED);
}


void
MediaWindow::_ClearParamView()
{
	BLayoutItem* item = fContentLayout->VisibleItem();
	if (!item)
		return;

	BView* view = item->View();
	if (view != fVideoView && view != fAudioView && view != fMidiView) {
		fContentLayout->RemoveItem(item);
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

