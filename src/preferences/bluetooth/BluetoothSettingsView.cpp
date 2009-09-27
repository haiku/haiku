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
#include <Locale.h>
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

#define TR_CONTEXT "Settings view"
	
static const int32 kMsgSetAntialiasing = 'anti';
static const int32 kMsgSetDeviceClassDesktop = 'sDCd';
static const int32 kMsgSetDeviceClassServer = 'sDCs';
static const int32 kMsgSetDeviceClassLaptop = 'sDCl';
static const int32 kMsgSetDeviceClassHandheld = 'sDCh';
static const int32 kMsgSetDeviceClassSmartPhone = 'sDCp';

static const int32 kMsgSetAverageWeight = 'afEa';
static const int32 kMsgLocalSwitched = 'lDsW';

static const char* kAllLabel = TR_MARK("From all devices");
static const char* kTrustedLabel = TR_MARK("Only from Trusted devices");
static const char* kAlwaysLabel = TR_MARK("Always ask");

static const char* kDesktopLabel = /*TR_MARK*/("Desktop");
static const char* kServerLabel = TR_MARK("Server");
static const char* kLaptopLabel = TR_MARK("Laptop");
static const char* kHandheldLabel = TR_MARK("Handheld");
static const char* kPhoneLabel = TR_MARK("Smart Phone");


//	#pragma mark -

BluetoothSettingsView::BluetoothSettingsView(const char* name)
	: BView(name, 0)
{
	// antialiasing menu
	_BuildConnectionPolicy();
	fAntialiasingMenuField = new BMenuField("antialiasing",
		TR("Incoming connections policy:"), fAntialiasingMenu, NULL);

	fAverageWeightControl = new BSlider("averageWeightControl",
		TR("Default Inquiry time:"), new BMessage(kMsgSetAverageWeight), 0, 255,
		B_HORIZONTAL);
	fAverageWeightControl->SetLimitLabels(/*TR*/("15 secs"), /*TR*/("61 secs"));
	fAverageWeightControl->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAverageWeightControl->SetHashMarkCount(255 / 15);
	fAverageWeightControl->SetEnabled(true);
	
	// hinting menu
	_BuildHintingMenu();
	fHintingMenuField = new BMenuField("hinting", TR("Identify host as:"),
		fHintingMenu, NULL);

	// localdevices menu
	_BuildLocalDevicesMenu();
	fLocalDevicesMenuField = new BMenuField("devices",
		TR("Local Devices found on system:"), fLocalDevicesMenu, NULL);

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
	
	DeviceClass devClass;
	
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
		
		case kMsgSetDeviceClassDesktop:
		{	
			devClass.SetRecord(1, 1, 0x72);
			ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}
		
		case kMsgSetDeviceClassServer:
		{	
			devClass.SetRecord(1, 2, 0x72);
			ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}
			
		case kMsgSetDeviceClassLaptop:
		{	
			devClass.SetRecord(1, 3, 0x72);
			ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}

		case kMsgSetDeviceClassHandheld:
		{	
			devClass.SetRecord(1, 4, 0x72);
			ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}
		
		case kMsgSetDeviceClassSmartPhone:
		{	
			devClass.SetRecord(2, 3, 0x72);
			ActiveLocalDevice->SetDeviceClass(devClass);
			break;
		}	

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
	fAntialiasingMenu = new BPopUpMenu(TR("Policy..."));

	BMessage* message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", false);

	BMenuItem* item = new BMenuItem(TR(kAllLabel), message);

	fAntialiasingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", true);

	item = new BMenuItem(TR(kTrustedLabel), message);

	fAntialiasingMenu->AddItem(item);

	BMenuItem* item2 = new BMenuItem(TR(kAlwaysLabel), NULL);

	fAntialiasingMenu->AddItem(item2);

}


void
BluetoothSettingsView::_BuildHintingMenu()
{	

	fHintingMenu = new BPopUpMenu(TR("Identify us as..."));
	BMessage* message;
	
	message = new BMessage(kMsgSetDeviceClassDesktop);
	BMenuItem* item = new BMenuItem(TR(kDesktopLabel), message);
	fHintingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetDeviceClassServer);
	item = new BMenuItem(TR(kServerLabel), message);
	fHintingMenu->AddItem(item);

	message = new BMessage(kMsgSetDeviceClassLaptop);
	item = new BMenuItem(TR(kLaptopLabel), message);
	fHintingMenu->AddItem(item);

	message = new BMessage(kMsgSetDeviceClassHandheld);
	item = new BMenuItem(TR(kHandheldLabel), message);
	fHintingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetDeviceClassSmartPhone);
	item = new BMenuItem(TR(kPhoneLabel), message);
	fHintingMenu->AddItem(item);


}


void
BluetoothSettingsView::_BuildLocalDevicesMenu()
{
	LocalDevice* lDevice;

	if (!fLocalDevicesMenu)
		fLocalDevicesMenu = new BPopUpMenu(TR("Pick LocalDevice..."));

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



