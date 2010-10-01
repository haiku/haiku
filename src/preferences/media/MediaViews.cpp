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


BarView::BarView()
	:
	BView("barView", B_WILL_DRAW ),
 	fDisplay(true)
{
}


void
BarView::Draw(BRect updateRect)
{
	BRect r = Bounds();

	if (fDisplay) {
		// Display the 3D Look Divider Bar
		SetHighColor(140,140,140,0);
		StrokeLine(BPoint(r.left,r.top),BPoint(r.right,r.top));
		SetHighColor(255,255,255,0);
		StrokeLine(BPoint(r.left,r.bottom),BPoint(r.right,r.bottom));
	} else {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		StrokeLine(BPoint(r.left,r.top),BPoint(r.right,r.top));
		StrokeLine(BPoint(r.left,r.bottom),BPoint(r.right,r.bottom));
	}
}


SettingsView::SettingsView (bool isVideo)
	:
	BView("SettingsView", B_WILL_DRAW | B_SUPPORTS_LAYOUT),
	fIsVideo(isVideo)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BBox* defaultsBox = new BBox("defaults");
	defaultsBox->SetLabel(fIsVideo ? B_TRANSLATE("Default nodes")
		: B_TRANSLATE("Defaults"));

	// create the default box
	BGroupLayout* defaultBoxLayout = new BGroupLayout(B_VERTICAL, 5);
	defaultBoxLayout->SetInsets(10,10,10,10);
	defaultsBox->SetLayout(defaultBoxLayout);
	defaultBoxLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(5));

	BGroupView* inputField = new BGroupView(B_HORIZONTAL);
	BGroupView* outputField = new BGroupView(B_HORIZONTAL);
	defaultsBox->GetLayout()->AddView(inputField);
	defaultsBox->GetLayout()->AddView(outputField);

	float divider = StringWidth(fIsVideo ? B_TRANSLATE("Video output:")
		: B_TRANSLATE("Audio output:")) + 5;
	fMenu1 = new BPopUpMenu(B_TRANSLATE("<none>"));
	fMenu1->SetLabelFromMarked(true);
	BMenuField* menuField1 = new BMenuField("menuField1",
		fIsVideo ? B_TRANSLATE("Video input:")
			: B_TRANSLATE("Audio input:"), fMenu1, NULL);
	menuField1->SetDivider(divider);

	fMenu2 = new BPopUpMenu(B_TRANSLATE("<none>"));
	fMenu2->SetLabelFromMarked(true);
	BMenuField* menuField2 = new BMenuField("menuField2",
		fIsVideo ? B_TRANSLATE("Video output:")
			: B_TRANSLATE("Audio output:"), fMenu2, NULL);
	menuField2->SetDivider(divider);

	inputField->GroupLayout()->AddView(menuField1);
	outputField->GroupLayout()->AddView(menuField2);

	BMenuField* menuField3 = NULL;
	if (!fIsVideo) {
		fMenu3 = new BPopUpMenu(B_TRANSLATE("<none>"));
		fMenu3->SetLabelFromMarked(true);
		menuField3 = new BMenuField("menuField3",
			B_TRANSLATE("Channel:"), fMenu3, NULL);
		outputField->GroupLayout()->AddView(menuField3);
		menuField3->SetDivider(StringWidth(B_TRANSLATE("Channel:"))+5);
	}

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

	// create the bottom line: volumen in deskbar checkbox and restart button
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
SettingsView::AddNodes(BList& list, bool isInput)
{
	BMenu* menu = isInput ? fMenu1 : fMenu2;
	void* item;
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete static_cast<dormant_node_info*>(item);

	BMessage message(ML_DEFAULT_CHANGE);
	message.AddBool("isVideo", fIsVideo);
	message.AddBool("isInput", isInput);

	for (int32 i = 0; i < list.CountItems(); i++) {
		dormant_node_info* info
			= static_cast<dormant_node_info*>(list.ItemAt(i));
		menu->AddItem(new SettingsItem(info, new BMessage(message)));
	}
}


void
SettingsView::SetDefault(dormant_node_info &info, bool isInput, int32 outputID)
{
	BMenu* menu = isInput ? fMenu1 : fMenu2;

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
		while ((item = fMenu3->RemoveItem((int32)0)) != NULL)
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
					fMenu3->AddItem(item);
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

