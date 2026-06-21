/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Fredrik Modéen <fredrik_at_modeen.se>
 */

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Messenger.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <LayoutBuilder.h>
#include <SpaceLayoutItem.h>

#include <PincodeWindow.h>
#include <bluetooth/RemoteDevice.h>

#include "BluetoothWindow.h"
#include "defs.h"
#include "InquiryPanel.h"
#include "RemoteDevicesView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Remote devices"

static const uint32 kMsgAddDevices = 'ddDv';
static const uint32 kMsgRemoveDevice = 'rmDv';
static const uint32 kMsgPairDevice = 'trDv';
static const uint32 kMsgDisconnectDevice = 'dsDv';
static const uint32 kMsgConnComplete = 'cmDv';
static const uint32 kMsgCancelConn = 'cnDv';
//static const uint32 kMsgBlockDevice = 'blDv';
//static const uint32 kMsgRefreshDevices = 'rfDv';


class DeviceListView : public BListView
{
public:
	DeviceListView(const char* name, list_view_type type)
		:
		BListView(name, type)
	{
	}


	void
	SelectionChanged()
	{
		RemoteDevicesView* remoteDevicesView = static_cast<RemoteDevicesView*>(Parent()->Parent());
		remoteDevicesView->DeviceSelected();
	}
};


RemoteDevicesView::RemoteDevicesView(const char* name, uint32 flags)
 :	BView(name, flags)
{
	addButton = new BButton("add", B_TRANSLATE("Add" B_UTF8_ELLIPSIS),
		new BMessage(kMsgAddDevices));

	removeButton = new BButton("remove", B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveDevice));

	pairButton = new BButton("pair", B_TRANSLATE("Pair" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPairDevice));
	pairButton->SetEnabled(false);

	disconnectButton = new BButton("disconnect", B_TRANSLATE("Disconnect"),
		new BMessage(kMsgDisconnectDevice));
	disconnectButton->SetEnabled(false);

	cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(kMsgCancelConn));
	cancelButton->SetEnabled(false);

	/*
		blockButton = new BButton("block", B_TRANSLATE("As blocked"),
			new BMessage(kMsgBlockDevice));

		//TODO:Here use GetFriendlyName(true)
		availButton = new BButton("check", B_TRANSLATE("Refresh" B_UTF8_ELLIPSIS),
			new BMessage(kMsgRefreshDevices));
	*/
	// Set up device list
	fDeviceList = new DeviceListView("DeviceList", B_SINGLE_SELECTION_LIST);

	fScrollView = new BScrollView("ScrollView", fDeviceList, 0, false, true);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.SetInsets(5)
		.Add(fScrollView)
		//.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
		.AddGroup(B_VERTICAL)
			.SetInsets(0, 15, 0, 15)
			.Add(addButton)
			.Add(removeButton)
			.AddGlue()
//			.Add(availButton)
	//		.AddGlue()
			.Add(pairButton)
			.Add(disconnectButton)
			.Add(cancelButton)
//			.Add(blockButton)
			.AddGlue()
		.End()
	.End();
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
	cancelButton->SetTarget(this);
//	blockButton->SetTarget(this);
//	availButton->SetTarget(this);

	LoadSettings();
	fDeviceList->Select(0);
}


void
RemoteDevicesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddDevices:
		{
			InquiryPanel* inquiryPanel= new InquiryPanel(
				BRect(100, 100, 450, 450), ActiveLocalDevice);
			inquiryPanel->Show();
			break;
		}

		case kMsgRemoveDevice:
			fDeviceList->RemoveItem(fDeviceList->CurrentSelection(0));
			break;
		case kMsgAddToRemoteList:
		{
			DeviceListItem* device = NULL;
			message->FindPointer("device", (void**)&device);
			bool isDuplicate = false;

			// check the list for duplicates
			for (int32 i = 0; i < fDeviceList->CountItems(); i++) {
				DeviceListItem* existingDevice
					= static_cast<DeviceListItem*>(fDeviceList->ItemAt(i));

				if (DeviceListItem::Compare(&existingDevice, &device)) {
					isDuplicate = true;
					break;
				}
			}

			if (!isDuplicate) {
				fDeviceList->AddItem((BListItem*)device);
				fDeviceList->Invalidate();
			} else {
				delete device;
			}

			break;
		}

		case kMsgPairDevice:
		{
			DeviceListItem* device = static_cast<DeviceListItem*>(fDeviceList
				->ItemAt(fDeviceList->CurrentSelection(0)));
			if (device == NULL)
				break;


			RemoteDevice* remote = dynamic_cast<RemoteDevice*>(device->Device());
			if (remote == NULL)
				break;

			fConnectingDeviceItem = device;
			fConnectingDeviceItem->SetConnState(RD_CONNECTING);
			DeviceSelected();

			remote->Connect();
			break;
		}

		case BT_MSG_CONN_COMPLETED:
		{
			uint8 status;
			message->FindUInt8("status", &status);
			if (status == BT_OK)
				fConnectingDeviceItem->SetConnState(RD_CONNECTED);
			else
				fConnectingDeviceItem->SetConnState(RD_DISCONNECTED);

			DeviceSelected();

			fConnectingDeviceItem = NULL;
			break;
		}

		case kMsgDisconnectDevice:
		{
			DeviceListItem* device = static_cast<DeviceListItem*>(fDeviceList
				->ItemAt(fDeviceList->CurrentSelection(0)));
			if (device == NULL)
				break;

			RemoteDevice* remote = dynamic_cast<RemoteDevice*>(device->Device());
			if (remote == NULL)
				break;
			fConnectingDeviceItem = device;
			remote->Disconnect();

			break;
		}

		case BT_MSG_CONN_FAILED:
		case BT_MSG_DISCONN_COMPLETED:
		{
			fConnectingDeviceItem->SetConnState(RD_DISCONNECTED);
			DeviceSelected();

			fConnectingDeviceItem = NULL;
			break;
		}

		case kMsgCancelConn:
		{
			fConnectingDeviceItem->Device()->CancelConnection();
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
RemoteDevicesView::DeviceSelected()
{
	DeviceListItem* device = static_cast<DeviceListItem*>(fDeviceList
				->ItemAt(fDeviceList->CurrentSelection(0)));
	if (device == NULL)
		return;

	switch (device->GetConnState()) {
		case RD_CONNECTING:
			pairButton->SetEnabled(false);
			disconnectButton->SetEnabled(false);
			cancelButton->SetEnabled(true);
			break;
		case RD_CONNECTED:
			pairButton->SetEnabled(false);
			disconnectButton->SetEnabled(true);
			cancelButton->SetEnabled(false);
			break;
		case RD_DISCONNECTED:
			pairButton->SetEnabled(true);
			disconnectButton->SetEnabled(false);
			cancelButton->SetEnabled(false);
			break;
	}
}


void
RemoteDevicesView::LoadSettings(void)
{

}


bool
RemoteDevicesView::IsDefaultable(void)
{
	return true;
}

