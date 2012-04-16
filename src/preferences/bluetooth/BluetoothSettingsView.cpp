/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Box.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include <bluetooth/LocalDevice.h>
#include "ExtendedLocalDeviceView.h"

#include "defs.h"
#include "BluetoothWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Settings view"

static const int32 kMsgSetConnectionPolicy = 'sCpo';

static const int32 kMsgSetDeviceClassDesktop = 'sDCd';
static const int32 kMsgSetDeviceClassServer = 'sDCs';
static const int32 kMsgSetDeviceClassLaptop = 'sDCl';
static const int32 kMsgSetDeviceClassHandheld = 'sDCh';
static const int32 kMsgSetDeviceClassSmartPhone = 'sDCp';

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
	: BView(name, 0),
	fLocalDevicesMenu(NULL)
{
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

	// hinting menu
	_BuildClassMenu();
	fClassMenuField = new BMenuField("class", B_TRANSLATE("Identify host as:"),
		fClassMenu);

	// localdevices menu
	_BuildLocalDevicesMenu();
	fLocalDevicesMenuField = new BMenuField("devices",
		B_TRANSLATE("Local devices found on system:"),
		fLocalDevicesMenu);

	fExtDeviceView = new ExtendedLocalDeviceView(BRect(0, 0, 5, 5), NULL);

	SetLayout(new BGroupLayout(B_VERTICAL));

	// controls pane
	AddChild(BGridLayoutBuilder(10, 10)

		.Add(fClassMenuField->CreateLabelLayoutItem(), 0, 0)
		.Add(fClassMenuField->CreateMenuBarLayoutItem(), 1, 0)

		.Add(fPolicyMenuField->CreateLabelLayoutItem(), 0, 1)
		.Add(fPolicyMenuField->CreateMenuBarLayoutItem(), 1, 1)

		.Add(BSpaceLayoutItem::CreateGlue(), 0, 2, 2)

		.Add(fInquiryTimeControl, 0, 3, 2)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 4, 2)

		.Add(fLocalDevicesMenuField->CreateLabelLayoutItem(), 0, 5)
		.Add(fLocalDevicesMenuField->CreateMenuBarLayoutItem(), 1, 5)

		.Add(fExtDeviceView, 0, 6, 2)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 7, 2)

		.SetInsets(10, 10, 10, 10)
	);

}


BluetoothSettingsView::~BluetoothSettingsView()
{

}


void
BluetoothSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fPolicyMenu->SetTargetForItems(this);
	fClassMenu->SetTargetForItems(this);
	fLocalDevicesMenu->SetTargetForItems(this);
	fInquiryTimeControl->SetTarget(this);
}


void
BluetoothSettingsView::MessageReceived(BMessage* message)
{

	DeviceClass devClass;

	switch (message->what) {
		case kMsgLocalSwitched:
		{
			LocalDevice* lDevice;
			if (message->FindPointer("LocalDevice", (void**) &lDevice) == B_OK) {
				// Device integrity should be rechecked
				fExtDeviceView->SetLocalDevice(lDevice);
				fExtDeviceView->SetEnabled(true);
				ActiveLocalDevice = lDevice;
			}
		}
		break;

		case kMsgSetDeviceClassDesktop:
		{
			devClass.SetRecord(1, 1, 0x72);
			if (ActiveLocalDevice != NULL)
				ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}

		case kMsgSetDeviceClassServer:
		{
			devClass.SetRecord(1, 2, 0x72);
			if (ActiveLocalDevice != NULL)
				ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}

		case kMsgSetDeviceClassLaptop:
		{
			devClass.SetRecord(1, 3, 0x72);
			if (ActiveLocalDevice != NULL)
				ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}

		case kMsgSetDeviceClassHandheld:
		{
			devClass.SetRecord(1, 4, 0x72);
			if (ActiveLocalDevice != NULL)
				ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}

		case kMsgSetDeviceClassSmartPhone:
		{
			devClass.SetRecord(2, 3, 0x72);
			if (ActiveLocalDevice != NULL)
				ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}

		case kMsgRefresh:
			_BuildLocalDevicesMenu();
			fLocalDevicesMenu->SetTargetForItems(this);
		break;
		default:
			BView::MessageReceived(message);
	}
}


void
BluetoothSettingsView::_BuildConnectionPolicy()
{
	BMessage* message = NULL;
	BMenuItem* item = NULL;

	fPolicyMenu = new BPopUpMenu(B_TRANSLATE("Policy..."));

	message = new BMessage(kMsgSetConnectionPolicy);
	message->AddInt8("Policy", 1);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kAllLabel), message);
	fPolicyMenu->AddItem(item);

	message = new BMessage(kMsgSetConnectionPolicy);
	message->AddBool("Policy", 2);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kTrustedLabel), message);
	fPolicyMenu->AddItem(item);

	message = new BMessage(kMsgSetConnectionPolicy);
	message->AddBool("Policy", 3);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kAlwaysLabel), NULL);
	fPolicyMenu->AddItem(item);

}


void
BluetoothSettingsView::_BuildClassMenu()
{

	fClassMenu = new BPopUpMenu(B_TRANSLATE("Identify us as..."));
	BMessage* message;

	message = new BMessage(kMsgSetDeviceClassDesktop);
	BMenuItem* item
		= new BMenuItem(B_TRANSLATE_NOCOLLECT(kDesktopLabel), message);
	fClassMenu->AddItem(item);

	message = new BMessage(kMsgSetDeviceClassServer);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kServerLabel), message);
	fClassMenu->AddItem(item);

	message = new BMessage(kMsgSetDeviceClassLaptop);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kLaptopLabel), message);
	fClassMenu->AddItem(item);

	message = new BMessage(kMsgSetDeviceClassHandheld);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kHandheldLabel), message);
	fClassMenu->AddItem(item);

	message = new BMessage(kMsgSetDeviceClassSmartPhone);
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kPhoneLabel), message);
	fClassMenu->AddItem(item);

}


void
BluetoothSettingsView::_BuildLocalDevicesMenu()
{
	LocalDevice* lDevice;

	if (!fLocalDevicesMenu)
		fLocalDevicesMenu = new BPopUpMenu(B_TRANSLATE("Pick LocalDevice..."));

	for (uint32 index = 0; index < LocalDevice::GetLocalDeviceCount(); index++) {

		lDevice = LocalDevice::GetLocalDevice();
		if (lDevice != NULL) {

			// TODO Check if they already exists
			BMessage* message = new BMessage(kMsgLocalSwitched);
			message->AddPointer("LocalDevice", lDevice);

			BMenuItem* item = new BMenuItem((lDevice->GetFriendlyName().String()),
				message);
			fLocalDevicesMenu->AddItem(item);
		}
	}
}
