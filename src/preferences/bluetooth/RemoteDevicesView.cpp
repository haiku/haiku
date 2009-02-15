/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "RemoteDevicesView.h"

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <GroupLayoutBuilder.h>
#include <Messenger.h>
#include <Path.h>
#include <SpaceLayoutItem.h>

#include <stdio.h>

#include "InquiryPanel.h"
#include "BluetoothWindow.h"
#include "defs.h"

static const uint32 kMsgAddDevices = 'ddDv';




RemoteDevicesView::RemoteDevicesView(const char *name, uint32 flags)
 :	BView(name, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	addButton = new BButton(BRect(5,5,5,5), "add", "Add" B_UTF8_ELLIPSIS, 
										new BMessage(kMsgAddDevices), B_FOLLOW_RIGHT);
	
	BButton* removeButton = new BButton(BRect(5,5,5,5), "remove", "Remove", 
										new BMessage(kMsgAddDevices), B_FOLLOW_RIGHT);

	BButton* trustButton = new BButton(BRect(5,5,5,5), "trust", "As Trusted", 
										new BMessage(kMsgAddDevices), B_FOLLOW_RIGHT);


	BButton* blockButton = new BButton(BRect(5,5,5,5), "trust", "As Blocked", 
										new BMessage(kMsgAddDevices), B_FOLLOW_RIGHT);


	BButton* availButton = new BButton(BRect(5,5,5,5), "check", "Refresh" B_UTF8_ELLIPSIS, 
										new BMessage(kMsgAddDevices), B_FOLLOW_RIGHT);

	
		
	// Set up list of color attributes
	fAttrList = new BListView("AttributeList", B_SINGLE_SELECTION_LIST);
	
	fScrollView = new BScrollView("ScrollView", fAttrList, 0, false, true);
	fScrollView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLayout(new BGroupLayout(B_VERTICAL));

	// TODO: Make list view and scroller use all the additional height
	// available!
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10)
		.Add(fScrollView)
		//.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
		.Add(BGroupLayoutBuilder(B_VERTICAL)						
						.Add(addButton)
						.Add(removeButton)						
						.AddGlue()
						.Add(availButton)
						.AddGlue()
						.Add(trustButton)
						.Add(blockButton)
						.AddGlue()
						.SetInsets(0, 15, 0, 15)
			)
		.SetInsets(5, 5, 5, 100)
	);

	fAttrList->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));
}

RemoteDevicesView::~RemoteDevicesView(void)
{

}

void
RemoteDevicesView::AttachedToWindow(void)
{
	fAttrList->SetTarget(this);
	addButton->SetTarget(this);

	LoadSettings();
	fAttrList->Select(0);
}

void
RemoteDevicesView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color *color;
		ssize_t size;
		
		if (msg->FindData("RGBColor", (type_code)'RGBC', (const void**)&color, &size) == B_OK) {

		}
	}

	switch(msg->what) {
		case kMsgAddDevices:
		{
			InquiryPanel* iPanel = new InquiryPanel(BRect(0,0,50,50));
			iPanel->Show();
		}
		break;
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void RemoteDevicesView::LoadSettings(void)
{

}

bool RemoteDevicesView::IsDefaultable(void)
{
	return true;
}

