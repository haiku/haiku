/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ExtendedLocalDeviceView.h"

#include <bluetooth/bdaddrUtils.h>

#include "defs.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Extended local device view"

ExtendedLocalDeviceView::ExtendedLocalDeviceView(BRect frame,
	LocalDevice* bDevice, uint32 resizingMode, uint32 flags)
	:
	BView(frame,"ExtendedLocalDeviceView", resizingMode, flags | B_WILL_DRAW),
	fDevice(bDevice),
	fScanMode(0)
{
	fDeviceView = new BluetoothDeviceView(bDevice);

	fDiscoverable = new BCheckBox("Discoverable",
		B_TRANSLATE("Discoverable"), new BMessage(SET_DISCOVERABLE));
	fVisible = new BCheckBox("Visible",
		B_TRANSLATE("Show name"), new BMessage(SET_VISIBLE));
	fAuthentication = new BCheckBox("Authenticate",
		B_TRANSLATE("Authenticate"), new BMessage(SET_AUTHENTICATION));

	SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(5)
		.Add(fDeviceView)
		.AddGroup(B_HORIZONTAL, 0)
			.SetInsets(5)
			.AddGlue()
			.Add(fDiscoverable)
			.Add(fVisible)
		.End()
		.Add(fAuthentication)
	.End();
}


ExtendedLocalDeviceView::~ExtendedLocalDeviceView(void)
{

}


void
ExtendedLocalDeviceView::SetLocalDevice(LocalDevice* lDevice)
{
	printf("ExtendedLocalDeviceView::SetLocalDevice\n");
	if (lDevice != NULL) {
		fDevice = lDevice;
		SetName(lDevice->GetFriendlyName().String());
		fDeviceView->SetBluetoothDevice(lDevice);

		ClearDevice();

		int value = fDevice->GetDiscoverable();
		printf("ExtendedLocalDeviceView::SetLocalDevice value = %d\n", value);
		if (value == 1)
			fDiscoverable->SetValue(true);
		else if (value == 2)
			fVisible->SetValue(true);
		else if  (value == 3) {
			fDiscoverable->SetValue(true);
			fVisible->SetValue(true);
		}
	}
}


void
ExtendedLocalDeviceView::AttachedToWindow()
{
	printf("ExtendedLocalDeviceView::AttachedToWindow\n");
	fDiscoverable->SetTarget(this);
	fVisible->SetTarget(this);
	fAuthentication->SetTarget(this);
}


void
ExtendedLocalDeviceView::SetTarget(BHandler* target)
{
	printf("ExtendedLocalDeviceView::SetTarget\n");
}


void
ExtendedLocalDeviceView::MessageReceived(BMessage* message)
{
	printf("ExtendedLocalDeviceView::MessageReceived\n");

	if (fDevice == NULL) {
		printf("ExtendedLocalDeviceView::Device missing\n");
		return;
	}

	if (message->WasDropped()) {

	}

	switch (message->what)
	{
		case SET_DISCOVERABLE:
		case SET_VISIBLE:
			fScanMode = 0;

			if (fDiscoverable->Value()) {
				fScanMode = 1;
				fVisible->SetEnabled(true);
			} else {
				fVisible->SetValue(false);
				fVisible->SetEnabled(false);
			}

			if (fVisible->Value())
				fScanMode |= 2;

			if (fDevice != NULL)
				fDevice->SetDiscoverable(fScanMode);

			break;
		case SET_AUTHENTICATION:
			if (fDevice != NULL)
				fDevice->SetAuthentication(fAuthentication->Value());
			break;

		default:
			BView::MessageReceived(message);
		break;
	}
}


void
ExtendedLocalDeviceView::SetEnabled(bool value)
{
	printf("ExtendedLocalDeviceView::SetEnabled\n");

	fVisible->SetEnabled(value);
	fAuthentication->SetEnabled(value);
	fDiscoverable->SetEnabled(value);
}


void
ExtendedLocalDeviceView::ClearDevice()
{
	printf("ExtendedLocalDeviceView::ClearDevice\n");

	fVisible->SetValue(false);
	fAuthentication->SetValue(false);
	fDiscoverable->SetValue(false);
}
