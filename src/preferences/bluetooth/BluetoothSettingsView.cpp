/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI, <triedgeai@gmail.com>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BluetoothSettingsView.h"

#include "defs.h"
#include "BluetoothSettings.h"
#include "BluetoothWindow.h"
#include "ExtendedLocalDeviceView.h"

#include <bluetooth/LocalDevice.h>

#include <Box.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include <stdio.h>
#include <stdlib.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Settings view"

static const int32 kMsgSetConnectionPolicy = 'sCpo';
static const int32 kMsgSetDeviceClass = 'sDC0';
static const int32 kMsgSetInquiryTime = 'afEa';
static const int32 kMsgLocalSwitched = 'lDsW';

static const char* kAllLabel = B_TRANSLATE_MARK("From all devices");
static const char* kTrustedLabel =
	B_TRANSLATE_MARK("Only from trusted devices");
static const char* kAlwaysLabel = B_TRANSLATE_MARK("Always ask");

static const char* kDesktopLabel = B_TRANSLATE_MARK("Desktop");
static const char* kServerLabel = B_TRANSLATE_MARK("Server");
static const char* kLaptopLabel = B_TRANSLATE_MARK("Laptop");
static const char* kHandheldLabel = B_TRANSLATE_MARK("Handheld");
static const char* kPhoneLabel = B_TRANSLATE_MARK("Smart phone");

//	#pragma mark -

BluetoothSettingsView::BluetoothSettingsView(const char* name)
	:
	BView(name, 0),
	fLocalDevicesMenu(NULL)
{
	fSettings.Load();

	_BuildConnectionPolicy();
	fPolicyMenuField = new BMenuField("policy",
		B_TRANSLATE("Incoming connections policy:"), fPolicyMenu);

	fInquiryTimeControl = new BSlider("time",
		B_TRANSLATE("Default inquiry time:"), new BMessage(kMsgSetInquiryTime),
		0, 255, B_HORIZONTAL);
	fInquiryTimeControl->SetLimitLabels(B_TRANSLATE("15 secs"),
		B_TRANSLATE("61 secs"));
	fInquiryTimeControl->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fInquiryTimeControl->SetHashMarkCount(255 / 15);
	fInquiryTimeControl->SetEnabled(true);

	fExtDeviceView = new ExtendedLocalDeviceView(NULL);

	// localdevices menu
	_BuildLocalDevicesMenu();
	fLocalDevicesMenuField = new BMenuField("devices",
		B_TRANSLATE("Local devices found on system:"),
		fLocalDevicesMenu);

	if (ActiveLocalDevice != NULL) {
		fExtDeviceView->SetLocalDevice(ActiveLocalDevice);
		fExtDeviceView->SetEnabled(true);

		DeviceClass rememberedClass = ActiveLocalDevice->GetDeviceClass();

		if (!rememberedClass.IsUnknownDeviceClass())
			fSettings.Data.LocalDeviceClass = rememberedClass;
	}

	// hinting menu
	_BuildClassMenu();
	fClassMenuField = new BMenuField("class", B_TRANSLATE("Identify host as:"),
		fClassMenu);

	BLayoutBuilder::Grid<>(this, 0)
		.SetInsets(10)
		.Add(fClassMenuField->CreateLabelLayoutItem(), 0, 0)
		.Add(fClassMenuField->CreateMenuBarLayoutItem(), 1, 0)

		.Add(fPolicyMenuField->CreateLabelLayoutItem(), 0, 1)
		.Add(fPolicyMenuField->CreateMenuBarLayoutItem(), 1, 1)

		.Add(fInquiryTimeControl, 0, 2, 2)

		.Add(fLocalDevicesMenuField->CreateLabelLayoutItem(), 0, 5)
		.Add(fLocalDevicesMenuField->CreateMenuBarLayoutItem(), 1, 5)

		.Add(fExtDeviceView, 0, 6, 2)
	.End();
}


BluetoothSettingsView::~BluetoothSettingsView()
{
	fSettings.Save();
}


void
BluetoothSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fPolicyMenu->SetTargetForItems(this);
	fClassMenu->SetTargetForItems(this);
	fLocalDevicesMenu->SetTargetForItems(this);
	fInquiryTimeControl->SetTarget(this);
}


void
BluetoothSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgLocalSwitched:
		{
			LocalDevice* lDevice;

			if (message->FindPointer("LocalDevice",
				(void**)&lDevice) == B_OK) {

				_MarkLocalDevice(lDevice);
			}

			break;
		}
		// TODO: To be fixed. :)

		/*
		case kMsgSetConnectionPolicy:
		{
			//uint8 Policy;
			//if (message->FindInt8("Policy", (int8*)&Policy) == B_OK)
			break;
		}

		case kMsgSetInquiryTime:
		{
			break;
		}
		*/
		case kMsgSetDeviceClass:
		{
			uint8 deviceClass;

			if (message->FindInt8("DeviceClass",
				(int8*)&deviceClass) == B_OK) {

				if (deviceClass == 5)
					_SetDeviceClass(2, 3, 0x72);
				else
					_SetDeviceClass(1, deviceClass, 0x72);
			}

			break;
		}
		case kMsgRefresh:
		{
			_BuildLocalDevicesMenu();
			fLocalDevicesMenu->SetTargetForItems(this);

			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
BluetoothSettingsView::_SetDeviceClass(uint8 major, uint8 minor,
	uint16 service)
{
	bool haveRun = true;

	fSettings.Data.LocalDeviceClass.SetRecord(major, minor, service);

	if (ActiveLocalDevice != NULL)
		ActiveLocalDevice->SetDeviceClass(fSettings.Data.LocalDeviceClass);
	else
		haveRun = false;

	return haveRun;
}


void
BluetoothSettingsView::_BuildConnectionPolicy()
{
	BMessage* message = NULL;
	BMenuItem* item = NULL;

	fPolicyMenu = new BPopUpMenu(B_TRANSLATE("Policy" B_UTF8_ELLIPSIS));

	message = new BMessage(kMsgSetConnectionPolicy);
	message->AddInt8("Policy", 1);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kAllLabel), message);
	fPolicyMenu->AddItem(item);

	message = new BMessage(kMsgSetConnectionPolicy);
	message->AddInt8("Policy", 2);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kTrustedLabel), message);
	fPolicyMenu->AddItem(item);

	message = new BMessage(kMsgSetConnectionPolicy);
	message->AddInt8("Policy", 3);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kAlwaysLabel), NULL);
	fPolicyMenu->AddItem(item);
}

void
BluetoothSettingsView::_BuildClassMenu()
{
	BMessage* message = NULL;
	BMenuItem* item = NULL;

	fClassMenu = new BPopUpMenu(B_TRANSLATE("Identify us as" B_UTF8_ELLIPSIS));

	message = new BMessage(kMsgSetDeviceClass);
	message->AddInt8("DeviceClass", 1);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kDesktopLabel), message);
	fClassMenu->AddItem(item);

	if (fSettings.Data.LocalDeviceClass.MajorDeviceClass() == 1 &&
		fSettings.Data.LocalDeviceClass.MinorDeviceClass() == 1)
			item->SetMarked(true);

	message = new BMessage(kMsgSetDeviceClass);
	message->AddInt8("DeviceClass", 2);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kServerLabel), message);
	fClassMenu->AddItem(item);

	if (fSettings.Data.LocalDeviceClass.MajorDeviceClass() == 1 &&
		fSettings.Data.LocalDeviceClass.MinorDeviceClass() == 2)
			item->SetMarked(true);

	message = new BMessage(kMsgSetDeviceClass);
	message->AddInt8("DeviceClass", 3);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kLaptopLabel), message);
	fClassMenu->AddItem(item);

	if (fSettings.Data.LocalDeviceClass.MajorDeviceClass() == 1 &&
		fSettings.Data.LocalDeviceClass.MinorDeviceClass() == 3)
			item->SetMarked(true);

	message = new BMessage(kMsgSetDeviceClass);
	message->AddInt8("DeviceClass", 4);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kHandheldLabel), message);
	fClassMenu->AddItem(item);

	if (fSettings.Data.LocalDeviceClass.MajorDeviceClass() == 1 &&
		fSettings.Data.LocalDeviceClass.MinorDeviceClass() == 4)
			item->SetMarked(true);

	message = new BMessage(kMsgSetDeviceClass);
	message->AddInt8("DeviceClass", 5);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kPhoneLabel), message);
	fClassMenu->AddItem(item);

	if (fSettings.Data.LocalDeviceClass.MajorDeviceClass() == 2 &&
		fSettings.Data.LocalDeviceClass.MinorDeviceClass() == 3)
			item->SetMarked(true);
}


void
BluetoothSettingsView::_BuildLocalDevicesMenu()
{
	LocalDevice* lDevice;

	if (!fLocalDevicesMenu)
		fLocalDevicesMenu = new BPopUpMenu(B_TRANSLATE("Pick device"
			B_UTF8_ELLIPSIS));

	while (fLocalDevicesMenu->CountItems() > 0) {
		BMenuItem* item = fLocalDevicesMenu->RemoveItem((int32)0);

		if (item != NULL) {
			delete item;
		}
	}

	ActiveLocalDevice = NULL;

	for (uint32 i = 0; i < LocalDevice::GetLocalDeviceCount(); i++) {
		lDevice = LocalDevice::GetLocalDevice();

		if (lDevice != NULL) {
			BMessage* message = new BMessage(kMsgLocalSwitched);
			message->AddPointer("LocalDevice", lDevice);

			BMenuItem* item = new BMenuItem(
				(lDevice->GetFriendlyName().String()), message);

			if (bdaddrUtils::Compare(lDevice->GetBluetoothAddress(),
				fSettings.Data.PickedDevice)) {

				item->SetMarked(true);
				ActiveLocalDevice = lDevice;
			}

			fLocalDevicesMenu->AddItem(item);
		}
	}
}

void
BluetoothSettingsView::_MarkLocalDevice(LocalDevice* lDevice)
{
	// TODO: Device integrity should be rechecked.

	fExtDeviceView->SetLocalDevice(lDevice);
	fExtDeviceView->SetEnabled(true);
	ActiveLocalDevice = lDevice;
	fSettings.Data.PickedDevice = lDevice->GetBluetoothAddress();
}
