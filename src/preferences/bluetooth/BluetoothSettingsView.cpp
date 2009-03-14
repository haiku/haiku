/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Box.h>
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
	
static const int32 kMsgSetAntialiasing = 'anti';
static const int32 kMsgSetHinting = 'hint';
static const int32 kMsgSetAverageWeight = 'afEa';
static const int32 kMsgLocalSwitched = 'lDsW';

static const char* kAllLabel = "From all devices";
static const char* kTrustedLabel = "Only from Trusted devices";
static const char* kAlwaysLabel = "Always ask";

static const char* kDesktopLabel = "Desktop";
static const char* kLaptopLabel = "Laptop";
static const char* kPhoneLabel = "Haiku Phone";


//	#pragma mark -

BluetoothSettingsView::BluetoothSettingsView(const char* name)
	: BView(name, 0)
{
	// antialiasing menu
	_BuildConnectionPolicy();
	fAntialiasingMenuField = new BMenuField("antialiasing",
		"Incoming connections policy:", fAntialiasingMenu, NULL);

	fAverageWeightControl = new BSlider("averageWeightControl",
		"Default Inquiry time:", new BMessage(kMsgSetAverageWeight), 0, 255, B_HORIZONTAL);
	fAverageWeightControl->SetLimitLabels("15 secs", "61 secs");
	fAverageWeightControl->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAverageWeightControl->SetHashMarkCount(255 / 15);
	fAverageWeightControl->SetEnabled(true);
	
	// hinting menu
	_BuildHintingMenu();
	fHintingMenuField = new BMenuField("hinting", "Identify host as:",
		fHintingMenu, NULL);

	// localdevices menu
	_BuildLocalDevicesMenu();
	fLocalDevicesMenuField = new BMenuField("devices", "Local Devices found on system:",
		fLocalDevicesMenu, NULL);

	fExtDeviceView = new ExtendedLocalDeviceView(BRect(0,0,5,5), NULL);

	SetLayout(new BGroupLayout(B_VERTICAL));

	// controls pane
	AddChild(BGridLayoutBuilder(10, 10)

		.Add(fHintingMenuField->CreateLabelLayoutItem(), 0, 0)
		.Add(fHintingMenuField->CreateMenuBarLayoutItem(), 1, 0)

		.Add(fAntialiasingMenuField->CreateLabelLayoutItem(), 0, 1)
	 	.Add(fAntialiasingMenuField->CreateMenuBarLayoutItem(), 1, 1)
		
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 2, 2)
		
		.Add(fAverageWeightControl, 0, 3, 2)
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

	fAntialiasingMenu->SetTargetForItems(this);
	fHintingMenu->SetTargetForItems(this);
	fLocalDevicesMenu->SetTargetForItems(this);
	fAverageWeightControl->SetTarget(this);
}


void
BluetoothSettingsView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgLocalSwitched:
		{
			LocalDevice* lDevice;
			if (msg->FindPointer("LocalDevice", (void**) &lDevice) == B_OK) {
				// Device integrity should be rechecked
				fExtDeviceView->SetLocalDevice(lDevice);
				fExtDeviceView->SetEnabled(true);
				ActiveLocalDevice = lDevice;			
			}
		}
		break;
		case kMsgRefresh:
			_BuildLocalDevicesMenu();
			fLocalDevicesMenu->SetTargetForItems(this);
		break;
		default:
			BView::MessageReceived(msg);
	}
}


void
BluetoothSettingsView::_BuildConnectionPolicy()
{
	fAntialiasingMenu = new BPopUpMenu("Policy...");

	BMessage* message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", false);

	BMenuItem* item = new BMenuItem(kAllLabel, message);

	fAntialiasingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", true);

	item = new BMenuItem(kTrustedLabel, message);

	fAntialiasingMenu->AddItem(item);

	BMenuItem* item2 = new BMenuItem(kAlwaysLabel, NULL);

	fAntialiasingMenu->AddItem(item2);

}


void
BluetoothSettingsView::_BuildHintingMenu()
{
	fHintingMenu = new BPopUpMenu("Identify us as...");

	BMessage* message = new BMessage(kMsgSetHinting);
	message->AddBool("hinting", false);

	BMenuItem* item = new BMenuItem(kDesktopLabel, message);

	fHintingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetAverageWeight);
	message->AddBool("hinting", true);

	item = new BMenuItem(kLaptopLabel, message);

	fHintingMenu->AddItem(item);

	BMenuItem* item2 = new BMenuItem(kPhoneLabel, NULL);
	fHintingMenu->AddItem(item2);

}


void
BluetoothSettingsView::_BuildLocalDevicesMenu()
{
	LocalDevice* lDevice;

	if (!fLocalDevicesMenu)
		fLocalDevicesMenu = new BPopUpMenu("Pick LocalDevice...");

    for (uint32 index = 0 ; index < LocalDevice::GetLocalDeviceCount() ; index++) {

    	lDevice = LocalDevice::GetLocalDevice();
        if (lDevice != NULL) {
        	// TODO Check if they already exists
        	
			BMessage* message = new BMessage(kMsgLocalSwitched);
			message->AddPointer("LocalDevice", lDevice);
        
			BMenuItem* item = new BMenuItem((lDevice->GetFriendlyName().String()), message);
			fLocalDevicesMenu->AddItem(item);
        }
	}
}



