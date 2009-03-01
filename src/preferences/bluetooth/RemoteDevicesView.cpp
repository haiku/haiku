/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <stdio.h>

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <GroupLayoutBuilder.h>
#include <Messenger.h>
#include <Path.h>
#include <SpaceLayoutItem.h>


#include "defs.h"
#include "InquiryPanel.h"
#include "BluetoothWindow.h"

#include "RemoteDevicesView.h"

static const uint32 kMsgAddDevices = 'ddDv';
static const uint32 kMsgRemoveDevice = 'rmDv';
static const uint32 kMsgTrustDevice = 'trDv';
static const uint32 kMsgBlockDevice = 'blDv';
static const uint32 kMsgRefreshDevices = 'rfDv';

RemoteDevicesView::RemoteDevicesView(const char *name, uint32 flags)
 :	BView(name, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	addButton = new BButton("add", "Add" B_UTF8_ELLIPSIS, 
										new BMessage(kMsgAddDevices));
	
	removeButton = new BButton("remove", "Remove", 
										new BMessage(kMsgRemoveDevice));

	trustButton = new BButton("trust", "As Trusted", 
										new BMessage(kMsgTrustDevice));


	blockButton = new BButton("trust", "As Blocked", 
										new BMessage(kMsgBlockDevice));


	availButton = new BButton("check", "Refresh" B_UTF8_ELLIPSIS, 
										new BMessage(kMsgRefreshDevices));

	
		
	// Set up device list
	fDeviceList = new BListView("DeviceList", B_SINGLE_SELECTION_LIST);
	
	fScrollView = new BScrollView("ScrollView", fDeviceList, 0, false, true);
	fScrollView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLayout(new BGroupLayout(B_VERTICAL));

	// TODO: use all the additional height
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

	fDeviceList->SetSelectionMessage(NULL);
}

RemoteDevicesView::~RemoteDevicesView(void)
{

}

void
RemoteDevicesView::AttachedToWindow(void)
{
	fDeviceList->SetTarget(this);
	addButton->SetTarget(this);
	removeButton->SetTarget(this);
	trustButton->SetTarget(this);
	blockButton->SetTarget(this);
	availButton->SetTarget(this);

	LoadSettings();
	fDeviceList->Select(0);
}

void
RemoteDevicesView::MessageReceived(BMessage *msg)
{
	printf("what = %ld\n", msg->what);
	switch(msg->what) {
		case kMsgAddDevices:
		{
			InquiryPanel* iPanel = new InquiryPanel(BRect(100,100,450,450), ActiveLocalDevice);
			iPanel->Show();
		}
		break;
		case kMsgRemoveDevice:
			printf("kMsgRemoveDevice: %ld\n", fDeviceList->CurrentSelection(0));
			fDeviceList->RemoveItem(fDeviceList->CurrentSelection(0));
		break;
		case kMsgAddToRemoteList:
		{
			BListItem* device;
			msg->FindPointer("device", (void**)&device);
			fDeviceList->AddItem(device);
			fDeviceList->Invalidate();
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

