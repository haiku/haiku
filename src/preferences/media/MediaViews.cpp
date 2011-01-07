// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered
//  by the OpenBeOS license.
//
//
//  File:        MediaViews.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#include "MediaViews.h"

#include <Alert.h>
#include <AutoDeleter.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Deskbar.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>

#include <assert.h>
#include <stdio.h>

#include "MediaWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Media views"

#define MEDIA_DEFAULT_INPUT_CHANGE 'dich'
#define MEDIA_DEFAULT_OUTPUT_CHANGE 'doch'
#define MEDIA_SHOW_HIDE_VOLUME_CONTROL 'shvc'


SettingsView::SettingsView()
	:
	BGridView(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING),
	fRealtimeCheckBox(NULL),
	fInputMenu(NULL),
	fOutputMenu(NULL),
	fRestartView(NULL)
{
	// input menu
	fInputMenu = new BPopUpMenu(B_TRANSLATE("<none>"));
	fInputMenu->SetLabelFromMarked(true);

	// input menu
	fOutputMenu = new BPopUpMenu(B_TRANSLATE("<none>"));
	fOutputMenu->SetLabelFromMarked(true);
}


BBox*
SettingsView::MakeRealtimeBox(const char* info, uint32 realtimeMask,
	const char* checkBoxLabel)
{
	// create the realtime box
	BBox* realtimeBox = new BBox("realtime");
	realtimeBox->SetLabel(B_TRANSLATE("Real-time"));

	BMessage* checkBoxMessage = new BMessage(ML_ENABLE_REAL_TIME);
	checkBoxMessage->AddUInt32("flags", realtimeMask);
	fRealtimeCheckBox = new BCheckBox("realtimeCheckBox", checkBoxLabel,
		checkBoxMessage);

	uint32 flags = 0;
	BMediaRoster::Roster()->GetRealtimeFlags(&flags);
	if (flags & realtimeMask)
		fRealtimeCheckBox->SetValue(B_CONTROL_ON);

	BTextView* textView = new BTextView("stringView");
	textView->Insert(info);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGroupView* realtimeGroup = new BGroupView(B_VERTICAL);
	BLayoutBuilder::Group<>(realtimeGroup)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING)
		.Add(fRealtimeCheckBox)
		.Add(textView);

	realtimeBox->AddChild(realtimeGroup);
	return realtimeBox;
}


BStringView*
SettingsView::MakeRestartMessageView()
{
	// note: this ought to display at the bottom of the default box...
	fRestartView = new BStringView("restartStringView",
		B_TRANSLATE("Restart the media server to apply changes."));
	fRestartView->SetHighColor(ui_color(B_FAILURE_COLOR));
		// not exactly failure, but sort of.
	fRestartView->Hide();
	fRestartView->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_VERTICAL_CENTER));
	fRestartView->SetAlignment(B_ALIGN_HORIZONTAL_CENTER);
	fRestartView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	return fRestartView;
}


BButton*
SettingsView::MakeRestartButton()
{
	return new BButton("restartButton",
		B_TRANSLATE("Restart media services"),
		new BMessage(ML_RESTART_MEDIA_SERVER));
}



void
SettingsView::AddInputNodes(NodeList& list)
{
	_EmptyMenu(fInputMenu);

	BMessage message(MEDIA_DEFAULT_INPUT_CHANGE);
	_PopulateMenu(fInputMenu, list, message);
}


void
SettingsView::AddOutputNodes(NodeList& list)
{
	_EmptyMenu(fOutputMenu);

	BMessage message(MEDIA_DEFAULT_OUTPUT_CHANGE);
	_PopulateMenu(fOutputMenu, list, message);
}


void
SettingsView::SetDefaultInput(const dormant_node_info* info)
{
	_ClearMenuSelection(fInputMenu);
	NodeMenuItem* item = _FindNodeItem(fInputMenu, info);
	if (item)
		item->SetMarked(true);
}


void
SettingsView::SetDefaultOutput(const dormant_node_info* info)
{
	_ClearMenuSelection(fOutputMenu);
	NodeMenuItem* item = _FindNodeItem(fOutputMenu, info);
	if (item)
		item->SetMarked(true);
}


void
SettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MEDIA_DEFAULT_INPUT_CHANGE:
		{
			int32 index;
			if (message->FindInt32("index", &index)!=B_OK)
				break;
			NodeMenuItem* item
				= static_cast<NodeMenuItem*>(fInputMenu->ItemAt(index));
			SetDefaultInput(item->NodeInfo());
			RestartRequired(true);
		}
		case MEDIA_DEFAULT_OUTPUT_CHANGE:
		{
			int32 index;
			if (message->FindInt32("index", &index)!=B_OK)
				break;
			NodeMenuItem* item
				= static_cast<NodeMenuItem*>(fOutputMenu->ItemAt(index));
			SetDefaultOutput(item->NodeInfo());
			RestartRequired(true);
		}
		case ML_ENABLE_REAL_TIME:
		{
			uint32 flags = 0;
			if (message->FindUInt32("flags", &flags) == B_OK)
				_FlipRealtimeFlag(flags);
			break;
		}
	}
	BGridView::MessageReceived(message);
}


void
SettingsView::AttachedToWindow()
{
	BMessenger thisMessenger(this);
	fInputMenu->SetTargetForItems(thisMessenger);
	fOutputMenu->SetTargetForItems(thisMessenger);
	fRealtimeCheckBox->SetTarget(thisMessenger);
}


void
SettingsView::RestartRequired(bool required)
{
	if (required)
		fRestartView->Show();
	else
		fRestartView->Hide();
}


MediaWindow*
SettingsView::_MediaWindow() const
{
	return static_cast<MediaWindow*>(Window());
}


void
SettingsView::_FlipRealtimeFlag(uint32 mask)
{
	fRealtimeCheckBox->SetValue(B_CONTROL_OFF);
	BAlert* alert = new BAlert("realtime alert", UnimplementedRealtimeError(),
		B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->Go();

#if 0
	// TODO: error codes
	uint32 flags = 0;
	BMediaRoster* roster = BMediaRoster::Roster();
	roster->GetRealtimeFlags(&flags);
	if (flags & mask)
		flags &= ~mask;
	else
		flags |= mask;
	roster->SetRealtimeFlags(flags);
#endif
}


void
SettingsView::_EmptyMenu(BMenu* menu)
{
	while (menu->CountItems() > 0)
		delete menu->RemoveItem((int32)0);
}


void
SettingsView::_PopulateMenu(BMenu* menu, NodeList& nodes,
	const BMessage& message)
{
	for (int32 i = 0; i < nodes.CountItems(); i++) {
		dormant_node_info* info = nodes.ItemAt(i);
		menu->AddItem(new NodeMenuItem(info, new BMessage(message)));
	}

	if (Window() != NULL)
		menu->SetTargetForItems(BMessenger(this));
}


NodeMenuItem*
SettingsView::_FindNodeItem(BMenu* menu, const dormant_node_info* nodeInfo)
{
	for (int32 i = 0; i < menu->CountItems(); i++) {
		NodeMenuItem* item = static_cast<NodeMenuItem*>(menu->ItemAt(i));
		const dormant_node_info* itemInfo = item->NodeInfo();
		if (itemInfo && itemInfo->addon == nodeInfo->addon
			&& itemInfo->flavor_id == nodeInfo->flavor_id) {
			return item;
		}
	}
	return NULL;
}


void
SettingsView::_ClearMenuSelection(BMenu* menu)
{
	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);
		item->SetMarked(false);
	}
}


NodeMenuItem::NodeMenuItem(const dormant_node_info* info, BMessage* message,
	char shortcut, uint32 modifiers)
	:
	BMenuItem(info->name, message, shortcut, modifiers),
	fInfo(info)
{

}


status_t
NodeMenuItem::Invoke(BMessage* message)
{
	if (IsMarked())
		return B_OK;
	return BMenuItem::Invoke(message);
}


ChannelMenuItem::ChannelMenuItem(media_input* input, BMessage* message,
	char shortcut, uint32 modifiers)
	:
	BMenuItem(input->name, message, shortcut, modifiers),
	fInput(input)
{
}


ChannelMenuItem::~ChannelMenuItem()
{
	delete fInput;
}


int32
ChannelMenuItem::DestinationID()
{
	return fInput->destination.id;
}


media_input*
ChannelMenuItem::Input()
{
	return fInput;
}


status_t
ChannelMenuItem::Invoke(BMessage* message)
{
	if (IsMarked())
		return B_OK;
	return BMenuItem::Invoke(message);
}


AudioSettingsView::AudioSettingsView()
{
	BBox* defaultsBox = new BBox("defaults");
	defaultsBox->SetLabel(B_TRANSLATE("Defaults"));
	BGridView* defaultsGridView = new BGridView();
	
	BMenuField* inputMenuField = new BMenuField("inputMenuField",
		B_TRANSLATE("Audio input:"), InputMenu(), NULL);

	BMenuField* outputMenuField = new BMenuField("outputMenuField",
		B_TRANSLATE("Audio output:"), OutputMenu(), NULL);

	BLayoutBuilder::Grid<>(defaultsGridView)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING, 0)
		.AddMenuField(inputMenuField, 0, 0, B_ALIGN_HORIZONTAL_UNSET, 1, 3, 1)
		.AddMenuField(outputMenuField, 0, 1)
		.AddMenuField(_MakeChannelMenu(), 2, 1)
		.Add(MakeRestartMessageView(), 0, 2, 4, 1);

	defaultsBox->AddChild(defaultsGridView);

	const char* realtimeLabel = B_TRANSLATE("Enable real-time audio");
	const char* realtimeInfo = B_TRANSLATE(
		"Enabling real-time audio allows system to record and play audio "
		"as fast as possible.  It achieves this performance by using more"
		" CPU and RAM.\n\nOnly enable this feature if you need the lowest"
		" latency possible.");


	BLayoutBuilder::Grid<>(this)
		.SetInsets(0, 0, 0, 0)
		.Add(defaultsBox, 0, 0, 2, 1)
		.Add(MakeRealtimeBox(realtimeInfo, B_MEDIA_REALTIME_AUDIO,
			realtimeLabel), 0, 1, 2, 1)
		.Add(_MakeVolumeCheckBox(),0, 2, 1, 1)
		.Add(MakeRestartButton(), 1, 2, 1, 1)
		.SetRowWeight(1, 10);
}


void
AudioSettingsView::SetDefaultChannel(int32 channelID)
{
	for (int32 i = 0; i < fChannelMenu->CountItems(); i++) {
		ChannelMenuItem* item = _ChannelMenuItemAt(i);
		item->SetMarked(item->DestinationID() == channelID);
	}
}


void
AudioSettingsView::AttachedToWindow()
{
	SettingsView::AttachedToWindow();

	BMessenger thisMessenger(this);
	fChannelMenu->SetTargetForItems(thisMessenger);
	fVolumeCheckBox->SetTarget(thisMessenger);
}


BString
AudioSettingsView::UnimplementedRealtimeError()
{
	return B_TRANSLATE("Real-time audio is currently unimplemented in Haiku.");
}


void
AudioSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ML_DEFAULT_CHANNEL_CHANGED:
			{
				int32 index;
				if (message->FindInt32("index", &index) != B_OK)
					break;
				ChannelMenuItem* item = _ChannelMenuItemAt(index);

				if (item) {
					BMediaRoster* roster = BMediaRoster::Roster();
					roster->SetAudioOutput(*item->Input());
					RestartRequired(true);
				} else
					fprintf(stderr, "ChannelMenuItem not found\n");
			}
			break;
		case MEDIA_SHOW_HIDE_VOLUME_CONTROL:
		{
			if (fVolumeCheckBox->Value() == B_CONTROL_ON)
				_ShowDeskbarVolumeControl();
			else
				_HideDeskbarVolumeControl();
			break;
		}

		default:
			SettingsView::MessageReceived(message);
	}
}


void
AudioSettingsView::SetDefaultInput(const dormant_node_info* info)
{
	SettingsView::SetDefaultInput(info);
	_MediaWindow()->UpdateInputListItem(MediaListItem::AUDIO_TYPE, info);
	BMediaRoster::Roster()->SetAudioInput(*info);
}


void
AudioSettingsView::SetDefaultOutput(const dormant_node_info* info)
{
	SettingsView::SetDefaultOutput(info);
	_MediaWindow()->UpdateOutputListItem(MediaListItem::AUDIO_TYPE, info);
	_FillChannelMenu(info);
	BMediaRoster::Roster()->SetAudioOutput(*info);
}


BMenuField*
AudioSettingsView::_MakeChannelMenu()
{
	fChannelMenu = new BPopUpMenu(B_TRANSLATE("<none>"));
	fChannelMenu->SetLabelFromMarked(true);
	BMenuField* channelMenuField = new BMenuField("channelMenuField",
		B_TRANSLATE("Channel:"), fChannelMenu, NULL);
	return channelMenuField;
}


BCheckBox*
AudioSettingsView::_MakeVolumeCheckBox()
{
	fVolumeCheckBox = new BCheckBox("volumeCheckBox",
		B_TRANSLATE("Show volume control on Deskbar"),
		new BMessage(MEDIA_SHOW_HIDE_VOLUME_CONTROL));

	if (BDeskbar().HasItem("MediaReplicant"))
		fVolumeCheckBox->SetValue(B_CONTROL_ON);

	return fVolumeCheckBox;
}


void
AudioSettingsView::_FillChannelMenu(const dormant_node_info* nodeInfo)
{
	_EmptyMenu(fChannelMenu);

	BMediaRoster* roster = BMediaRoster::Roster();
	media_node node;
	media_node_id node_id;

	status_t err = roster->GetInstancesFor(nodeInfo->addon,
		nodeInfo->flavor_id, &node_id);
	if (err != B_OK) {
		err = roster->InstantiateDormantNode(*nodeInfo, &node,
			B_FLAVOR_IS_GLOBAL);
	} else {
		err = roster->GetNodeFor(node_id, &node);
	}

	if (err == B_OK) {
		int32 inputCount = 4;
		media_input* inputs = new media_input[inputCount];
		BPrivate::ArrayDeleter<media_input> inputDeleter(inputs);

		while (true) {
			int32 realInputCount = 0;
			err = roster->GetAllInputsFor(node, inputs,
				inputCount, &realInputCount);
			if (realInputCount > inputCount) {
				inputCount *= 2;
				inputs = new media_input[inputCount];
				inputDeleter.SetTo(inputs);
			} else {
				inputCount = realInputCount;
				break;
			}
		}

		if (err == B_OK) {
			BMessage message(ML_DEFAULT_CHANNEL_CHANGED);

			for (int32 i = 0; i < inputCount; i++) {
				media_input* input = new media_input();
				memcpy(input, &inputs[i], sizeof(*input));
				ChannelMenuItem* channelItem = new ChannelMenuItem(input,
					new BMessage(message));
				fChannelMenu->AddItem(channelItem);

				if (channelItem->DestinationID() == 0)
					channelItem->SetMarked(true);
			}
		}
	}

	if (Window())
		fChannelMenu->SetTargetForItems(BMessenger(this));
}


void
AudioSettingsView::_ShowDeskbarVolumeControl()
{
	BDeskbar deskbar;
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
}


void
AudioSettingsView::_HideDeskbarVolumeControl()
{
	BDeskbar deskbar;
	status_t status = deskbar.RemoveItem("MediaReplicant");
	if (status != B_OK) {
		fprintf(stderr, B_TRANSLATE(
			"Couldn't remove volume control in Deskbar: %s\n"),
			strerror(status));
	}
}


ChannelMenuItem*
AudioSettingsView::_ChannelMenuItemAt(int32 index)
{
	return static_cast<ChannelMenuItem*>(fChannelMenu->ItemAt(index));
}


VideoSettingsView::VideoSettingsView()
{
	BBox* defaultsBox = new BBox("defaults");
	defaultsBox->SetLabel(B_TRANSLATE("Defaults"));
	BGridView* defaultsGridView = new BGridView();
	
	BMenuField* inputMenuField = new BMenuField("inputMenuField",
		B_TRANSLATE("Video input:"), InputMenu(), NULL);

	BMenuField* outputMenuField = new BMenuField("outputMenuField",
		B_TRANSLATE("Video output:"), OutputMenu(), NULL);

	BLayoutBuilder::Grid<>(defaultsGridView)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING, 0)
		.AddMenuField(inputMenuField, 0, 0)
		.AddMenuField(outputMenuField, 0, 1)
		.Add(MakeRestartMessageView(), 0, 2, 2, 1);

	defaultsBox->AddChild(defaultsGridView);

	const char* realtimeLabel = B_TRANSLATE("Enable real-time video");
	const char*	realtimeInfo = B_TRANSLATE(
		"Enabling real-time video allows system to "
		"perform video operations as fast and smoothly as possible.  It "
		"achieves optimum performance by using more RAM.\n\n"
		"Only enable this feature if you need the lowest latency possible.");

	BLayoutBuilder::Grid<>(this)
		.SetInsets(0, 0, 0, 0)
		.Add(defaultsBox, 0, 0, 2, 1)
		.Add(MakeRealtimeBox(realtimeInfo, B_MEDIA_REALTIME_VIDEO,
			realtimeLabel), 0, 1, 2, 1)
		.Add(MakeRestartButton(), 1, 2, 1, 1)
		.SetRowWeight(1, 10);
}


void
VideoSettingsView::SetDefaultInput(const dormant_node_info* info)
{
	SettingsView::SetDefaultInput(info);
	_MediaWindow()->UpdateInputListItem(MediaListItem::VIDEO_TYPE, info);
	BMediaRoster::Roster()->SetVideoInput(*info);
}


void
VideoSettingsView::SetDefaultOutput(const dormant_node_info* info)
{
	SettingsView::SetDefaultOutput(info);
	_MediaWindow()->UpdateOutputListItem(MediaListItem::VIDEO_TYPE, info);
	BMediaRoster::Roster()->SetVideoOutput(*info);
}


BString
VideoSettingsView::UnimplementedRealtimeError()
{
	return B_TRANSLATE("Real-time video is currently unimplemented in Haiku.");
}

