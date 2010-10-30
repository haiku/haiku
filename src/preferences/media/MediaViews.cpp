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

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Entry.h>
#include <GridView.h>
#include <GroupView.h>
#include <Locale.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include <stdio.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Media views"


SettingsView::SettingsView (bool isVideo)
	:
	BView("SettingsView", B_WILL_DRAW | B_SUPPORTS_LAYOUT),
	fIsVideo(isVideo)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// create the default box

	// input menu
	fInputMenu = new BPopUpMenu(B_TRANSLATE("<none>"));
	fInputMenu->SetLabelFromMarked(true);
	BMenuField* inputMenuField = new BMenuField("inputMenuField",
		fIsVideo ? B_TRANSLATE("Video input:")
			: B_TRANSLATE("Audio input:"), fInputMenu, NULL);

	// output menu
	fOutputMenu = new BPopUpMenu(B_TRANSLATE("<none>"));
	fOutputMenu->SetLabelFromMarked(true);
	BMenuField* outputMenuField = new BMenuField("outputMenuField",
		fIsVideo ? B_TRANSLATE("Video output:")
			: B_TRANSLATE("Audio output:"), fOutputMenu, NULL);

	// channel menu (audio only)
	BMenuField* channelMenuField = NULL;
	if (!fIsVideo) {
		fChannelMenu = new BPopUpMenu(B_TRANSLATE("<none>"));
		fChannelMenu->SetLabelFromMarked(true);
		channelMenuField = new BMenuField("channelMenuField",
			B_TRANSLATE("Channel:"), fChannelMenu, NULL);
		channelMenuField->SetDivider(StringWidth(B_TRANSLATE("Channel:"))+5);
	}

	BBox* defaultsBox = new BBox("defaults");
	defaultsBox->SetLabel(fIsVideo ? B_TRANSLATE("Default nodes")
		: B_TRANSLATE("Defaults"));

	// put our menus in a BGridView in our BBox, this way, the BBox makes sure
	// we have are not blocking the label.
	BGridView* defaultsGridView = new BGridView();
	defaultsBox->AddChild(defaultsGridView);

	BGridLayout* defaultsGrid = defaultsGridView->GridLayout();
	defaultsGrid->SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING);

	BLayoutItem* labelItem = inputMenuField->CreateLabelLayoutItem();
	BLayoutItem* menuItem = inputMenuField->CreateMenuBarLayoutItem();
	defaultsGrid->AddItem(labelItem, 0, 0, 1, 1);
	defaultsGrid->AddItem(menuItem, 1, 0, 3, 1);

	int32 outputMenuWidth = 3;
	if (channelMenuField) {
		outputMenuWidth = 1;
		labelItem = channelMenuField->CreateLabelLayoutItem();
		menuItem = channelMenuField->CreateMenuBarLayoutItem();
		defaultsGrid->AddItem(labelItem, 2, 1, 1, 1);
		defaultsGrid->AddItem(menuItem, 3, 1, 1, 1);
	}

	labelItem = outputMenuField->CreateLabelLayoutItem();
	menuItem = outputMenuField->CreateMenuBarLayoutItem();
	defaultsGrid->AddItem(labelItem, 0, 1, 1, 1);
	defaultsGrid->AddItem(menuItem, 1, 1, outputMenuWidth, 1);


	rgb_color red_color = {222, 32, 33};
	fRestartView = new BStringView("restartStringView",
		B_TRANSLATE("Restart the media server to apply changes."));
	fRestartView->SetHighColor(red_color);
	defaultsBox->AddChild(fRestartView);
	fRestartView->Hide();

	// create the realtime box
	BBox* realtimeBox = new BBox("realtime");
	realtimeBox->SetLabel(B_TRANSLATE("Real-time"));

	BMessage* message = new BMessage(ML_ENABLE_REAL_TIME);
	message->AddBool("isVideo", fIsVideo);
	fRealtimeCheckBox = new BCheckBox("realtimeCheckBox",
		fIsVideo ? B_TRANSLATE("Enable real-time video")
			: B_TRANSLATE("Enable real-time audio"),
		message);

	uint32 flags;
	BMediaRoster::Roster()->GetRealtimeFlags(&flags);
	if (flags & (fIsVideo ? B_MEDIA_REALTIME_VIDEO : B_MEDIA_REALTIME_AUDIO))
		fRealtimeCheckBox->SetValue(B_CONTROL_ON);

	BTextView* textView = new BTextView("stringView");
	textView->Insert(fIsVideo ? B_TRANSLATE(
		"Enabling real-time video allows system to "
		"perform video operations as fast and smoothly as possible.  It "
		"achieves optimum performance by using more RAM."
		"\n\nOnly enable this feature if you need the lowest latency possible.")
		: B_TRANSLATE(
		"Enabling real-time audio allows system to record and play audio "
		"as fast as possible.  It achieves this performance by using more"
		" CPU and RAM.\n\nOnly enable this feature if you need the lowest"
		" latency possible."));

	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGroupLayout* realtimeBoxLayout = new BGroupLayout(B_VERTICAL, 5);
	realtimeBoxLayout->SetInsets(10,10,10,10);
	realtimeBox->SetLayout(realtimeBoxLayout);

	realtimeBoxLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(5));
	realtimeBoxLayout->AddView(fRealtimeCheckBox);
	realtimeBoxLayout->AddView(textView);

	// create the bottom line: volume in deskbar checkbox and restart button
	BGroupView* bottomView = new BGroupView(B_HORIZONTAL);
	BButton* restartButton = new BButton("restartButton",
		B_TRANSLATE("Restart media services"),
		new BMessage(ML_RESTART_MEDIA_SERVER));

	if (!fIsVideo) {
		fVolumeCheckBox = new BCheckBox("volumeCheckBox",
			B_TRANSLATE("Show volume control on Deskbar"),
			new BMessage(ML_SHOW_VOLUME_CONTROL));
		bottomView->GroupLayout()->AddView(fVolumeCheckBox);
		if (BDeskbar().HasItem("MediaReplicant"))
			fVolumeCheckBox->SetValue(B_CONTROL_ON);
	}
	else{
		bottomView->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());
	}
	bottomView->GroupLayout()->AddView(restartButton);

	// compose all stuff
	BGroupLayout* rootlayout = new BGroupLayout(B_VERTICAL, 5);
	SetLayout(rootlayout);

	rootlayout->AddView(defaultsBox);
	rootlayout->AddView(realtimeBox);
	rootlayout->AddView(bottomView);
}


void
SettingsView::AddNodes(NodeList& list, bool isInput)
{
	BMenu* menu = isInput ? fInputMenu : fOutputMenu;

	for (BMenuItem* item; (item = menu->RemoveItem((int32)0)) != NULL;)
		delete item;

	BMessage message(ML_DEFAULT_CHANGE);
	message.AddBool("isVideo", fIsVideo);
	message.AddBool("isInput", isInput);

	for (int32 i = 0; i < list.CountItems(); i++) {
		dormant_node_info* info = list.ItemAt(i);
		menu->AddItem(new SettingsItem(info, new BMessage(message)));
	}
}


void
SettingsView::SetDefault(dormant_node_info &info, bool isInput, int32 outputID)
{
	BMenu* menu = isInput ? fInputMenu : fOutputMenu;

	for (int32 i = 0; i < menu->CountItems(); i++) {
		SettingsItem* item = static_cast<SettingsItem*>(menu->ItemAt(i));
		if (item->fInfo && item->fInfo->addon == info.addon
			&& item->fInfo->flavor_id == info.flavor_id) {
			item->SetMarked(true);
			break;
		}
	}

	if (!fIsVideo && !isInput && outputID >= 0) {
		BMenuItem* item;
		while ((item = fChannelMenu->RemoveItem((int32)0)) != NULL)
			delete item;

		BMediaRoster* roster = BMediaRoster::Roster();
		media_node node;
		media_node_id node_id;
		status_t err;
		if (roster->GetInstancesFor(info.addon, info.flavor_id,
			&node_id) != B_OK) {
			err = roster->InstantiateDormantNode(info, &node, 
				B_FLAVOR_IS_GLOBAL);
		} else {
			err = roster->GetNodeFor(node_id, &node);
		}

		if (err == B_OK) {
			media_input inputs[16];
			int32 inputCount = 16;
			if (roster->GetAllInputsFor(node, inputs, 16, &inputCount)==B_OK) {
				BMessage message(ML_DEFAULTOUTPUT_CHANGE);

				for (int32 i = 0; i < inputCount; i++) {
					media_input* input = new media_input();
					memcpy(input, &inputs[i], sizeof(*input));
					item = new Settings2Item(&info, input,
						new BMessage(message));
					fChannelMenu->AddItem(item);
					if (inputs[i].destination.id == outputID)
						item->SetMarked(true);
				}
			}
		}
	}
}


SettingsItem::SettingsItem(dormant_node_info* info, BMessage* message,
		char shortcut, uint32 modifiers)
	:
	BMenuItem(info->name, message, shortcut, modifiers),
	fInfo(info)
{

}


status_t
SettingsItem::Invoke(BMessage* message)
{
	if (IsMarked())
		return B_OK;
	return BMenuItem::Invoke(message);
}


Settings2Item::Settings2Item(dormant_node_info* info, media_input* input,
		BMessage* message, char shortcut, uint32 modifiers)
	:
	BMenuItem(input->name, message, shortcut, modifiers),
	fInfo(info),
	fInput(input)
{
}


Settings2Item::~Settings2Item()
{
	delete fInput;
}


status_t
Settings2Item::Invoke(BMessage* message)
{
	if (IsMarked())
		return B_OK;
	return BMenuItem::Invoke(message);
}

