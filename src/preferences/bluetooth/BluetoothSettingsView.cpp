/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI, <triedgeai@gmail.com>
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Fredrik Modéen <fredrik_at_modeen.se>
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
#include <OptionPopUp.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include <stdio.h>
#include <stdlib.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Settings view"

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
	fSettings.LoadSettings();

	fPolicyMenu = new BOptionPopUp("policy",
		B_TRANSLATE("Incoming connections policy:"),
		new BMessage(kMsgSetConnectionPolicy));
	fPolicyMenu->AddOption(B_TRANSLATE_NOCOLLECT(kAllLabel), 1);
	fPolicyMenu->AddOption(B_TRANSLATE_NOCOLLECT(kTrustedLabel), 2);
	fPolicyMenu->AddOption(B_TRANSLATE_NOCOLLECT(kAlwaysLabel), 3);

	fPolicyMenu->SetValue(fSettings.Policy());

	BString label(B_TRANSLATE("Default inquiry time:"));
	label <<  " " << fSettings.InquiryTime();
	fInquiryTimeControl = new BSlider("time", label.String()
		, new BMessage(kMsgSetInquiryTime), 15, 61, B_HORIZONTAL);
	fInquiryTimeControl->SetLimitLabels(B_TRANSLATE("15 secs"),
		B_TRANSLATE("61 secs"));
	fInquiryTimeControl->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fInquiryTimeControl->SetHashMarkCount(20);
	fInquiryTimeControl->SetEnabled(true);
	fInquiryTimeControl->SetValue(fSettings.InquiryTime());

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
			fSettings.SetLocalDeviceClass(rememberedClass);
	}

	fClassMenu = new BOptionPopUp("DeviceClass", B_TRANSLATE("Identify host as:"),
		new BMessage(kMsgSetDeviceClass));
	fClassMenu->AddOption(B_TRANSLATE_NOCOLLECT(kDesktopLabel), 1);
	fClassMenu->AddOption(B_TRANSLATE_NOCOLLECT(kServerLabel), 2);
	fClassMenu->AddOption(B_TRANSLATE_NOCOLLECT(kLaptopLabel), 3);
	fClassMenu->AddOption(B_TRANSLATE_NOCOLLECT(kHandheldLabel), 4);
	fClassMenu->AddOption(B_TRANSLATE_NOCOLLECT(kPhoneLabel), 5);

	fClassMenu->SetValue(_GetClassForMenu());

	BLayoutBuilder::Grid<>(this, 0)
		.SetInsets(10)
		.Add(fClassMenu, 0, 0)
		.Add(fPolicyMenu, 0, 1)

		.Add(fInquiryTimeControl, 0, 2, 2)

		.Add(fLocalDevicesMenuField->CreateLabelLayoutItem(), 0, 5)
		.Add(fLocalDevicesMenuField->CreateMenuBarLayoutItem(), 1, 5)

		.Add(fExtDeviceView, 0, 6, 2)
	.End();
}


BluetoothSettingsView::~BluetoothSettingsView()
{
	fSettings.SaveSettings();
}


void
BluetoothSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fLocalDevicesMenu->SetTargetForItems(this);
	fInquiryTimeControl->SetTarget(this);
}


void
BluetoothSettingsView::MessageReceived(BMessage* message)
{
	//message->PrintToStream();
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

		case kMsgSetConnectionPolicy:
		{
			int32 policy;
			if (message->FindInt32("be:value", (int32*)&policy) == B_OK) {
				fSettings.SetPolicy(policy);
			}
			break;
		}

		case kMsgSetInquiryTime:
		{
			fSettings.SetInquiryTime(fInquiryTimeControl->Value());
			BString label(B_TRANSLATE("Default inquiry time:"));
			label <<  " " << fInquiryTimeControl->Value();
			fInquiryTimeControl->SetLabel(label.String());
			break;
		}

		case kMsgSetDeviceClass:
		{
			int32 deviceClass;
			if (message->FindInt32("be:value",
				(int32*)&deviceClass) == B_OK) {

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

	fSettings.SetLocalDeviceClass(DeviceClass(major, minor, service));

	if (ActiveLocalDevice != NULL)
		ActiveLocalDevice->SetDeviceClass(fSettings.LocalDeviceClass());
	else
		haveRun = false;

	return haveRun;
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
				fSettings.PickedDevice())) {

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
	fSettings.SetPickedDevice(lDevice->GetBluetoothAddress());
}


int
BluetoothSettingsView::_GetClassForMenu()
{
	int deviceClass =
			fSettings.LocalDeviceClass().MajorDeviceClass()+
			fSettings.LocalDeviceClass().MinorDeviceClass();

	// As of now we only support MajorDeviceClass = 1 and MinorDeviceClass 1-4
	// and MajorDeviceClass = 2 and MinorDeviceClass 3.
	if (fSettings.LocalDeviceClass().MajorDeviceClass() == 1
			&& (fSettings.LocalDeviceClass().MinorDeviceClass() > 0
			&& fSettings.LocalDeviceClass().MinorDeviceClass() < 5))
		deviceClass -= 1;

	return deviceClass;
}
