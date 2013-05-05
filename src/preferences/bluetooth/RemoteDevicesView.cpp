/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Messenger.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>

#include <PincodeWindow.h>
#include <bluetooth/RemoteDevice.h>

#include "BluetoothWindow.h"
#include "defs.h"
#include "DeviceListItem.h"
#include "InquiryPanel.h"
#include "RemoteDevicesView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Remote devices"

static const uint32 kMsgAddDevices = 'ddDv';
static const uint32 kMsgRemoveDevice = 'rmDv';
static const uint32 kMsgPairDevice = 'trDv';
static const uint32 kMsgDisconnectDevice = 'dsDv';
static const uint32 kMsgBlockDevice = 'blDv';
static const uint32 kMsgRefreshDevices = 'rfDv';

using namespace Bluetooth;

RemoteDevicesView::RemoteDevicesView(const char* name, uint32 flags)
 :	BView(name, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	addButton = new BButton("add", B_TRANSLATE("Add" B_UTF8_ELLIPSIS),
		new BMessage(kMsgAddDevices));

	removeButton = new BButton("remove", B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveDevice));

	pairButton = new BButton("pair", B_TRANSLATE("Pair" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPairDevice));

	disconnectButton = new BButton("disconnect", B_TRANSLATE("Disconnect"),
		new BMessage(kMsgDisconnectDevice));

	blockButton = new BButton("block", B_TRANSLATE("As blocked"),
		new BMessage(kMsgBlockDevice));


	availButton = new BButton("check", B_TRANSLATE("Refresh" B_UTF8_ELLIPSIS),
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
			.Add(pairButton)
			.Add(disconnectButton)
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
	pairButton->SetTarget(this);
	disconnectButton->SetTarget(this);
	blockButton->SetTarget(this);
	availButton->SetTarget(this);

	LoadSettings();
	fDeviceList->Select(0);
}


void
RemoteDevicesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddDevices:
		{
			InquiryPanel* inquiryPanel = new InquiryPanel(BRect(100, 100, 450, 450),
				ActiveLocalDevice);
			inquiryPanel->Show();
			break;
		}

		case kMsgRemoveDevice:
			printf("kMsgRemoveDevice: %ld\n", fDeviceList->CurrentSelection(0));
			fDeviceList->RemoveItem(fDeviceList->CurrentSelection(0));
			break;
		case kMsgAddToRemoteList:
		{
			BListItem* device;
			message->FindPointer("device", (void**)&device);
			fDeviceList->AddItem(device);
			fDeviceList->Invalidate();
			break;
		}

		case kMsgPairDevice:
		{
			DeviceListItem* device = static_cast<DeviceListItem*>(fDeviceList
				->ItemAt(fDeviceList->CurrentSelection(0)));
			RemoteDevice* remote = dynamic_cast<RemoteDevice*>(device->Device());

			if (remote != NULL)
				remote->Authenticate();

			break;
		}
		case kMsgDisconnectDevice:
		{
			DeviceListItem* device = static_cast<DeviceListItem*>(fDeviceList
				->ItemAt(fDeviceList->CurrentSelection(0)));
			RemoteDevice* remote = dynamic_cast<RemoteDevice*>(device->Device());

			if (remote != NULL)
				remote->Disconnect();

			break;
		}

		default:
			BView::MessageReceived(message);
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

